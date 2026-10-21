#if WITH_EDITOR
#include "FRRU/FRRUParityReviewSafetyV87.h"
namespace FrontlineAchievementsV76
{
struct FNationalParityFixtureV87
{
    FAccount Before;
    FTransientReviewV81 Queues;
    FNationalParityFixtureV87():Before(Account){}
    ~FNationalParityFixtureV87(){Account=Before;}
};
TSharedPtr<FNationalParityFixtureV87> BeginNationalParityFixtureV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString User;const bool Safe=B&&B->IsProgressionVolatile()&&B->Phase==EFrontlinePhase::Briefing
        &&FRRUParityReviewSafetyV87::UserDir(User)&&FPaths::IsSamePath(AccountPath(),User/TEXT("AccountReview/achievements-v2.json"));
    Check(TEXT("v87_parity_account_exact_isolated_profile"),Safe);if(!Safe)return {};
    Load();if(Account.Corrupt||Account.Unsupported){Check(TEXT("v87_parity_account_readable"),false);return {};}
    auto Token=MakeShared<FNationalParityFixtureV87>();
    Account=FAccount();Account.Loaded=true;Account.Routes=15;
    // Explicit synthetic career/qualification fixture. The normal save,
    // receipt merger and award evaluator run below on this disposable account;
    // these rows do not demonstrate any naturally completed campaign.
    for(int32 N:{2,3})
    {
        auto& S=Account.Medals[N];S.Infantry=N==2?2000:3000;S.Armour=N==2?50:60;S.Victories=40;S.CleanVictories=20;
    }
    FrontlineMedalsV8215::RunCatalogueChecks(Check);FrontlineMedalsV8215::RunExtensionChecksV84412(Check);
    RunNationalArmourTallyCodecChecksV87(Check);
    for(int32 N:{2,3})
    {
        const auto Before=Account.Medals[N];const auto Other=Account.Medals[5-N];
        const FGuid Campaign=FGuid::NewGuid(),Receipt=FGuid::NewGuid();FSettlementV81 Settlement;Settlement.CampaignV8215=Campaign;
        FrontlineMedalsV8215::FStats D;D.Infantry=80+N;D.Armour=2;D.Victories=1;D.CleanVictories=1;D.NaturalRoute=1;
        Settlement.MedalOutcomesV8215.Add(Receipt,{N,D});
        FrontlineMedalsV8215::FProof Proof;Proof.Campaign=Campaign;Proof.Nation=N;Proof.Date=N==2?19171024:19160701;Proof.Difficulty=2;
        Proof.Infantry=D.Infantry;Proof.Armour=D.Armour;Proof.Obstacles=5;Proof.EnteredInfantryV842=40;Proof.ReturnedInfantryV842=32;
        Proof.Won=Proof.Completed=Proof.Undefeated=true;
        Settlement.MedalProofsV8216.Add(Receipt,Proof);
        MergeSettlementV8219(SettlementsV81.FindOrAdd(Campaign),Settlement);PersistSettlementsV81();
        Check(FString::Printf(TEXT("v87_nation_%d_production_atomic_settlement_uses_own_counter"),N),
            Account.MedalReceiptsV8215.Contains(Receipt)&&Account.Medals[N].Infantry==Before.Infantry+D.Infantry
            &&Account.Medals[N].Armour==Before.Armour+D.Armour&&Account.Medals[N].Victories==Before.Victories+1
            &&Account.Medals[5-N].Infantry==Other.Infantry&&Account.Medals[0].Infantry==0&&Account.Medals[1].Infantry==0);
        Check(FString::Printf(TEXT("v87_nation_%d_production_settlement_awards_expected_real_base"),N),
            Account.MedalAwardsV8216.FindRef(FrontlineMedalsV8215::Definition(N==2?6:9).Id).Grade==1);
        const auto Committed=Account.Medals[N];MergeSettlementV8219(SettlementsV81.FindOrAdd(Campaign),Settlement);PersistSettlementsV81();
        Check(FString::Printf(TEXT("v87_nation_%d_duplicate_receipt_does_not_repeat_counts_or_grade"),N),
            Account.Medals[N].Infantry==Committed.Infantry&&Account.Medals[N].Armour==Committed.Armour
            &&Account.Medals[N].Victories==Committed.Victories&&Account.MedalAwardsV8216.FindRef(FrontlineMedalsV8215::Definition(N==2?6:9).Id).Grade==1);
    }
    const auto Expected=Account;Account=FAccount();Load();
    Check(TEXT("v87_four_country_account_cold_reload_preserves_distinct_production_receipts"),!Account.Corrupt&&!Account.Unsupported
        &&Account.Medals[2].Infantry==Expected.Medals[2].Infantry&&Account.Medals[3].Infantry==Expected.Medals[3].Infantry
        &&Account.MedalReceiptsV8215.Num()==2&&Account.Routes==15);
    FString Text;TSharedPtr<FJsonObject> Json;
    const bool Read=FFileHelper::LoadFileToString(Text,*AccountPath())&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json)&&Json.IsValid();
    Check(TEXT("v87_production_four_country_account_json_readable"),Read);
    if(Read)
    {
        auto Rows=Json->GetArrayField(TEXT("medal_stats_v8215"));Check(TEXT("v87_production_writer_emits_four_rows"),Rows.Num()==4);
        if(Rows.Num()==4)
        {
            Rows.SetNum(2);Json->SetArrayField(TEXT("medal_stats_v8215"),Rows);Json->SetNumberField(TEXT("schema"),8);
            FString Old;FJsonSerializer::Serialize(Json.ToSharedRef(),TJsonWriterFactory<>::Create(&Old));FAccount Decoded;
            Check(TEXT("v87_old_two_country_account_decodes_without_forging_fr_ru_counters"),Decode(Old,Decoded)&&Decoded.Medals[2].Infantry==0&&Decoded.Medals[3].Infantry==0);
            Json->SetNumberField(TEXT("schema"),9);Old.Empty();FJsonSerializer::Serialize(Json.ToSharedRef(),TJsonWriterFactory<>::Create(&Old));
            Check(TEXT("v87_four_country_schema_rejects_two_missing_rows"),!Decode(Old,Decoded));
        }
    }
    return Token;
}
}
#endif
