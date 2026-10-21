// Original short biographies for the appended late-1918 operations.
// V37 supplies four reference-matched likenesses; returning commanders reuse V30 portraits.
// Primary/archive research:
// Horne, Rawlinson, Byng: IWM 132, https://film.iwmcollections.org.uk/record/4196
// Horne on 26 August: DND Canadian Airmen and the First World War, p552,
// https://www.canada.ca/content/dam/themes/defence/caf/militaryhistory/dhh/popular/airmen-ww1-12.pdf
// Below appointments: Bundesarchiv, Die deutschen Heeresgruppen im Ersten Weltkrieg,
// https://www.bundesarchiv.de/themen-entdecken/online-entdecken/themenbeitraege/die-deutschen-heeresgruppen-im-ersten-weltkrieg/
// Carlowitz: Peter Mertens, original ISGV biography (9 August 2007),
// https://saebi.isgv.de/biografie/Hans_Carl_Adolph_von_Carlowitz_%281858-1928%29
// Mudra: LWL regional history research, appoints 17th Army on 12 October,
// https://www.lwl.org/westfaelische-geschichte/nstopo/strnam/Begriff_413.html
// Carlowitz's appointment is variously dated 22/24 September; these bios avoid
// that disputed exact day. All references place him in command by 29 September.
#pragma once
namespace WFV32Commanders
{
struct FBiography
{
    const TCHAR* Name;
    const TCHAR* Role;
    const TCHAR* Text;
};
inline const FBiography& GetForMission(bool bGerman,int32 MissionIndex)
{
    static const FBiography British[4]={
        {TEXT("亨利·霍恩"),TEXT("英军第一集团军 · 斯卡尔普，1918"),
         TEXT("第一集团军司令。八月二十六日，他的部队以加拿大军为先导，在斯卡尔普河与桑塞河掩护的翼侧向东推进。眼前的任务是越过旧阵地，让炮兵与补给跟上持续变化的战线。")},
        {TEXT("亨利·罗林森"),TEXT("英军第四集团军 · 圣康坦运河，1918"),
         TEXT("第四集团军由亚眠继续向兴登堡防线推进。澳大利亚与配属美军在贝利库尔作战，其他渡口同样需要炮火、步兵与工兵配合；运河的深切地形让每一条通路都至关重要。")},
        {TEXT("亨利·罗林森"),TEXT("英军第四集团军 · 塞勒河，1918"),
         TEXT("突破兴登堡防线之后，第四集团军继续追击到塞勒河。曾经依托连续战壕的战争再次转入河谷与村落，桥头、道路和邻军联络决定了推进能否持续。")},
        {TEXT("朱利安·宾"),TEXT("英军第三集团军 · 桑布尔北翼，1918"),
         TEXT("第三集团军在百日攻势中继续向东推进，新西兰师也在其序列内。勒凯努瓦的城墙与桑布尔战区的村落仍有守军；停战以前的最后一周，前线的任务并没有因胜利将近而变得轻松。")}
    };
    static const FBiography German[4]={
        {TEXT("奥托·冯·贝洛"),TEXT("德军第十七集团军 · 斯卡尔普，1918"),
         TEXT("先后经历东线、马其顿和意大利战场，二月起接掌第十七集团军。春季的进攻已转为夏末的防御，他的部队需要用旧阵地和后卫争取时间，抵挡斯卡尔普方向的持续压力。")},
        {TEXT("阿道夫·冯·卡洛维茨"),TEXT("德军第二集团军 · 圣康坦运河，1918"),
         TEXT("萨克森军人，曾任王国战争大臣并指挥军级部队。九月下旬接掌第二集团军时，兴登堡防线正面临协约军的新一轮攻势；他的职责是让分散的支点继续构成能够互相支援的防御。")},
        {TEXT("阿道夫·冯·卡洛维茨"),TEXT("德军第二集团军 · 塞勒河，1918"),
         TEXT("第二集团军在持续后撤中寻找新的立足点。河流和铁路可以帮助后卫争取时间，却无法替代步兵守住通路；从纵深防线到河谷阵地，保存联络已和保存兵力同样困难。")},
        {TEXT("布鲁诺·冯·穆德拉"),TEXT("德军第十七集团军 · 桑布尔北翼，1918"),
         TEXT("工兵出身，长期从事工程兵与要塞事务，曾在阿尔贡指挥作战。十月十二日接掌第十七集团军之后，他面对的是不断后移的战线；河岸、村落与有限的交通线成为最后的后卫依托。")}
    };
    const int32 I=FMath::Clamp(MissionIndex-6,0,3);
    return bGerman?German[I]:British[I];
}
}
