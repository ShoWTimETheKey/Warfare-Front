// Generated from ArtSource/Text/V841/interface.json. Do not edit.
struct FManualPageV841
{
    const TCHAR* TitleZh;
    const TCHAR* TitleEn;
    const TCHAR* const* SectionsZh;
    const TCHAR* const* SectionsEn;
    int32 Count;
    const TCHAR* SummaryZh;
    const TCHAR* SummaryEn;
};

const TCHAR* const ManualSectionsZh0V841[] = {
    TEXT("鼠标靠近友壕时打开命令面板，离开后收起。点击一支驻军的肖像令它出击；整壕前进令全体守军出发。"),
    TEXT("关闭入壕不会令驻军出击。后续小队从壕旁通过，满员的友壕也能绕过。进入敌壕前，先清除守军。"),
    TEXT("军士与军官附近的友军获得指挥。金星标示指挥者，银星标示受指挥的小队。多名指挥者同时在场时，采用较强的指挥。"),
    TEXT("指挥者受重压、阵亡或离开部队后，指挥中断。军士停步时重整；德军军官在行进中也能重整。"),
};
const TCHAR* const ManualSectionsEn0V841[] = {
    TEXT("Move the pointer near a friendly trench to open its orders; move away to close them. Select a garrison portrait to advance that section, or send the whole trench forward."),
    TEXT("Closing entry leaves the garrison in place and lets following sections pass. Full friendly trenches can also be passed. Clear the defenders before entering an enemy trench."),
    TEXT("NCOs and officers command nearby troops. Gold stars mark the leaders; silver stars mark sections under orders. Where several leaders are present, the strongest command is used."),
    TEXT("Heavy suppression, death or separation breaks command. NCOs rally troops while halted; German officers can also rally them on the move."),
};

const TCHAR* const ManualSectionsZh1V841[] = {
    TEXT("清除敌军后，步兵连续驻守 8 秒可夺取工事。至少两名守军在位时，工事才提供火力或补给。敌军进入后，夺取过程受阻。"),
    TEXT("工兵在无敌军争夺的友壕内停步施工。连续作业 12 秒完成壕肩工事；受到压制时施工中断。"),
    TEXT("完成工事并留守两人，正面枪弹命中率降低 22%。火焰与侧翼火力可绕过掩护，炮击能摧毁工事。"),
    TEXT("工兵接近可疑地面后检查地雷。掩护工兵作业，避免其他小队抢先穿过危险地段。"),
};
const TCHAR* const ManualSectionsEn1V841[] = {
    TEXT("After clearing the enemy, hold a position with infantry for 8 uninterrupted seconds to capture it. At least two defenders are needed to operate its guns or stores. Enemy troops interrupt capture."),
    TEXT("Pioneers halt to work in an uncontested friendly trench. Twelve uninterrupted seconds complete the revetment; suppression interrupts construction."),
    TEXT("A completed revetment with two defenders reduces frontal bullet hit chance by 22%. Flame and flanking fire bypass its cover; shellfire can destroy it."),
    TEXT("Pioneers inspect suspect ground as they approach. Cover their work and keep other sections from entering the dangerous ground first."),
};

const TCHAR* const ManualSectionsZh2V841[] = {
    TEXT("车体、行走机构与机枪可分别损坏。车体被毁才记为击毁；履带损坏会限制行进，损坏的枪位不能开火。"),
    TEXT("第 9 次行动起，工兵可抢修履带。连续作业 12 秒恢复该侧 50% 耐久，每侧可修一次；附近交火或重压会中断作业。"),
    TEXT("研究枪位抢修后，工兵可修复一处损坏机枪。连续作业 8 秒恢复 50% 耐久，每辆可修一处；11 米内有敌军时不能维修。"),
    TEXT("两种配属装甲共用整备，各型等待时间不同。Mark IV 雄型与雌型在出战前择一；换装只影响之后出动的车辆。"),
};
const TCHAR* const ManualSectionsEn2V841[] = {
    TEXT("Hull, running gear and machine guns can be damaged separately. Only a destroyed hull counts as a kill. Broken tracks limit movement and damaged gun stations cannot fire."),
    TEXT("From operation 9, pioneers can repair tracks. Twelve uninterrupted seconds restore 50% durability to that side, once per side. Nearby fighting or heavy pressure interrupts work."),
    TEXT("After gun-recovery research, pioneers can restore one damaged machine gun per vehicle. Eight uninterrupted seconds restore 50% durability. Work cannot proceed with enemy troops or armour within 11 metres."),
    TEXT("The two assigned vehicles share readiness, with a different wait for each model. Choose Male or Female Mark IV before departure; the choice affects vehicles called out afterwards."),
};

