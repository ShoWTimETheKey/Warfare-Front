// V60_EDITED_COPY: dates and event IDs retained; live prose maintained here.
// Authored by Scripts/Gameplay/Author-DispatchesV50.py. Stable IDs are save-receipt bits.
static FHistoricalEvent DispatchV50(EFrontlineFaction Faction,int32 Id)
{
    const bool G=Faction==EFrontlineFaction::German;FHistoricalEvent E;E.Id=Id;
    switch(Id)
    {
    case 0:
        E.Title=G?TEXT("连部电话簿"):TEXT("第一张行军表");
        E.Body=G?TEXT("阿拉斯前哨的电话中断。联络兵沿交通壕寻找失联分队，连部重新标出可用枪位。"):TEXT("阿拉斯地道里的部队等待出发。炮兵测地表已送到连部，军士核对各班的行军路线。");
        E.Date=TEXT("9 APRIL 1917");E.ArtKey=TEXT("Dispatch");E.ResearchAward=1;E.Consequence=TEXT("获得 1 研究点，可立即用于条令训练。");break;
    case 3:
        E.Title=G?TEXT("备用线还在"):TEXT("坑岸上的测地桩");
        E.Body=G?TEXT("爆破震断山脊电话，反斜面接线箱仍可通话。观察员把幸存枪位报回炮兵。"):TEXT("梅西讷攻击前，测绘员复核爆破点。澳新邻段送来接合部草图，炮兵据此修订火力表。");
        E.Date=TEXT("7 JUNE 1917");E.ArtKey=TEXT("Anzac");E.ResearchAward=1;E.Consequence=TEXT("获得 1 研究点。梅西讷两次行动中，首轮野炮提前 22 秒；首轮烟幕后再整备缩短 8 秒。");break;
    case 5:
        E.Title=G?TEXT("肩上的最后一箱"):TEXT("木板路上的弹药");
        E.Body=G?TEXT("炮车陷在皮尔克姆泥地，携弹队改用肩背运送壕炮弹。少数木板路承担了整段前沿的补给。"):TEXT("佛兰德排水沟被炮火切断。重炮弹滞留后方，携弹队沿木板路把壕炮弹送到前沿。");
        E.Date=TEXT("31 JULY 1917");E.ArtKey=TEXT("Dispatch");E.ResearchAward=1;E.Consequence=TEXT("获得 1 研究点。皮尔克姆与朗厄马尔克的迫击炮提前 18 秒，首轮野炮延后 24 秒。");break;
    case 7:
        E.Title=G?TEXT("反击队集合处"):TEXT("先拿下，再站稳");
        E.Body=G?TEXT("反击队在路口与炮兵观察员会合。气弹送到炮位，突击路线重新标在同一张图上。"):TEXT("梅宁路的火力表划定了短距离推进目标。步兵夺取林缘后就地固守，预备队等待德军反击。");
        E.Date=TEXT("20 SEPTEMBER 1917");E.ArtKey=TEXT("Dispatch");E.ResearchAward=1;E.Consequence=TEXT("获得 1 研究点。开放毒气支援，呼吸器需另行研发。中央阵位易手后，敌军将反击。");break;
    case 8:
        E.Title=G?TEXT("康布雷反击令"):TEXT("履带与时刻表");
        E.Body=G?TEXT("十一月三十日，德军反击康布雷突出部。野炮守住装甲接近线，缴获坦克仍在修理场。"):TEXT("康布雷的硬地承住坦克履带。炮兵按测算数据突然开火，步兵随装甲越过铁丝。");
        E.Date=TEXT("NOVEMBER 1917");E.ArtKey=TEXT("Dispatch");E.ResearchAward=1;E.Consequence=TEXT("获得 1 研究点。康布雷与布尔隆首轮野炮提前 28 秒，迫击炮提前 5 秒。");break;
    case 10:
        E.Title=G?TEXT("铁丝前的脚印"):TEXT("只带能背走的东西");
        E.Body=G?TEXT("前沿等待对面换防的消息。轻装巡逻沿低地接近前哨，随队只配烟幕支援。"):TEXT("冬夜巡逻名单送到连部。轻装分队沿低地出发，炮兵联络与装甲配属已从命令中划去。");
        E.Date=TEXT("12 FEBRUARY 1918");E.ArtKey=TEXT("Dispatch");E.ResearchAward=1;E.Consequence=TEXT("获得 1 研究点。本次仅配轻装部队与烟幕；装甲、野炮、迫击炮及机队不可用。");break;
    case 11:
        E.Title=G?TEXT("从东线来的列车"):TEXT("西去的传令兵");
        E.Body=G?TEXT("东线部队陆续运抵西线。突击训练与弹药分配令送到各连，携弹队开始向前集中。"):TEXT("米夏埃尔攻势迫使第五集团军后撤。法军靠近受威胁的接合部，补充兵带着新联络表抵达。");
        E.Date=TEXT("21 MARCH 1918");E.ArtKey=TEXT("Dispatch");E.ResearchAward=1;E.Consequence=TEXT("获得 1 研究点。米夏埃尔与利斯河行动开场士气增加 6。首轮野炮提前 20 秒，首轮烟幕后再整备缩短 12 秒。");break;
    case 14:
        E.Title=G?TEXT("黑日里的接替令"):TEXT("同一张炮兵表");
        E.Body=G?TEXT("八月八日，邻段枪位接连失联。缴获坦克与反坦克步枪接到急令，掩护后卫撤离通路。"):TEXT("亚眠火力表列明相邻部队的出发时刻。澳大利亚、加拿大、英国与法国部队在各自地段推进。");
        E.Date=TEXT("8 AUGUST 1918");E.ArtKey=TEXT("Anzac");E.ResearchAward=1;E.Consequence=TEXT("获得 1 研究点。亚眠与斯卡尔普行动开场士气增加 4。首轮野炮提前 22 秒，迫击炮提前 12 秒。");break;
    case 16:
        E.Title=G?TEXT("仍能通行的纤道"):TEXT("明渠边的接合部");
        E.Body=G?TEXT("运河坡岸被炮火击破，残存纤道仍可通行。烟弹与炮兵联络优先送到后卫路口。"):TEXT("北面的贝利库尔仍在交战。里克瓦尔明渠旁，工兵检查纤道，观察员沿岸寻找射界。");
        E.Date=TEXT("29 SEPTEMBER 1918");E.ArtKey=TEXT("Anzac");E.ResearchAward=2;E.Consequence=TEXT("获得 2 研究点。运河与塞勒河首轮野炮提前 12 秒、迫击炮提前 8 秒。首轮烟幕后再整备缩短 12 秒。");break;
    case 17:
        E.Title=G?TEXT("最后一班撤运"):TEXT("河谷里的铁轨");
        E.Body=G?TEXT("塞勒河车站仍有物资待装。列车炮组留守切口，步兵沿路堤保护两翼。"):TEXT("追击抵达塞勒河。列车火力封住切口，本连受命打通河谷，为后续部队开路。");
        E.Date=TEXT("17 OCTOBER 1918");E.ArtKey=TEXT("Railway");E.ResearchAward=0;E.Consequence=TEXT("本关有装甲列车参战。运河优先令继续生效，中央补给湾可缩短下一轮整备。");break;
    case 18:
        E.Title=G?TEXT("军车与手推车"):TEXT("窗户后的灯");
        E.Body=G?TEXT("军车与平民手推车挤在城外道路上。老兵带领补充兵守住路口，炮兵收紧射击区域。"):TEXT("瓦朗谢讷仍有居民，加拿大军正逼近城郊。炮兵标出禁射区，老兵领着补充兵沿浅沟前进。");
        E.Date=TEXT("1 NOVEMBER 1918");E.ArtKey=TEXT("Dispatch");E.ResearchAward=1;E.Consequence=TEXT("获得 1 研究点。最后两关禁用毒气与空中轰炸，开场士气增加 4。首轮野炮提前 12 秒，首轮烟幕后再整备缩短 12 秒。");break;
    case 19:
        E.Title=G?TEXT("折回口袋的电报"):TEXT("城墙外的清晨");
        E.Body=G?TEXT("南方停战的电报送到连部，西线仍在开火。军士沿桑布尔渡口清点等待撤离的分队。"):TEXT("新西兰部队逼近勒凯努瓦，桑布尔渡口仍传来枪声。携弹队沿城外道路送往下一处枪位。");
        E.Date=TEXT("4 NOVEMBER 1918");E.ArtKey=TEXT("Armistice");E.ResearchAward=0;E.Consequence=TEXT("城郊火力限制与老兵接替继续生效。夺取中央后，仍须击退反扑并肃清后沿。");break;
    default:E.Id=INDEX_NONE;break;
    }
    return E;
}
