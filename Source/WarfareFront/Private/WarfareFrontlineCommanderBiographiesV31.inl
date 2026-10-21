// Original V31 hover biographies; historical basis: Docs/V31-MAP-ART-SOURCES.md.
// Army commanders identify the selected campaign sector, not the player's company.
#pragma once
namespace WFV31Commanders
{
struct FBiography
{
    const TCHAR* Name;
    const TCHAR* Role;
    const TCHAR* Text;
};
// Five British portraits followed by the three German portraits in V30 atlas order.
static const FBiography Biographies[8] =
{
    {TEXT("埃德蒙·艾伦比"),TEXT("英军第三集团军 · 阿拉斯，1917"),
     TEXT("骑兵军官出身，1915年起统率第三集团军。阿拉斯攻势中，他试图以密集炮火打开斯卡普河谷；雪幕下的突进很快又受到德军阵地与补给距离的牵制。")},
    {TEXT("赫伯特·普卢默"),TEXT("英军第二集团军 · 梅西讷，1917"),
     TEXT("一位重视周密准备的集团军司令。梅西讷的地道、炮兵与步兵计划在他麾下合为一体：先夺山脊，再稳固阵地，让反击撞上已经转向的炮口。")},
    {TEXT("休伯特·高夫"),TEXT("英军第五集团军 · 伊普尔与圣康坦"),
     TEXT("1917年，他指挥第五集团军向皮尔肯山脊进攻；1918年春，同一支部队又被拉到漫长的圣康坦正面。从追求突破到掩护撤退，他的战绩始终伴随着激烈争议。")},
    {TEXT("朱利安·宾"),TEXT("英军第三集团军 · 康布雷，1917"),
     TEXT("曾率加拿大军攻取维米岭，随后接掌第三集团军。康布雷的坦克、步兵与炮火协同显示了新的突破可能；十天后的德军反击，也暴露了突出部难以巩固的代价。")},
    {TEXT("亨利·罗林森"),TEXT("英军第四集团军 · 亚眠，1918"),
     TEXT("1918年8月，他在亚眠组织第四集团军的突击。英国第三军与澳大利亚、加拿大两军分据索姆河两侧，在隐蔽集结与炮坦协同下展开攻势；法国第一集团军在南翼并肩作战。")},
    {TEXT("路德维希·冯·法尔肯豪森"),TEXT("德军第六集团军 · 阿拉斯，1917"),
     TEXT("经历过普奥、普法战争的普鲁士老将。1917年4月，他统率第六集团军面对阿拉斯攻势。阵地与预备队未能及时止住英军最初的突入，他在当月离开前线指挥岗位。")},
    {TEXT("弗里德里希·西克斯特·冯·阿尔明"),TEXT("德军第四集团军 · 佛兰德，1917"),
     TEXT("1917年指挥第四集团军守卫佛兰德。从梅西讷山脊到伊普尔突出部，他的部队依托混凝土据点、纵深阵地与反击部队抵挡协约国攻势，也承受了矿坑与泥海中的惨重消耗。")},
    {TEXT("格奥尔格·冯·德·马维茨"),TEXT("德军第二集团军 · 康布雷至亚眠"),
     TEXT("第二集团军司令。1917年康布雷反击、1918年春的迈克尔攻势，都有他麾下部队的身影；到了亚眠，这支军队转而面对协约国的炮兵、坦克和步兵协同突击。")}
};
inline const FBiography& GetForPortrait(bool bGerman, int32 PortraitSlot)
{
    return Biographies[bGerman ? 5+FMath::Clamp(PortraitSlot,0,2) : FMath::Clamp(PortraitSlot,0,4)];
}
inline const FBiography& GetForMission(bool bGerman, int32 MissionIndex)
{
    static const int32 BritishSlots[6]={0,1,2,3,2,4};
    static const int32 GermanSlots[6]={0,1,1,2,2,2};
    const int32 I=FMath::Clamp(MissionIndex,0,5);
    return GetForPortrait(bGerman,bGerman?GermanSlots[I]:BritishSlots[I]);
}
}