const TCHAR* const ManualSectionsZh3V841[] = {
    TEXT("战斗与电报带来研究点。先完成所列前置研究，再研究相接的条令。装备仍依作战命令配属。"),
    TEXT("完成观察研究后，指挥组与同队一名同伴停稳 2 秒，并看清目标，即可校射。野炮与迫击炮首轮提前 2.5 秒，散布缩小 30%。"),
    TEXT("研究伴随枪组后，在出战序列中切换固定与伴随编组。两种编制共用整备，改动只影响之后征募的小队。"),
    TEXT("步兵贴近坦克，在 9 米内帮助车载机枪辨认目标。完成步坦协同研究后，坦克机枪命中率提高 10%；附近更多步兵不会继续提高。"),
    TEXT("驻壕士兵受压较低，5 秒没有发现目标且弹仓不足半数时，会自行装填。德军留守掩护研究令枪组掩护整壕冲锋；前方 16 米内有己方冲锋步兵时，火力得到加强。"),
};
const TCHAR* const ManualSectionsEn3V841[] = {
    TEXT("Battles and dispatches provide research points. Complete the listed prerequisites before researching the next doctrine. Equipment is assigned through operation orders."),
    TEXT("After observation research, a command section can register fire by halting for 2 seconds with a section companion and a clear view of the target. The opening field-gun or mortar salvo arrives 2.5 seconds sooner; scatter is reduced by 30%."),
    TEXT("After researching mobile gun sections, switch between fixed and mobile crews in the order of battle. Both share readiness; the change affects sections recruited afterwards."),
    TEXT("Infantry within 9 metres help the tank gunners identify targets. Close-escort research raises tank machine-gun accuracy by 10%; additional infantry do not increase it further."),
    TEXT("Men in the trench top up their magazines when suppression is low, no target has appeared for 5 seconds and less than half the ammunition remains. German rear-cover research keeps gun sections firing during a trench advance; assault troops within 16 metres ahead receive stronger covering fire."),
};

const FManualPageV841 ManualPagesV841[] = {
    {TEXT("命令面板"),TEXT("Trench orders"),ManualSectionsZh0V841,ManualSectionsEn0V841,4,TEXT("查看驻军，决定哪些小队出击、哪些留守。关闭入壕，让后续部队通过。"),TEXT("Review the garrison and decide which sections advance and which remain. Close entry to let following troops pass.")},
    {TEXT("战场工事"),TEXT("Field positions"),ManualSectionsZh1V841,ManualSectionsEn1V841,4,TEXT("先清除敌军，再让步兵接管工事。守军留在阵位，设备才能继续提供火力或补给。"),TEXT("Clear the enemy, then let infantry take over the position. Its defenders keep the guns or stores in operation.")},
    {TEXT("装甲部件"),TEXT("Armour parts"),ManualSectionsZh2V841,ManualSectionsEn2V841,4,TEXT("靠近装甲查看车体、行走机构与武器状况。掩护工兵维修，受损战车仍可能遭到炮击。"),TEXT("Inspect the hull, running gear and weapons by pointing at the vehicle. Cover the pioneers during repairs; damaged armour remains exposed to shellfire.")},
    {TEXT("研究"),TEXT("Research"),ManualSectionsZh3V841,ManualSectionsEn3V841,5,TEXT("战前完成研究，再调整出战序列。先研究所列前置条令，装备依作战命令配属。"),TEXT("Conduct research before reviewing deployment. Complete the listed prerequisite doctrines; equipment follows operation orders.")},
};
