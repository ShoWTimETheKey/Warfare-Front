// Named operations establish the historical setting. The playable company sector
// is an authored composite, not a survey reconstruction of an entire battle.
const TArray<FFrontlineCampaignMission>& AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction Faction)
{
    auto Build=[](bool German)
    {
        TArray<FFrontlineCampaignMission> Result;
        auto Add=[&](int32 Id,const TCHAR* Title,const TCHAR* Place,const TCHAR* Date,int32 Year,
            const TCHAR* Context,const TCHAR* Orders,const TCHAR* Terrain,const TCHAR* Map,
            FVector2D Position,float Rain,float Variation,float Wet,float Sight,float Mud)
        {
            FFrontlineCampaignMission M;
            M.Id=Id;M.Title=Title;M.Location=Place;M.DateLabel=Date;M.Year=Year;
            M.HistoricalContext=Context;M.Briefing=Orders;M.TerrainNotes=Terrain;M.MapKey=Map;M.MapPosition=Position;
            M.RainBase=Rain;M.RainVariation=Variation;M.Wetness=Wet;M.VisibilityScale=Sight;M.MudMoveScale=Mud;
            M.ObjectiveText=TEXT("突破敌方后沿，或击溃守军士气");
            M.EnemyRecruitScale=FMath::Max(.90f,1.13f-Id*.045f);M.EnemyAssaultDelay=FMath::Max(6.f,15.f-Id*1.6f);
            M.PlayerMorale=100;M.EnemyMorale=94+Id*4;
            M.PlayerArtilleryDelay=65-Id*6;M.EnemyArtilleryDelay=130-Id*10;
            M.PlayerSmokeDelay=Id?0:12;M.PlayerMortarDelay=Id?22:40;
            M.RandomSeed=23000+(German?100:0)+Id*17;
            M.EnemyRoleOrder={EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Rifle,EFrontlineRole::Mills,EFrontlineRole::Officer,EFrontlineRole::Marksman};
            Result.Add(M);
        };
        if(!German)
        {
            Add(0,TEXT("斯卡尔普河的晨雾"),TEXT("法国 · 阿拉斯东郊"),TEXT("9 APRIL 1917"),1917,
                TEXT("阿拉斯的炮声穿过雨雪，英军正向德军前沿推进。白垩高地遮住了远处枪位；洞穴、交通壕和仍在回答的电话，将守军的各个支点连在一起。"),
                TEXT("阿拉斯的炮声盖过了城里的钟声。沿白垩路推进，肃清高地后方守军，为后续部队打开通路。"),
                TEXT("白垩坡上的短交通壕；残雪与雨水弄湿了壕肩。"),TEXT("arras"),FVector2D(2.80,50.29),.12f,.10f,.66f,.95f,.94f);
            Add(1,TEXT("梅西讷的破晓"),TEXT("比利时 · 梅西讷山脊"),TEXT("7 JUNE 1917"),1917,
                TEXT("凌晨的地下爆破撕开梅西讷山脊。普卢默的第二集团军沿炮兵计划推进，澳大利亚与新西兰部队也在其中；新弹坑之间，幸存枪位仍在寻找彼此。"),
                TEXT("凌晨三时十分，地下爆破撕开梅西讷山脊。澳新部队正在邻段推进；机枪跟上坑岸，步兵打通破碎高地。"),
                TEXT("陡破坑岸与残存反斜面交错，山脊后的补给湾仍可利用。"),TEXT("messines"),FVector2D(2.895,50.765),.08f,.07f,.48f,1.f,.97f);
            Add(2,TEXT("雨中的皮尔克姆"),TEXT("比利时 · 伊普尔突出部"),TEXT("31 JULY 1917"),1917,
                TEXT("皮尔克姆方向的进攻已经展开。伊普尔周围的排水沟被炮火打断，雨水漫进田野和旧弹坑；送弹队与伤员只能挤向少数仍能通行的道路。"),
                TEXT("皮尔克姆的排水沟已被炮击切断，送弹队挤上残存木板路。壕炮先到，野炮仍陷在后方；逐个压住湿地中的枪位。"),
                TEXT("高积水、软泥和较宽的横向战场；铁丝把干燥落脚处分割开。"),TEXT("ypres"),FVector2D(2.90,50.89),.72f,.20f,.97f,.84f,.80f);
            Add(3,TEXT("康布雷的缺口"),TEXT("法国 · 康布雷西南"),TEXT("20 NOVEMBER 1917"),1917,
                TEXT("康布雷近郊的道路与铁路通向一座重要补给枢纽。坚实田地上布满履带印，兴登堡防线的铁丝和纵深壕横在其间；步兵与炮兵正争夺这些接近线。"),
                TEXT("康布雷的坚实地面承住了 Mark IV 的履带。坦克压过铁丝，步兵清理遗下的壕道，继续向后方交通线推进。"),
                TEXT("坚实田地、长射界与密集兴登堡线铁丝并存。"),TEXT("cambrai"),FVector2D(3.19,50.14),.04f,.04f,.40f,1.f,1.f);
            Add(4,TEXT("雾里的后卫"),TEXT("法国 · 圣康坦以西"),TEXT("21 MARCH 1918"),1918,
                TEXT("米夏埃尔攻势在浓雾与密集炮火中展开。英军第五集团军守着宽阔正面，德军突击队寻找支点之间的薄弱处；电话交换所与道路交叉口都在遭受轰击。"),
                TEXT("三月二十一日，米夏埃尔攻势冲破浓雾。第五集团军正在后撤，本连须接通被截断的道路，掩护失散部队归队。"),
                TEXT("浓雾削短观察距离，旧壕与补给湾仍能维持前后联络。"),TEXT("michael"),FVector2D(3.22,49.86),.13f,.09f,.62f,.78f,.91f);
            Add(5,TEXT("亚眠：雾后向前"),TEXT("法国 · 亚眠东部"),TEXT("8 AUGUST 1918"),1918,
                TEXT("亚眠清晨的雾里，澳大利亚与加拿大部队向东展开，英军在左侧、法军在南侧行动。炮兵、坦克和航空观察按同一张时刻表工作，德军前沿正设法接回失联枪位。"),
                TEXT("八月八日清晨，澳大利亚与加拿大部队在亚眠以东推进，法军攻击南侧。坦克随步兵出发，炮火切断守军增援。"),
                TEXT("较宽田野和残存村界适合装甲展开，薄雾仍会干扰观察。"),TEXT("amiens"),FVector2D(2.48,49.89),.06f,.05f,.53f,.90f,.96f);
        }
        else
        {
            Add(0,TEXT("阿拉斯：守住支点"),TEXT("法国 · 阿拉斯东郊"),TEXT("9 APRIL 1917"),1917,
                TEXT("阿拉斯的炮声穿过雨雪，英军正向德军前沿推进。白垩高地遮住了远处枪位；洞穴、交通壕和仍在回答的电话，将守军的各个支点连在一起。"),
                TEXT("阿拉斯前哨的电话只剩杂音。沿白垩路反击，接回失联的枪位，打通后方交通线。"),
                TEXT("白垩坡上的短交通壕；残雪与雨水弄湿了壕肩。"),TEXT("arras"),FVector2D(2.82,50.29),.12f,.10f,.66f,.95f,.94f);
            Add(1,TEXT("山脊失守之后"),TEXT("比利时 · 梅西讷东坡"),TEXT("7 JUNE 1917"),1917,
                TEXT("凌晨的地下爆破撕开梅西讷山脊。普卢默的第二集团军沿炮兵计划推进，澳大利亚与新西兰部队也在其中；新弹坑之间，幸存枪位仍在寻找彼此。"),
                TEXT("梅西讷的旧壕道埋在爆破后的碎土里。沿新弹坑收拢守军，夺回补给湾，接通仍在开火的支点。"),
                TEXT("陡破坑岸与残存反斜面交错，山脊后的补给湾仍可利用。"),TEXT("messines"),FVector2D(2.92,50.76),.08f,.07f,.48f,1.f,.97f);
            Add(2,TEXT("佛兰德的泥线"),TEXT("比利时 · 伊普尔东北"),TEXT("31 JULY 1917"),1917,
                TEXT("皮尔克姆方向的进攻已经展开。伊普尔周围的排水沟被炮火打断，雨水漫进田野和旧弹坑；送弹队与伤员只能挤向少数仍能通行的道路。"),
                TEXT("雨水漫过皮尔克姆的旧弹坑。携弹队先把壕炮弹背到前沿，机枪沿尚可通行的硬地换位。"),
                TEXT("高积水、软泥和较宽的横向战场；铁丝把干燥落脚处分割开。"),TEXT("ypres"),FVector2D(2.94,50.89),.72f,.20f,.97f,.84f,.80f);
            Add(3,TEXT("康布雷：反击令"),TEXT("法国 · 康布雷突出部"),TEXT("30 NOVEMBER 1917"),1917,
                TEXT("康布雷近郊的道路与铁路通向一座重要补给枢纽。坚实田地上布满履带印，兴登堡防线的铁丝和纵深壕横在其间；步兵与炮兵正争夺这些接近线。"),
                TEXT("十一月三十日，康布雷反击开始。缴获坦克尚在修理场；步枪与 MG 08/15 打开通路，野炮迎击英军装甲。"),
                TEXT("坚实田地、长射界与密集兴登堡线铁丝并存。"),TEXT("cambrai"),FVector2D(3.22,50.12),.04f,.04f,.40f,1.f,1.f);
            Add(4,TEXT("风暴迫近"),TEXT("法国 · 圣康坦战区"),TEXT("21 MARCH 1918"),1918,
                TEXT("米夏埃尔攻势在浓雾与密集炮火中展开。英军第五集团军守着宽阔正面，德军突击队寻找支点之间的薄弱处；电话交换所与道路交叉口都在遭受轰击。"),
                TEXT("米夏埃尔攻势的炮火向纵深延伸。突击队穿过雾中缺口，A7V 与机枪跟进；中央补给湾关系到后续攻势。"),
                TEXT("浓雾削短观察距离，旧壕与补给湾仍能维持前后联络。"),TEXT("michael"),FVector2D(3.26,49.86),.13f,.09f,.62f,.78f,.91f);
            Add(5,TEXT("亚眠的后沿"),TEXT("法国 · 亚眠东部"),TEXT("8 AUGUST 1918"),1918,
                TEXT("亚眠清晨的雾里，澳大利亚与加拿大部队向东展开，英军在左侧、法军在南侧行动。炮兵、坦克和航空观察按同一张时刻表工作，德军前沿正设法接回失联枪位。"),
                TEXT("亚眠的前沿接连失去联络。缴获坦克与 Tankgewehr 组抵达，机枪掩护后卫；为撤出的部队重新打开交通。"),
                TEXT("较宽田野和残存村界适合装甲展开，薄雾仍会干扰观察。"),TEXT("amiens"),FVector2D(2.50,49.89),.06f,.05f,.53f,.90f,.96f);
        }
        for(int32 I=0;I<Result.Num();++I)
        {
            auto& M=Result[I];
            M.Objective=EFrontlineCampaignObjective::Breakthrough;
            M.HoldCenterSeconds=0.f;
            M.ObjectiveText=TEXT("突破敌方后沿，或击溃守军士气");
            if(I>=2)M.EnemyOpeningRoles={EFrontlineRole::Lewis};
            M.bEnemyScopedMarksmen=German && I>=2;
            M.bEnemyStormtroopers=!German && M.Year>=1918;
            M.bEnemyCommissionedOfficers=I>=1;
            if(I==4)
            {
                M.EnemyRoleOrder=German?TArray<EFrontlineRole>{EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Marksman,EFrontlineRole::Officer}:
                    TArray<EFrontlineRole>{EFrontlineRole::Mills,EFrontlineRole::Rifle,EFrontlineRole::Mills,EFrontlineRole::Lewis,EFrontlineRole::Officer};
                M.PlayerArtilleryDelay=German?20:60;M.EnemyArtilleryDelay=German?100:65;
            }
        }
        FrontlineCampaignV32::ApplyMissionExtensions(Result,German);
        FrontlineGameplayV33::ApplyEarlyMissionPacing(Result,German);
        FrontlineCampaignV41::Expand(Result,German);
        return Result;
    };
    static const TArray<FFrontlineCampaignMission> British=Build(false),German=Build(true);
    return Faction==EFrontlineFaction::German?German:British;
}
