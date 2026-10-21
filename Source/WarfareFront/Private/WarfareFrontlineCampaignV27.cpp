#include "WarfareFrontlineCampaignV27.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFrontlineUnlocksV53.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlinePlayerGuideV43.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"

int32 AWarfareFrontlineGameMode::RoleUnlockMission(EFrontlineRole InRole,int32 Team) const
{
    if(uint8(InRole)>=uint8(EFrontlineRole::Count))return MAX_int32;
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUEquipmentV82::FirstMissionForRole(this,InRole,Team);
    // Local attachments follow the classic campaign's asymmetric introductions;
    // these are company assignments, never claims of a weapon's invention date.
    return FrontlineCampaignV41::RoleArrival(GetFactionForTeam(Team),InRole);
}

bool AWarfareFrontlineGameMode::IsRoleUnlocked(EFrontlineRole InRole,int32 Team) const
{
    if((Team!=0 && Team!=1) || uint8(InRole)>=uint8(EFrontlineRole::Count))return false;
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUEquipmentV82::RoleAvailable(this,InRole,Team);
    if(InRole==EFrontlineRole::Signals)return false;
    if(Team==0&&InRole==EFrontlineRole::MobileGun)
        return FrontlineGameplayV52::RecruitVariant(this,EFrontlineRole::Lewis)==EFrontlineRole::MobileGun;
    if(!FrontlineCampaignV41::RoleAllowed(GetFactionForTeam(Team),InRole))return false;
    if(FrontlineUltimateArchiveV8215::FullArmory(this,Team))return true;
    // The opposing army keeps each mission's authored order of battle.
    if(Team==1)return FrontlineGameplayV33::EnemyRoleAvailable(this,InRole);
    const auto* Run=GetCurrentCampaign();if(!Run)return true;
    return FrontlineUnlocksV53::Stage(this,Team)>=RoleUnlockMission(InRole,Team)||FrontlineCampaignV52::IsLoanRole(this,InRole);
}

FString AWarfareFrontlineGameMode::RoleUnlockReason(EFrontlineRole InRole,int32 Team) const
{
    if(IsRoleUnlocked(InRole,Team))return FString();
    const int32 Arrival=RoleUnlockMission(InRole,Team);const auto& Missions=GetCampaignMissions();
    return Missions.IsValidIndex(Arrival)?FString::Printf(TEXT("于「%s」配属。已完成的训练届时生效。"),Missions[Arrival].Title):TEXT("该编制尚未配属");
}

FString AWarfareFrontlineGameMode::TechnologyRoleBlockReason(int32 Id) const
{
    if(FrontlineUltimateArchiveV8215::FullArmory(this))return {};
    if(PlayerFaction==EFrontlineFaction::German&&(Id==12||Id==14||Id==15)&&GetEncounterYear()<1918)
        return TEXT("MP 18 于 1918 年行动配发，已完成的训练保留。");
    // Training can be prepared before an attachment arrives. Already paid bits
    // remain valid when replaying earlier operations; only the actual MP18 date remains.
    return FString();
}

