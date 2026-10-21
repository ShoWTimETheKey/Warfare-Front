#include "WarfareFrontlineArmourValourV8221.h"
#include "WarfareFRRUCampaignV82.h"

namespace FrontlineArmourValourV8221
{
FDisplay Display(EFRRUNationV81 Nation,int32 ConfirmedKills)
{
    FDisplay R;R.Nation=Nation;R.Total=FMath::Clamp(ConfirmedKills,0,20000);
    const int32 N=int32(Nation);if(N<0||N>3){R.Total=0;R.NextAt=0;return R;}
    R.Available=N<2||WarfareFRRUCampaignV82::CampaignsAvailable(Nation);
    static constexpr int32 Gates[]={1,5,15,40};
    const int32 Maximum=N==1?2:4;
    for(int32 I=0;I<Maximum&&R.Total>=Gates[I];++I)++R.Distinctions;
    R.NextAt=R.Distinctions<Maximum?Gates[R.Distinctions]:0;
    R.Device=N==0?EDevice::SilverRosettes:N==1?EDevice::IronCrossPin:
        N==2?EDevice::CitationDevices:EDevice::GeorgeClass;
    return R;
}
FString BannerTitle(bool ZH)
{return ZH?TEXT("英勇行为：步兵击毁装甲"):TEXT("ACT OF VALOUR: INFANTRY ARMOUR KILL");}
FString RibbonLabel(const FDisplay& R,bool ZH)
{
    if(!R.Available)return ZH?TEXT("暂未开放"):TEXT("NOT YET AVAILABLE");
    if(!R.Distinctions)return ZH?TEXT("尚无记录"):TEXT("NO DEEDS RECORDED");
    switch(R.Device)
    {
    case EDevice::SilverRosettes:
        return R.Distinctions==1?(ZH?TEXT("英勇行为纪念勋略"):TEXT("ACT OF VALOUR RIBBON")):
            (ZH?FString::Printf(TEXT("复授银饰 · %d"),R.Distinctions-1):FString::Printf(TEXT("SILVER ROSETTES · %d"),R.Distinctions-1));
    case EDevice::IronCrossPin:
        return R.Distinctions==1?(ZH?TEXT("英勇行为纪念胸章"):TEXT("ACT OF VALOUR BREAST BADGE")):
            (ZH?TEXT("英勇行为胸章 · 复授"):TEXT("ACT OF VALOUR BREAST BADGE · REPEAT AWARD"));
    case EDevice::CitationDevices:
        {static const TCHAR* EN[]={TEXT("BRONZE STAR"),TEXT("SILVER STAR"),TEXT("SILVER-GILT STAR"),TEXT("BRONZE PALM")};static const TCHAR* CN[]={TEXT("铜星嘉奖"),TEXT("银星嘉奖"),TEXT("镀金银星嘉奖"),TEXT("铜棕榈枝嘉奖")};return ZH?CN[FMath::Clamp(R.Distinctions-1,0,3)]:EN[FMath::Clamp(R.Distinctions-1,0,3)];}
    case EDevice::GeorgeClass:
        {static const TCHAR* EN[]={TEXT("SILVER"),TEXT("SILVER AND BOW"),TEXT("GOLD"),TEXT("GOLD AND BOW")};static const TCHAR* CN[]={TEXT("银质嘉奖"),TEXT("银质与勋带结"),TEXT("金质嘉奖"),TEXT("金质与勋带结")};return ZH?CN[FMath::Clamp(R.Distinctions-1,0,3)]:EN[FMath::Clamp(R.Distinctions-1,0,3)];}
    default:return {};
    }
}
FString Description(const FDisplay& R,bool ZH)
{
    if(!R.Available)return ZH?TEXT("本军战役暂未开放。"):TEXT("This army's campaign is not yet available.");
    FString Text;
    if(R.Nation==EFRRUNationV81::British)
        Text=ZH?FString::Printf(TEXT("银饰记下步兵迎击战车的战功。这是本作的战绩纪念章。\n步兵击毁装甲：%d 辆"),R.Total):
            FString::Printf(TEXT("Silver rosettes record infantry deeds against armour. A commemorative award created for Warfare Front.\nInfantry armour kills: %d"),R.Total);
    else if(R.Nation==EFRRUNationV81::German)
        Text=ZH?FString::Printf(TEXT("橡叶环抱折断的履带，记下步兵迎击战车的战功。这是本作的战绩纪念章。\n步兵击毁装甲：%d 辆"),R.Total):
            FString::Printf(TEXT("Oak leaves enclose a broken track, recording infantry deeds against armour. A commemorative award created for Warfare Front.\nInfantry armour kills: %d"),R.Total);
    if(R.Nation==EFRRUNationV81::French||R.Nation==EFRRUNationV81::Russian)
        Text=ZH?FString::Printf(TEXT("勋饰记下步兵迎击战车的战功。这是本作的战绩纪念章。\n步兵击毁装甲：%d 辆"),R.Total):FString::Printf(TEXT("The insignia records infantry deeds against armour. A commemorative award created for Warfare Front.\nInfantry armour kills: %d"),R.Total);
    if(R.NextAt>0)Text+=ZH?FString::Printf(TEXT("\n下次嘉奖：%d 辆"),R.NextAt):
        (R.NextAt==1?FString::Printf(TEXT("\nNext distinction: %d vehicle"),R.NextAt):FString::Printf(TEXT("\nNext distinction: %d vehicles"),R.NextAt));
    return Text;
}
TArray<FString> AssetPaths()
{
    TArray<FString> Paths={TEXT("/Game/Warfare/UI/V8221/T_CitationStarV8221.T_CitationStarV8221"),
        TEXT("/Game/Warfare/UI/V8221/T_StGeorgeBowV8221.T_StGeorgeBowV8221"),
        TEXT("/Game/Warfare/UI/V8216/T_MedalAttachment_SilverRosette_V8216.T_MedalAttachment_SilverRosette_V8216"),
        TEXT("/Game/Warfare/UI/V8216/T_MedalAttachment_Palm_V8216.T_MedalAttachment_Palm_V8216"),
        TEXT("/Game/Warfare/UI/V8216/T_Medal_de_iron_cross_ii_V8216_first.T_Medal_de_iron_cross_ii_V8216_first"),
        TEXT("/Game/Warfare/UI/V8216/T_Medal_ru_st_george_cross_V8216.T_Medal_ru_st_george_cross_V8216"),
        TEXT("/Game/Warfare/UI/V8216/T_Medal_ru_st_george_cross_V8216_second.T_Medal_ru_st_george_cross_V8216_second")};
    for(int32 N=0;N<4;++N){Paths.Add(BadgePath(EFRRUNationV81(N)));Paths.Add(RibbonPath(EFRRUNationV81(N)));}
    return Paths;
}
FString BadgePath(EFRRUNationV81 Nation)
{
    const int32 N=int32(Nation);if(N<0||N>3)return {};
    static const TCHAR* Codes[]={TEXT("UK"),TEXT("DE"),TEXT("FR"),TEXT("RU")};
    const FString Version=N==1?TEXT("V837"):TEXT("V836");
    const FString Stem=FString(TEXT("T_ArmourValour_"))+Codes[N]+TEXT("_")+Version;
    return TEXT("/Game/Warfare/UI/")+Version+TEXT("/Valour/")+Stem+TEXT(".")+Stem;
}
FString RibbonPath(EFRRUNationV81 Nation)
{
    const int32 N=int32(Nation);if(N<0||N>3)return {};
    static const TCHAR* Codes[]={TEXT("UK"),TEXT("DE"),TEXT("FR"),TEXT("RU")};
    const FString Stem=FString(TEXT("T_ArmourValourRibbon_"))+Codes[N]+TEXT("_V836");
    return TEXT("/Game/Warfare/UI/V836/Valour/")+Stem+TEXT(".")+Stem;
}
FIntRect RibbonTextileCutV837(EFRRUNationV81 Nation)
{
    // Native alpha readback: each crop is centred in the complete opaque
    // woven bar, four texels inside the fray. No new pixels or resampling.
    static const FIntRect Cuts[]={FIntRect(157,22,981,228),FIntRect(161,22,977,226),FIntRect(165,22,973,224),FIntRect(161,23,977,227)};
    const int32 N=int32(Nation);return N>=0&&N<4?Cuts[N]:FIntRect(0,0,0,0);
}
FIntPoint RibbonSourceSizeV837(EFRRUNationV81 Nation)
{
    static const FIntPoint Sizes[]={FIntPoint(1138,249),FIntPoint(1139,248),FIntPoint(1139,246),FIntPoint(1138,249)};
    const int32 N=int32(Nation);return N>=0&&N<4?Sizes[N]:FIntPoint(0,0);
}
void RunChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    using N=EFRRUNationV81;
    Check(TEXT("v8221_valour_exact_chinese_banner"),BannerTitle(true)==TEXT("英勇行为：步兵击毁装甲"));
    Check(TEXT("v8221_valour_bilingual_title"),!BannerTitle(false).IsEmpty());
    TSet<FString> NationalBadgePaths;for(int32 N=0;N<4;++N)NationalBadgePaths.Add(BadgePath(EFRRUNationV81(N)));
    Check(TEXT("v8222_valour_four_distinct_national_emblems"),NationalBadgePaths.Num()==4&&!NationalBadgePaths.Contains(FString())&&BadgePath(N::Invalid).IsEmpty());
    TSet<FString> NationalRibbonPaths;for(int32 I=0;I<4;++I)NationalRibbonPaths.Add(RibbonPath(EFRRUNationV81(I)));
    Check(TEXT("v836_valour_four_independent_commemorative_ribbons"),NationalRibbonPaths.Num()==4&&!NationalRibbonPaths.Contains(FString())&&RibbonPath(N::Invalid).IsEmpty());
    bool TextileFits=true;for(int32 I=0;I<4;++I){const auto C=RibbonTextileCutV837(N(I));const auto S=RibbonSourceSizeV837(N(I));TextileFits&=C.Width()==C.Height()*4&&C.Min.X>=0&&C.Min.Y>=0&&C.Max.X<=S.X&&C.Max.Y<=S.Y;}
    Check(TEXT("v837_valour_four_opaque_textiles_share_exact_four_to_one_aspect"),TextileFits&&RibbonTextileCutV837(N::Invalid).Width()==0);
    Check(TEXT("v8221_valour_zero_has_no_devices"),Display(N::British,0).Distinctions==0&&Display(N::German,0).Distinctions==0);
    Check(TEXT("v8221_valour_british_three_true_repeat_devices"),Display(N::British,40).Distinctions==4&&Display(N::British,20000).Distinctions==4);
    Check(TEXT("v8221_valour_german_no_grand_cross_for_tank_count"),Display(N::German,5).Distinctions==2&&Display(N::German,20000).Distinctions==2);
    Check(TEXT("v87_valour_national_awards_follow_campaign_gate"),Display(N::French,40).Available==WarfareFRRUCampaignV82::CampaignsAvailable(N::French)&&Display(N::Russian,40).Available==WarfareFRRUCampaignV82::CampaignsAvailable(N::Russian));
    Check(TEXT("v8221_valour_four_french_citation_forms"),Display(N::French,40).Distinctions==4);
    Check(TEXT("v8221_valour_four_russian_real_classes"),Display(N::Russian,40).Distinctions==4);
    Check(TEXT("v8221_valour_invalid_and_negative_bounded"),Display(N::Invalid,INT32_MAX).Total==0&&Display(N::British,-1).Distinctions==0);
    bool Boundaries=true;for(int32 K:{1,5,15,40}){const auto Before=Display(N::British,K-1),After=Display(N::British,K);Boundaries&=After.Distinctions==Before.Distinctions+1;}
    Check(TEXT("v8221_valour_exact_milestone_boundaries"),Boundaries);
    bool Monotonic=true;int32 Last=0;for(int32 T=0;T<=20000;++T){const auto R=Display(N::British,T);Monotonic&=R.Total==T&&R.Distinctions>=Last&&R.Distinctions<=4;Last=R.Distinctions;}
    Check(TEXT("v8221_valour_each_deed_retained_after_highest_distinction"),Monotonic);
}
}