FFrontlineMissionIntroduction AWarfareFrontlineGameMode::GetMissionIntroduction() const
{
    const auto* Mission=FrontlineCampaignV32::CurrentOperation(this);const int32 ActualStage=Mission?Mission->Id:0;const int32 Stage=FrontlineCampaignV41::LegacyStage(ActualStage);
    const bool German=PlayerFaction==EFrontlineFaction::German;
    FFrontlineMissionIntroduction I;
    if(Mission)
    {
        I.Heading=TEXT("本连配属命令");I.Title=Mission->Title;I.Context=Mission->Briefing;I.Tactic=Mission->AlliedContext;
        I.Role=EFrontlineRole::Rifle;
        for(const auto NewRole:FrontlinePlayerGuideV43::NewRoles(PlayerFaction,*Mission))
            {I.Role=NewRole;I.Title=GetRecruitName(I.Role);I.Context=FrontlinePlayerGuideV43::Unit(this,I.Role);}
        return I;
    }
    if(Stage>=6)
    {
        I.Heading=TEXT("行动简报");
        if(Stage==6)
        {
            I.Role=EFrontlineRole::Pioneer;I.Title=GetRecruitName(I.Role);
            I.Context=TEXT("工兵携剪钳与探针检查通路，清除铁丝和遗留弹药。作业时需要停步，近敌与压制会使工作中断。");
            I.Tactic=TEXT("步枪与机枪掩护工兵接近障碍。出发前在作战室选定本次配属。");
        }
        else if(Stage==7)
        {
            I.Role=EFrontlineRole::Flamethrower;I.Title=GetRecruitName(I.Role);
            I.Context=German?TEXT("Wex 以短促喷流打击近处掩体。射程与燃料有限，喷火手需要掩护才能接近。"):
                TEXT("法军 Schilt 分遣队配属本连。储罐中的燃料供给短促喷流，用于清理近处掩体。");
            I.Tactic=TEXT("烟幕与机枪掩护接近。喷流射程短，燃料耗尽后火力中断。");
        }
        else if(Stage==8)
        {
            I.Role=EFrontlineRole::Pioneer;I.Title=TEXT("铁路炮火");
            I.Context=TEXT("装甲列车沿本段支线提供火炮与机枪掩护，活动范围受轨道限制。步兵争夺铁路切口，反装甲火力可击毁车体。");
            I.Tactic=German?TEXT("步兵掩护列车侧翼，工兵清理接近线；烟幕可阻断敌方反坦克炮观察。"):
                TEXT("反装甲火力打击炮车，烟幕遮住铁路旁的空地。工兵与步枪分队交替接近切口。");
        }
        else
        {
            I.Role=EFrontlineRole::Officer;I.Title=TEXT("最后的通路");
            I.Context=TEXT("西线仍在交火，居民与部队共用村外通路。本次禁止毒气与空中轰炸，地面部队须清除残存障碍。");
            I.Tactic=TEXT("工兵开路，步枪与机枪交替掩护。守军仍可能在最后一道阵位反击。");
        }
        return I;
    }
    static const EFrontlineRole British[]={EFrontlineRole::Rifle,EFrontlineRole::Mills,EFrontlineRole::Lewis,EFrontlineRole::Marksman,EFrontlineRole::Officer,EFrontlineRole::Lewis};
    static const EFrontlineRole Deutsch[]={EFrontlineRole::Mills,EFrontlineRole::Lewis,EFrontlineRole::Officer,EFrontlineRole::Marksman,EFrontlineRole::Mills,EFrontlineRole::Lewis};
    I.Role=German?Deutsch[Stage]:British[Stage];
    const auto* Run=GetCurrentCampaign();const bool Replay=Run && Run->CompletedMissions>Stage;
    I.Heading=Stage==5 || Replay?TEXT("战前简报"):TEXT("本连配属命令");
    I.Title=GetRecruitName(I.Role);
    if(Stage==5)
    {
        I.Title=TEXT("诸兵协同");
        I.Context=German?TEXT("后沿电话时断时续。机枪守住出口，步枪接替空壕，突击队准备近距离反击。"):
            TEXT("亚眠晨雾掩护部队展开。步枪占壕，刘易斯压制敌军，投弹班接近被压住的枪位。");
        I.Tactic=TEXT("分批推进，保留预备队。炮击结束前，弹着区内的友军同样危险。");
    }
    else if(German && Stage==4)
    {
        I.Title=HasStormtroopers()?TEXT("MP 18 风暴突击队"):TEXT("春季突击训练");
        I.Context=TEXT("突击队寻找强点之间的缺口，以手雷与自动火力夺取近壕。后续步兵跟进，守住打开的通路。");
        I.Tactic=HasStormtroopers()?TEXT("借雾接近，短点射压住壕口；换弹期间需要同伴掩护。"):
            TEXT("风暴队操练可在作战室研发。完成后，投弹班于 1918 年行动换装 MP 18。");
    }
    else if(I.Role==EFrontlineRole::Rifle)
    {
        I.Context=TEXT("步枪班负责占壕与接替防线。胸墙提供掩护，穿越空地时容易受到机枪与炮火打击。");
        I.Tactic=TEXT("点击壕旁的小队肖像即可出击。前壕满员时，部队继续寻找前方阵位。");
    }
    else if(I.Role==EFrontlineRole::Mills)
    {
        I.Context=German?TEXT("突击手携柄式手榴弹加入队列，擅长近距离清壕。烟幕与压制火力有助于接近，投掷仍有散布。"):
            TEXT("Mills 投弹班负责清理壕角与近处守军。手雷可越过低矮掩体，散布会危及附近友军。");
        I.Tactic=TEXT("机枪压住守军后，投弹班更易接近。友军须避开投掷区域。");
    }
    else if(I.Role==EFrontlineRole::Lewis)
    {
        I.Context=German?TEXT("MG 08/15 枪组抵达。机枪守住长射界，同伴掩护枪手；转移前需收枪，到位后重新架设。"):
            TEXT("刘易斯枪组抵达，短点射为步枪班提供掩护。枪手停稳架设后开火，同伴留在身旁警戒。");
        I.Tactic=TEXT("机枪据壕掩护，步枪分队推进。换弹与转移时火力中断，需要相邻部队接替。");
    }
    else if(I.Role==EFrontlineRole::Marksman)
    {
        I.Context=German?TEXT("精确射手优先寻找暴露的机枪手。瞄准需要时间与清楚视线，近敌和压制会打断射击。"):
            TEXT("精确射手负责压制敌方机枪手。完成狙击学校训练后，可换装射程更远的镜瞄 SMLE。");
        I.Tactic=TEXT("掩体保护射手，烟幕会遮住双方视线。近距离威胁需要普通步兵处理。");
    }
    else
    {
        I.Context=German?TEXT("士官联络相邻枪位，提高命中率并缓解压制。升级为军官后，指挥收益增强，行进中也能重整部队。"):
            TEXT("军士重整附近部队，提高命中率并缓解压制。战地委任后指挥范围扩大，完成观察研习还可校射炮火。");
        I.Tactic=TEXT("金星标记指挥者，银星标记受指挥分队。组长阵亡、受重压或离开范围后，指挥加成失效。");
    }
    return I;
}
