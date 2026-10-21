#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineCombatRecordV65.h"
#include "WarfareFrontlineArmourKillTallyV8220.h"
#include "WarfareFrontlineArmourValourV8221.h"
#include "WarfareFrontlineArmourKillTallyAccessV8220.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineGameplayFocusedV84412.h"
#include "WarfareFrontlineMovementFocusedV84413.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineDebugConsoleV37.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineGround.h"
#include "Kismet/GameplayStatics.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineNewUnlockV8215.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFRRUArmourV82.h"
#include "FRRU/Native/TacticalRuntime.h"
#include "HAL/PlatformProcess.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineSupportV28.h"
#include "EngineUtils.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

// This memento is used only inside the guarded, disposable account review.
// Changing a public campaign through menu APIs would reset the live review scene.
struct FFrontlineAchievementReviewV76
{
    static bool ArchiveRewardAndSaveV8215(AWarfareFrontlineGameMode* Battle)
    { Battle->AwardEncounterResearch(0);return Battle->SaveProgression(); }
    static void ArchiveResetV8215(AWarfareFrontlineGameMode* Battle)
    { Battle->ResetEncounter(); }
    static bool ArchiveBindV8215(AWarfareFrontlineGameMode* Battle,int32 Mission)
    {
        auto* Run=Battle->CampaignRuns.FindByPredicate([&](const FFrontlineCampaignProgress& R){return R.Id==Battle->ActiveCampaignId;});
        if(!Run||!Battle->IsProgressionVolatile()||WarfareFRRUCampaignV82::Active(Battle)
            ||!Battle->GetCampaignMissionsForFaction(Run->Faction).IsValidIndex(Mission))return false;
        // This disposable fixture needs a legal assignment before the real bind.
        Run->SelectedMission=Run->CompletedMissions=Mission;
        Battle->BindCampaignEncounter();
        const auto* Bound=Battle->GetEncounterCampaignMission();return Bound&&Bound->Id==Mission;
    }
    static bool NewUnlockFixtureV8215(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Mission)
    {
        FString Dir;if(!B||!B->IsProgressionVolatile()||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)
            ||FrontlineUltimateArchiveV8215::Active(B)||WarfareFRRUCampaignV82::Active(B)||!B->Units.IsEmpty()||B->Phase!=EFrontlinePhase::Briefing
            ||uint8(F)>1||!B->GetCampaignMissionsForFaction(F).IsValidIndex(Mission)||Mission>=19)return false;
        FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=F;R.Difficulty=EFrontlineDifficulty::Veteran;
        R.CompletedMissions=R.SelectedMission=Mission;R.Research.ResearchPoints=17;
        R.CreatedUtc=R.UpdatedUtc=FDateTime::UtcNow().ToIso8601();FrontlineCampaignV52::Initialize(R);
        R.DispatchResolvedMaskV52=(1u<<20)-1u;
        B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=F;B->bProgressionReadOnly=false;
        B->EncounterId=FGuid::NewGuid();B->BoundCampaignId.Invalidate();B->BoundCampaignEncounterId.Invalidate();
        B->BoundCampaignMission=B->BoundAttachmentStageV53=INDEX_NONE;B->bEncounterRewarded=false;B->LastResearchReward=0;
        B->ResearchBeforeOutcomeV828=INDEX_NONE;B->bCampaignMissionAdvanced=false;B->RewardedEncounterIds.Empty();
        B->CombatRecordV65[0]=FFrontlineCombatRecordV65();B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;
        B->BindCampaignEncounter();const auto* Bound=B->GetEncounterCampaignMission();return Bound&&Bound->Id==Mission;
    }
    static void NewUnlockVictoryV8215(AWarfareFrontlineGameMode* B)
    {B->Phase=EFrontlinePhase::Fighting;B->Elapsed=90;B->ShotsFired=8;FrontlineAchievementsV76::BeginEncounter(B);B->FinishBattle(0,TEXT("V82.15 isolated attachment review"));}
    static void FinishReviewBattleV8219(AWarfareFrontlineGameMode* B,int32 Winner)
    {B->FinishBattle(Winner,TEXT("Isolated restart settlement review"));}
    static void FinishInitialReviewBattleV8219(AWarfareFrontlineGameMode* B,int32 Winner)
    {B->FinishBattle(Winner,TEXT("独立结局保存重试：突破"));}
    struct FInitialProfileV8219{FString File,Status;bool Volatile=false,ReadOnly=false;};
    static FInitialProfileV8219 InitialProfileV8219(AWarfareFrontlineGameMode* B)
    {return {B->ProgressionFile,B->ProgressionSaveStatus,B->bProgressionVolatile,B->bProgressionReadOnly};}
    static void InitialProfileV8219(AWarfareFrontlineGameMode* B,const FInitialProfileV8219& P)
    {B->ProgressionFile=P.File;B->ProgressionSaveStatus=P.Status;B->bProgressionVolatile=P.Volatile;B->bProgressionReadOnly=P.ReadOnly;}
    static void NewUnlockChecksV8215(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        const auto Runs=B->CampaignRuns;const auto Receipts=B->RewardedEncounterIds;
        const auto Active=B->ActiveCampaignId,Encounter=B->EncounterId;const auto Faction=B->PlayerFaction;
        const auto Bound=B->BoundCampaignId,BoundEncounter=B->BoundCampaignEncounterId;
        const int32 Mission=B->BoundCampaignMission,Attachment=B->BoundAttachmentStageV53,Reward=B->LastResearchReward,Displayed=B->ResearchBeforeOutcomeV828,Shots=B->ShotsFired,Outro=B->BattleOutroWinner;
        const bool Rewarded=B->bEncounterRewarded,Advanced=B->bCampaignMissionAdvanced,ReadOnly=B->bProgressionReadOnly;
        const auto Phase=B->Phase;const float Elapsed=B->Elapsed,OutroElapsed=B->BattleOutroElapsed;const auto Record=B->CombatRecordV65[0];const FString Reason=B->EndReason,Last=B->LastMessage;
        ON_SCOPE_EXIT
        {B->CampaignRuns=Runs;B->RewardedEncounterIds=Receipts;B->ActiveCampaignId=Active;B->PlayerFaction=Faction;B->EncounterId=Encounter;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;B->LastResearchReward=Reward;B->ResearchBeforeOutcomeV828=Displayed;B->ShotsFired=Shots;B->BattleOutroWinner=Outro;B->bEncounterRewarded=Rewarded;B->bCampaignMissionAdvanced=Advanced;B->bProgressionReadOnly=ReadOnly;B->Phase=Phase;B->Elapsed=Elapsed;B->BattleOutroElapsed=OutroElapsed;B->CombatRecordV65[0]=Record;B->EndReason=Reason;B->LastMessage=Last;};
        for(int32 Nation=0;Nation<2;++Nation)
        {
            B->Phase=EFrontlinePhase::Briefing;const bool Seed=NewUnlockFixtureV8215(B,EFrontlineFaction(Nation),0);
            const FString Prefix=FString::Printf(TEXT("v8215_next_unlock_nation_%d_"),Nation);Check(Prefix+TEXT("real_operation_bound"),Seed);if(!Seed)continue;
            const bool Before=!B->IsRoleUnlocked(EFrontlineRole::Lewis)&&!B->IsSupportUnlocked(EFrontlineSupport::Smoke);
            const int32 Points=B->GetResearchPoints();NewUnlockVictoryV8215(B);const auto* R=B->GetCurrentCampaign();const auto& Cards=FrontlineNewUnlockV8215::Cards(B);
            bool Gun=false,Smoke=false;for(const auto& C:Cards){Gun|=C.Kind==0&&C.Index==int32(EFrontlineRole::Lewis);Smoke|=C.Kind==1&&C.Index==int32(EFrontlineSupport::Smoke);}
            Check(Prefix+TEXT("victory_cards_match_next_attachments"),R&&R->CompletedMissions==1&&R->SelectedMission==1&&Gun&&Smoke);
            Check(Prefix+TEXT("victory_retains_bound_old_operation_eligibility"),Before&&B->GetEncounterCampaignMission()&&B->GetEncounterCampaignMission()->Id==0&&!B->IsRoleUnlocked(EFrontlineRole::Lewis)&&!B->IsSupportUnlocked(EFrontlineSupport::Smoke));
            Check(Prefix+TEXT("outro_holds_notice"),B->IsBattleOutroActive()&&!FrontlineNewUnlockV8215::Pending(B));
            Check(Prefix+TEXT("single_reward_and_old_displayed_points"),B->GetResearchPoints()==Points+3&&B->GetLastResearchReward()==3&&B->GetDisplayedResearchPointsV828()==Points);
            const int32 Count=Cards.Num();B->AwardEncounterResearch(0);FrontlineNewUnlockV8215::Capture(B,0);
            Check(Prefix+TEXT("duplicate_settlement_does_not_repeat_reward_or_cards"),B->GetResearchPoints()==Points+3&&FrontlineNewUnlockV8215::Cards(B).Num()==Count&&B->RewardedEncounterIds.Num()==1);
            const auto CampaignRecordBefore=B->GetCurrentCampaign()->CombatRecordV65;const int32 RewardBeforeTail=B->GetLastResearchReward(),PointsBeforeTail=B->GetResearchPoints();
            B->CombatRecordV65[0].Total.Infantry+=7;B->CombatRecordV65[0].SupportSources[int32(EFrontlineSupport::Artillery)].Infantry+=7;B->CombatRecordV65[0].InfantryLost+=2;
            const bool TailSaved=B->FinalizeCampaignCombatRecordV8217();const auto CampaignRecordAfter=B->GetCurrentCampaign()->CombatRecordV65;const bool TailAgain=B->FinalizeCampaignCombatRecordV8217();
            Check(Prefix+TEXT("outro_campaign_kills_sources_losses_saved_once"),TailSaved&&TailAgain&&CampaignRecordAfter.Total.Infantry==CampaignRecordBefore.Total.Infantry+7&&CampaignRecordAfter.SupportSources[int32(EFrontlineSupport::Artillery)].Infantry==CampaignRecordBefore.SupportSources[int32(EFrontlineSupport::Artillery)].Infantry+7&&CampaignRecordAfter.InfantryLost==CampaignRecordBefore.InfantryLost+2&&B->GetCurrentCampaign()->CombatRecordV65.Total.Infantry==CampaignRecordAfter.Total.Infantry&&CampaignRecordAfter.BattlesRecorded==CampaignRecordBefore.BattlesRecorded&&B->GetResearchPoints()==PointsBeforeTail&&B->GetLastResearchReward()==RewardBeforeTail);
            B->BattleOutroElapsed=7.01f;Check(Prefix+TEXT("notice_waits_through_report_after_outro"),!FrontlineNewUnlockV8215::Pending(B));
            const bool Returned=B->ContinueCampaign();
            Check(Prefix+TEXT("notice_released_only_after_report_return_to_operations"),Returned&&FrontlineNewUnlockV8215::ReleaseAfterReportV8217(B)&&FrontlineNewUnlockV8215::Pending(B));
            FrontlineNewUnlockV8215::Dismiss(B);FrontlineNewUnlockV8215::Capture(B,0);Check(Prefix+TEXT("dismissed_notice_does_not_reopen"),!FrontlineNewUnlockV8215::Pending(B));
        }
    }
    AWarfareFrontlineGameMode* Battle;
    FFrontlineCampaignProgress* Run;
    FFrontlineCampaignProgress BeforeRun;
    EFrontlineFaction BeforeFaction;
    FGuid BeforeActive,BeforeBound,BeforeEncounter,BeforeHostEncounter;
    int32 BeforeMission;
    explicit FFrontlineAchievementReviewV76(AWarfareFrontlineGameMode* B)
        :Battle(B),Run(const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())),
         BeforeRun(Run?*Run:FFrontlineCampaignProgress()),BeforeFaction(B->PlayerFaction),
         BeforeActive(B->ActiveCampaignId),BeforeBound(B->BoundCampaignId),BeforeEncounter(B->BoundCampaignEncounterId),BeforeHostEncounter(B->EncounterId),BeforeMission(B->BoundCampaignMission){}
    ~FFrontlineAchievementReviewV76()
    {
        if(Run)*Run=BeforeRun;
        Battle->PlayerFaction=BeforeFaction;Battle->ActiveCampaignId=BeforeActive;Battle->BoundCampaignId=BeforeBound;
        Battle->BoundCampaignEncounterId=BeforeEncounter;Battle->EncounterId=BeforeHostEncounter;Battle->BoundCampaignMission=BeforeMission;
    }
    void Campaign(EFrontlineFaction Faction)
    {
        // Review scenarios can replace the campaign ID to model an independent
        // route. GetCurrentCampaign resolves the selected ID, not the bound ID.
        Battle->ActiveCampaignId=Run->Id;Battle->EncounterId=FGuid::NewGuid();
        Battle->PlayerFaction=Faction;Run->Faction=Faction;Run->Difficulty=EFrontlineDifficulty::Veteran;
        Run->SelectedMission=19;Run->CompletedMissions=0;Run->bAchievementEligibleV76=true;
        Run->Research.TechnologyMask=(1u<<9)|(1u<<6)|(1u<<12);
        Battle->BoundCampaignId=Run->Id;Battle->BoundCampaignEncounterId=Battle->GetEncounterId();Battle->BoundCampaignMission=19;
    }
};
#include "WarfareFrontlineCheatCampaignAccessV8222.inl"
namespace FrontlineAchievementsV76
{
namespace
{
const FDefinition Catalog[]={
{TEXT("wire_crossing"),TEXT("Across the Wire"),TEXT("越过铁丝网"),TEXT("Clear at least 3 obstacles and win by breakthrough."),TEXT("清除至少 3 处障碍，并以突破获胜。"),TEXT("At Cambrai, the wire was an obstacle long before a trench came into reach."),TEXT("在康布雷，尚未抵近战壕，铁丝网就已挡住去路。")},
{TEXT("last_redoubt"),TEXT("Backs to the Wall"),TEXT("背水之战"),TEXT("Win by breakthrough after morale falls to 10% or less."),TEXT("士气曾降至 10% 或以下，随后以突破获胜。"),TEXT("In April 1918, Haig ordered his exhausted army to hold with its backs to the wall."),TEXT("1918 年四月，黑格向疲惫的部队下令：背水而战，坚守到底。")},
{TEXT("quiet_orders"),TEXT("Whistle, Then Move"),TEXT("哨响之后"),TEXT("Have one commissioned officer rally 6 different squads and survive."),TEXT("同一名军官重整 6 支不同小队，并活到胜利。"),TEXT("A whistle had to carry farther than a voice amid the guns."),TEXT("炮声中，哨音比喊声传得更远。")},
{TEXT("gun_watch"),TEXT("One Gun Is Enough"),TEXT("一挺就够"),TEXT("Win on Memories of the Abyss or Returned to the Front with at least 12 enemy infantry killed by a single machine-gun section."),TEXT("在“深渊回忆”难度或“去而复返”难度下获胜，一支机枪小队击杀至少 12 名敌军步兵。"),TEXT("The gun depended on men carrying belts, water and spare parts through the barrage."),TEXT("机枪离不开冒着炮火运来弹带、冷却水和零件的同伴。")},
{TEXT("clear_passage"),TEXT("Clear Passage"),TEXT("一线生路"),TEXT("Clear at least 6 distinct obstacles in one battle."),TEXT("一场战斗中清除至少 6 处不同障碍。"),TEXT("Shellfire rarely cleared every strand of wire. The remaining gaps had to be cut by hand."),TEXT("炮火很少能摧毁全部铁丝。剩下的缺口，要靠工兵亲手剪开。")},
{TEXT("storm_passage"),TEXT("Through the Gaps"),TEXT("防线的缝隙"),TEXT("As Germany, stormtroopers kill at least 24 enemy infantry."),TEXT("指挥德军，由风暴突击队累计击杀至少 24 名敌军步兵。"),TEXT("In 1918, assault detachments moved through weak points before the reserves could close them."),TEXT("1918 年，突击分队赶在预备队封堵之前穿过防线薄弱处。")},
{TEXT("iron_answer"),TEXT("David, in Field Grey"),TEXT("原野灰中的大卫"),TEXT("On Memories of the Abyss or Returned to the Front, win a German battle after an anti-tank rifle destroys at least 1 enemy heavy tank."),TEXT("在“深渊回忆”或“去而复返”难度下，指挥德军获胜，并以反坦克步枪击毁至少 1 辆敌军重型坦克。"),TEXT("The Tankgewehr was a heavy rifle; facing a tank with it still meant facing a tank on foot."),TEXT("Tankgewehr 是一支沉重的步枪。拿着它迎击坦克，终究还是步兵面对装甲。")},
{TEXT("smoke_crossing"),TEXT("Through the Smoke"),TEXT("穿过烟幕"),TEXT("Have 3 squads each advance at least 12 metres within friendly smoke."),TEXT("3 支不同小队各在己方烟幕内推进至少 12 米。"),TEXT("Smoke concealed the approach; wire and shell holes still determined the way through."),TEXT("烟幕遮住接近路线，铁丝和弹坑仍决定队伍如何穿过。")},
{TEXT("wind_holds"),TEXT("The Wind Held"),TEXT("风向未改"),TEXT("Your gas kills at least 10 enemy infantry without killing friendly troops."),TEXT("己方毒气击杀至少 10 名敌军步兵，且未杀伤友军。"),TEXT("At Loos in 1915, an uncertain wind could carry the gas back towards the men who released it."),TEXT("1915 年的洛斯，风向一变，毒气便可能吹向施放它的部队。")},
{TEXT("steel_storm"),TEXT("A Storm of Steel"),TEXT("钢铁骤雨"),TEXT("One artillery, mortar or creeping-barrage mission kills at least 12 enemy infantry without friendly casualties."),TEXT("同一次炮兵、迫击炮或徐进弹幕任务击杀至少 12 名敌军步兵，且未造成友军伤亡。"),TEXT("At Arras in 1917, infantry advanced behind a barrage moving to a shared timetable."),TEXT("1917 年的阿拉斯，步兵跟随按统一时刻表移动的弹幕推进。")},
{TEXT("coal_blood"),TEXT("Tickets, Please"),TEXT("请出示车票"),TEXT("Dismount both sections from the same armoured train; finish with at least 75% hull integrity."),TEXT("从同一列装甲列车派出步枪队和工兵队，获胜时列车保有至少 75% 车体完整度。"),TEXT("A train could bring men to the line. Getting them home required keeping the track and the train intact."),TEXT("列车把士兵送到前线。要带他们回去，铁轨和列车都得保住。")},
{TEXT("long_homeward"),TEXT("The Long Road Home"),TEXT("漫长归途"),TEXT("Complete the British, German, French and Russian campaign routes without using cheat commands."),TEXT("完成英军、德军、法军和俄军四条战役路线，全程不使用作弊命令。"),TEXT("After the fighting, soldiers still waited for orders to return home."),TEXT("战斗结束后，士兵还要等待归乡的命令。")},
{TEXT("fr_mort_homme"),TEXT("Never Yield the Ridge"),TEXT("死丘不退"),TEXT("Win at Mort-Homme with at most 4 infantry losses."),TEXT("赢得死丘之战，步兵阵亡不超过 4 人。"),TEXT("The Mort-Homme heights west of the Meuse became a battlefield in March 1916. A ridge was held by the men who could still answer roll call."),TEXT("1916 年三月，默兹河西岸的死丘高地卷入战火。守住山脊，也要让士兵还能应答点名。"),EFRRUNationV81::French},
{TEXT("fr_vaux"),TEXT("The Last Telephone"),TEXT("最后的电话"),TEXT("Win at Vaux with your first signals team intact."),TEXT("赢得沃堡之战，最先部署的通信组全员生还。"),TEXT("At Vaux in June 1916, shellfire severed the telephone lines. Messengers and the pigeon Vaillant carried the fort's appeals through the siege."),TEXT("1916 年六月，炮火切断了沃堡的电话线。传令兵与信鸽“英勇号”继续把求援送出重围。"),EFRRUNationV81::French},
{TEXT("fr_douaumont"),TEXT("The Fort Comes Home"),TEXT("要塞归来"),TEXT("At Douaumont, one VB team kills at least 12 enemy infantry with rifle grenades."),TEXT("在杜奥蒙，同一 VB 组以枪榴弹击杀至少 12 名敌军步兵。"),TEXT("Douaumont was retaken in October 1916. A cleared approach and coordinated infantry fire made the return to a ruined fort possible."),TEXT("杜奥蒙于 1916 年十月被收复。打开接近路、协调步兵火力，才有机会重返残破的要塞。"),EFRRUNationV81::French},
{TEXT("fr_malmaison"),TEXT("By the Clock"),TEXT("分秒不差"),TEXT("At Malmaison, one fire mission kills at least 12 enemy infantry before the objective approach is complete."),TEXT("在马尔迈松，完成目标接近前，同一次炮火任务击杀至少 12 名敌军步兵。"),TEXT("The limited attack at Malmaison in October 1917 put preparation and coordinated fire ahead of a hurried advance."),TEXT("1917 年十月的马尔迈松有限进攻，把周密准备与协同炮火置于仓促推进之前。"),EFRRUNationV81::French},
{TEXT("fr_champagne"),TEXT("An Empty Front"),TEXT("空壕不空防"),TEXT("At Champagne, one Hotchkiss kills at least 18 enemy infantry; its original gunner survives."),TEXT("在香槟，同一挺霍奇基斯击杀至少 18 名敌军步兵，最初的枪手生还。"),TEXT("In July 1918, defence in depth in Champagne traded exposed ground for the chance to preserve men and meet the attack farther back."),TEXT("1918 年七月，香槟的纵深防御放弃暴露地带，以保全部队并在后方迎击。"),EFRRUNationV81::French},
{TEXT("fr_soissons"),TEXT("A Little Renault"),TEXT("雷诺虽小"),TEXT("At Soissons, one Renault FT kills at least 18 enemy infantry and retains at least half its hull."),TEXT("在苏瓦松，同一辆雷诺 FT 击杀至少 18 名敌军步兵，并保有至少一半车体完整度。"),TEXT("The Renault FT first fought near Soissons on 31 May 1918. Its compact turreted form carried a new kind of armoured support into the summer battles."),TEXT("雷诺 FT 于 1918 年五月三十一日在苏瓦松附近首次参战。小巧的旋转炮塔战车把新的装甲支援带入夏季战斗。"),EFRRUNationV81::French},
{TEXT("ru_galicia"),TEXT("A Crossing Prepared"),TEXT("渡口已备"),TEXT("At Galicia, clear the bridge approach and bring your first engineer section home intact."),TEXT("在加利西亚，打开桥口通路，最先部署的工兵组全员生还。"),TEXT("In August 1914, Russian armies advanced across Galicia's rivers. Engineers opened the crossings ahead of the columns."),TEXT("1914 年八月，俄军向加利西亚的河谷推进。工兵在纵队前方打通渡口。"),EFRRUNationV81::Russian},
{TEXT("ru_przemysl"),TEXT("A Patient Siege"),TEXT("围城有度"),TEXT("At Przemysl, one rifle section kills at least 16 enemy infantry."),TEXT("在普热梅希尔，同一步枪班击杀至少 16 名敌军步兵。"),TEXT("Przemysl fell after a long siege in March 1915. The men outside the fortress also paid for every day spent before its works."),TEXT("1915 年三月，普热梅希尔在长期围攻后陷落。堡垒外的士兵也为围城的每一天付出代价。"),EFRRUNationV81::Russian},
{TEXT("ru_gorlice"),TEXT("Bring the Rearguard Home"),TEXT("后卫归来"),TEXT("Win the Gorlice rearguard action with at most 4 infantry losses."),TEXT("赢得戈尔利采后卫战，步兵阵亡不超过 4 人。"),TEXT("The Gorlice-Tarnow breakthrough in 1915 forced a Russian retreat. The rearguard covered the army's retreat."),TEXT("1915 年的戈尔利采—塔尔努夫突破迫使俄军后撤。后卫掩护主力退却，在追击中维持队形。"),EFRRUNationV81::Russian},
{TEXT("ru_lutsk"),TEXT("The Work Before Dawn"),TEXT("黎明前的准备"),TEXT("At Lutsk, one assault section kills at least 16 enemy infantry."),TEXT("在卢茨克，同一突击组击杀至少 16 名敌军步兵。"),TEXT("The Brusilov Offensive opened in June 1916 with attacks at several points. Preparations at each approach helped the infantry enter the position."),TEXT("布鲁西洛夫攻势于 1916 年六月在多处同时展开。各处接近路上的准备，为步兵打开阵地入口。"),EFRRUNationV81::Russian},
{TEXT("ru_kovel"),TEXT("Rails Run Red"),TEXT("血染铁路线"),TEXT("At Kovel, one Maxim kills at least 18 enemy infantry; its original gunner survives."),TEXT("在科韦利，同一挺马克沁击杀至少 18 名敌军步兵，最初的枪手生还。"),TEXT("Kovel was a railway objective in the 1916 offensive. Its approaches turned the promise of a breakthrough into a costly struggle."),TEXT("科韦利是 1916 年攻势中的铁路目标。通向它的接近路，把突破的希望变成代价沉重的争夺。"),EFRRUNationV81::Russian},
{TEXT("ru_riga"),TEXT("The Dvina Still Flows"),TEXT("德维纳河仍流淌"),TEXT("Win the Riga rearguard action with your first signals team intact."),TEXT("赢得里加后卫战，最先部署的通信组全员生还。"),TEXT("Riga was lost in September 1917. Preserving contact and covering the withdrawal could still save soldiers on the Dvina front."),TEXT("里加于 1917 年九月失守。维持联络、掩护撤收，仍能在德维纳河战线上挽救士兵。"),EFRRUNationV81::Russian},
{TEXT("cross_roll_call"),TEXT("Roll Call"),TEXT("无人缺席"),TEXT("As each of the 4 armies, win with at least 24 infantry returning and at least 75% of the infantry who entered the field alive."),TEXT("分别指挥四国军队获胜，每场至少 24 名步兵归队，入场步兵至少 75% 生还。"),TEXT("A roll call followed the fighting in every army. The names answered mattered as much as the ground gained."),TEXT("各国军队都要在战斗后点名。能应答的姓名，与争得的阵地同样重要。")},
{TEXT("cross_both_sides"),TEXT("Both Sides of the Wire"),TEXT("铁丝网两侧"),TEXT("As Britain and Germany, win by breakthrough after morale falls to 10% or less."),TEXT("分别指挥英军和德军，在士气曾降至 10% 或以下后以突破获胜。"),TEXT("British and German soldiers faced one another across the wire. In 2025, an unknown British soldier and an unknown German soldier were buried together near Ypres."),TEXT("英军与德军曾隔着铁丝网对峙。2025 年，两军各一名无名阵亡者被共同安葬在伊普尔附近。")},
{TEXT("cross_whole_section"),TEXT("Each Has a Part"),TEXT("各尽其职"),TEXT("As each of the 4 armies, kill at least 10 enemy infantry with each of 3 different infantry roles."),TEXT("分别指挥四国军队获胜，每场至少 3 种不同步兵兵种各击杀 10 名敌军步兵。"),TEXT("Rifles held the ground, machine-gun teams covered approaches and engineers opened the wire. No single section could do every job."),TEXT("步枪兵守住阵地，机枪组掩护接近路，工兵打开铁丝网。没有一种分队能独自完成所有任务。")},
{TEXT("cross_guns_and_men"),TEXT("Guns and Men"),TEXT("炮火与步兵"),TEXT("As each of the 4 armies, kill at least 20 enemy infantry with infantry and 20 with artillery, mortar or creeping barrage."),TEXT("分别指挥四国军队获胜，每场步兵击杀至少 20 名敌军步兵，炮兵、迫击炮或徐进弹幕合计另击杀至少 20 人。"),TEXT("Artillery could suppress a position, but infantry still had to enter and hold it. Coordination turned firepower into an advance."),TEXT("炮兵能压制阵地，步兵仍须进入并守住它。只有协同，火力才能化为推进。")},
{TEXT("hearts_homeward"),TEXT("Hearts of the Great War"),TEXT("勇敢的心"),TEXT("On Memories of the Abyss or Returned to the Front, win after one pioneer section clears at least 2 distinct obstacles."),TEXT("在“深渊回忆”或“去而复返”难度下获胜，同一工兵小队清除至少 2 处不同障碍。"),TEXT("Beyond the wire, a letter waits to be sent home. The men who open the passage must have a road back too."),TEXT("铁丝后面，是尚未寄出的家书。剪开通路的人，也要活着回去。")},
{TEXT("all_quiet_at_last"),TEXT("All Quiet on the Western Front"),TEXT("西线无战事"),TEXT("Win the final battle on Returned to the Front with survivors from at least 3 different infantry types."),TEXT("在“去而复返”难度下赢得最后一战，至少 3 种不同步兵兵种各有士兵从战场生还。"),TEXT("The report says: all quiet on the Western Front. At roll call, some names receive no answer."),TEXT("战报只写着：“西线无战事。”名册上被划去的那几行，再没有人应答。")}
};
static_assert(UE_ARRAY_COUNT(Catalog)==Count&&Count==30&&CrossFactionStartV83==24&&TributeStartV836==CrossFactionStartV83+CrossFactionCountV83&&HomewardIndexV82==11&&Count<32,"Thirty honours retain all twenty-eight existing indices in one uint32 mask.");
constexpr uint8 CrossRequiredNationsV83[]={15,3,15,15};
const TCHAR* CrossNationKeysV83[]={TEXT("britain"),TEXT("germany"),TEXT("france"),TEXT("russia")};
const TCHAR* CrossNationNamesENV83[]={TEXT("Britain"),TEXT("Germany"),TEXT("France"),TEXT("Russia")};
const TCHAR* CrossNationNamesZHV83[]={TEXT("英军"),TEXT("德军"),TEXT("法军"),TEXT("俄军")};
static_assert(UE_ARRAY_COUNT(CrossRequiredNationsV83)==CrossFactionCountV83,"Each new challenge has an explicit nation requirement.");
struct FAccount{uint32 EarnedMask=0,CheatHonoursV8218=0,ResetGenerationV83=0;uint8 Routes=0,CrossNationsV83[CrossFactionCountV83]={};TSet<FGuid> Tainted;TSet<FString> CheatMedalsV8218;TMap<FString,FString> Dates,CrossDatesV83;FrontlineMedalsV8215::FStats Medals[4];TSet<FGuid> MedalReceiptsV8215;TMap<FString,FrontlineMedalsV8215::FAward> MedalAwardsV8216,CheatMedalAwardsV8222;bool Loaded=false,Recovered=false,Corrupt=false,Unsupported=false;FString Notice;};
constexpr int32 AccountSchemaV82=9;
constexpr int32 MaximumArmourKillReceiptsV8220=20000;
constexpr uint32 LegacyAccountMaskV83=(1u<<CrossFactionStartV83)-1u;
FString CrossDateKeyV83(int32 Feat,int32 Nation){return FString(Catalog[CrossFactionStartV83+Feat].Id)+TEXT(".")+CrossNationKeysV83[Nation];}
uint8 RouteBitV82(EFRRUNationV81 Nation)
{
    switch(Nation)
    {
    case EFRRUNationV81::British:return 1;
    case EFRRUNationV81::German:return 2;
    case EFRRUNationV81::French:return 4;
    case EFRRUNationV81::Russian:return 8;
    default:return 0;
    }
}
FAccount Account;
// Frozen event/outcome proofs enter this queue. Retry never reads another battle's
// actors, difficulty, faction or combat counters to reconstruct an award.
struct FSettlementV81
{
    uint32 Mask=0,NotifyMask=0,CommittedNotifyMask=0;uint8 Routes=0;bool NotifyRoute=false;
    uint8 CrossNationsV83[CrossFactionCountV83]={};TMap<FString,FString> CrossDatesV83;
    TMap<FString,FString> Dates;FString RouteDate;
    FGuid CampaignV8215;TMap<FGuid,TPair<int32,FrontlineMedalsV8215::FStats>> MedalOutcomesV8215;
    TMap<FGuid,FrontlineMedalsV8215::FProof> MedalProofsV8216;
    bool NotifyMedalsV8217=false;TArray<FMedalNotificationV8217> CommittedMedalNoticesV8217;
};
TArray<FMedalNotificationV8217> MedalNotificationsV8217;
struct FArmourTallyLedgerV8220
{int32 Totals[4]={};uint32 ResetGenerationV83=0;TMap<FGuid,uint8> Receipts;bool Loaded=false,Recovered=false,Invalid=false;};
struct FArmourTallyPendingV8220
{FGuid Campaign;FrontlineArmourKillTallyV8220::FNotification Notice;bool PrimaryCommitted=false;};
FArmourTallyLedgerV8220 ArmourTallyLedgerV8220;
TMap<FGuid,FArmourTallyPendingV8220> PendingArmourTallyV8220;
double ArmourTallyRetryV8220=0;
TArray<FrontlineArmourKillTallyV8220::FNotification> ArmourKillNotificationsV8220;
TArray<int32> NotificationsV81;TMap<FGuid,FSettlementV81> SettlementsV81;double NextRetryV81=0;
TSet<FGuid> RetryTaintedV81;
bool ReviewFailSaveV81=false,ReviewFailLockV81=false,ReviewFailMirrorV81=false,ReviewFailTallyMirrorV83=false;
struct FTransientReviewV81
{
    TArray<FMedalNotificationV8217> MedalNotices=MedalNotificationsV8217;
    TArray<FrontlineArmourKillTallyV8220::FNotification> ArmourKillNotices=ArmourKillNotificationsV8220;
    FArmourTallyLedgerV8220 TallyLedger=ArmourTallyLedgerV8220;
    TMap<FGuid,FArmourTallyPendingV8220> TallyPending=PendingArmourTallyV8220;double TallyRetry=ArmourTallyRetryV8220;
    TArray<int32> Notices=NotificationsV81;TMap<FGuid,FSettlementV81> Settlements=SettlementsV81;
    TSet<FGuid> Tainted=RetryTaintedV81;double Next=NextRetryV81;bool Fail=ReviewFailSaveV81,Lock=ReviewFailLockV81,Mirror=ReviewFailMirrorV81,TallyMirror=ReviewFailTallyMirrorV83;
    FTransientReviewV81(){ArmourTallyLedgerV8220=FArmourTallyLedgerV8220();PendingArmourTallyV8220.Empty();ArmourTallyRetryV8220=0;MedalNotificationsV8217.Empty();ArmourKillNotificationsV8220.Empty();NotificationsV81.Empty();SettlementsV81.Empty();RetryTaintedV81.Empty();NextRetryV81=0;ReviewFailSaveV81=ReviewFailLockV81=ReviewFailMirrorV81=ReviewFailTallyMirrorV83=false;}
    ~FTransientReviewV81(){ArmourTallyLedgerV8220=TallyLedger;PendingArmourTallyV8220=TallyPending;ArmourTallyRetryV8220=TallyRetry;MedalNotificationsV8217=MedalNotices;ArmourKillNotificationsV8220=ArmourKillNotices;NotificationsV81=Notices;SettlementsV81=Settlements;RetryTaintedV81=Tainted;NextRetryV81=Next;ReviewFailSaveV81=Fail;ReviewFailLockV81=Lock;ReviewFailMirrorV81=Mirror;ReviewFailTallyMirrorV83=TallyMirror;}
};
struct FBattery{int32 Enemy=0,Friendly=0;bool CapturedAfter=false,BeforeNationalObjectiveV82=false;};
struct FNationalSectionV82
{int32 Squad=INDEX_NONE,Role=INDEX_NONE,Alive=0,Kills=0,SectionKillsV84=0,OriginalSizeV84=0;FString Weapon;bool OriginalGunnerAlive=false,FirstSectionV84=false;};
struct FNationalProofV82
{
    EFRRUNationV81 Nation=EFRRUNationV81::Invalid,StartedNation=EFRRUNationV81::Invalid;
    FString Mission,StartedMission;int32 Date=0,StartedDate=0,Difficulty=0,StartedDifficulty=0,Lost=0,Alive=0;
    bool CampaignMatches=false,Eligible=false,Saved=false,Won=false,Fallback=false,Cleared=false,TimelyBatteryV84=false,FT=false;
    TSet<FString> Stages;TArray<FNationalSectionV82> Sections;
};
struct FSmokeMove{TWeakObjectPtr<AWarfareFrontlineSoldier> Men[2];float Last[2]={0,0},High[2]={0,0},Travel=0;bool InSmoke=false;};
struct FOfficer{TWeakObjectPtr<AWarfareFrontlineSoldier> Actor;TSet<int32> Squads;float Relief=0;};
struct FRail{TWeakObjectPtr<AFrontlineTank> Actor;int32 Squads[2]={INDEX_NONE,INDEX_NONE};uint8 Sections=0;};
struct FEncounter
{
    FGuid Id,Campaign;bool Eligible=false,Started=false,Captured=false,AnyCapture=false,SettlementCaptured=false,LegacySavedV83=false;float MinimumMorale=1,TickAge=0;
    int32 CapturedInfantryKillsV8217=-1,CapturedArmourKillsV8217=-1;bool OutroKillsFinalizedV8217=false;
    FGuid PendingTailReceiptV8219;int32 PendingTailInfantryV8219=0,PendingTailArmourV8219=0;
    TOptional<FSettlementV81> DeferredOutcomeV8219;
    bool PendingInitialReceiptV8219=false;
    int32 Roles[int32(EFrontlineRole::Count)]={},GasEnemy=0,GasFriendly=0,StormKills=0,FriendlyFireV8216=0;bool AntiTankKill=false;
    TSet<FIntPoint> Obstacles;TMap<int32,FOfficer> Officers;TMap<int32,FSmokeMove> Smoke;TMap<int32,FBattery> Batteries;TMap<int32,FRail> Rails;TMap<int32,TWeakObjectPtr<AWarfareFrontlineSoldier>> OriginalGunners;
    EFRRUNationV81 NationV82=EFRRUNationV81::Invalid;FString MissionV82;int32 DateV82=0,DifficultyV82=0;bool FallbackV82=false;
    TSet<FString> NativeStagesV82,NativeClearancesV82;TMap<int32,TMap<FString,int32>> WeaponKillsV82;
    TMap<int32,int32> FirstSectionsV84,OriginalSectionSizesV84;
    TMap<int32,TSet<FIntPoint>> PioneerClearancesV837;
    // Stable identities retain the denominator after a casualty actor is freed.
    TSet<FIntPoint> EnteredInfantryV842;
    // Actual fatal-hull receipts only; section identity and victim serial survive
    // actor disposal. Light vehicles and armoured trains cannot stand in for tanks.
    TSet<FIntPoint> HeavyRifleVictimsV84415;
    // Fatal infantry receipts retain each gun section's deed after actor or
    // section cleanup. Replacement sections never pool their personal kills.
    TSet<int32> GunSectionsV851;
    TMap<int32,int32> GunSectionKillsV851;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FEncounter> Encounters;
TWeakObjectPtr<AWarfareFrontlineGameMode> IsolatedAwardBattle;
// The legacy account-retry fixture opts into silent automatic settlements.
// Its explicit NotifyMask fault/retry cases still exercise player notices.
TWeakObjectPtr<AWarfareFrontlineGameMode> SilentAwardReviewBattleV836;
bool DisposableAccount(FString* Out=nullptr)
{
    FString Dir;const bool Valid=FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty()&&!FPaths::IsRelative(Dir);
    if(Valid&&Out)*Out=Dir;return Valid;
}
FString AccountPath()
{
    FString Dir;
    if(FrontlineUltimateArchiveV8215::ReviewRequested())
        return FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)?FPaths::Combine(Dir,TEXT("AccountReview/achievements-v2.json")):FString();
    if(WarfareFRRUCampaignV82::PersistenceReviewRequestedV82())
    {
        if(WarfareFRRUCampaignV82::TryPersistenceReviewUserV82(Dir))
            return FPaths::Combine(Dir,TEXT("Saved/FRRU/Account/achievements-v2.json"));
        return FString();
    }
    // Four-army accounts start fresh; the earlier two-route account is retained.
    if(DisposableAccount(&Dir))return FPaths::Combine(Dir,TEXT("AccountReview/achievements-v2.json"));
    if(FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {
        // Native archive replays can earn honours. A private campaign review
        // must still resolve its account locally, never fall through to the
        // real cross-campaign profile merely because it is a replay.
        FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
        if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82Review"))
            &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!FPaths::IsRelative(Dir)&&!Dir.Contains(TEXT("..")))
        {
            Dir=FPaths::ConvertRelativePathToFull(Dir);FPaths::NormalizeDirectoryName(Dir);
            const FString Fixed=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation"));
            if(Project.Equals(Fixed,ESearchCase::IgnoreCase)&&Dir.StartsWith(Project+TEXT("/Saved/FRRU/"),ESearchCase::IgnoreCase))
                return FPaths::Combine(Dir,TEXT("AccountReview/achievements-v2.json"));
        }
        return FString();
    }
    return FPaths::Combine(FPlatformProcess::UserSettingsDir(),TEXT("WarfareFront/Account/achievements-v2.json"));
}
bool IsolatedAwardAccountV8217()
{
    FString Dir;
    // The new harness must pass the exact absolute allowlist and NoSave flags;
    // this does not turn ordinary volatile previews into writable accounts.
    return DisposableAccount()||FrontlineUltimateArchiveV8215::ReviewUserDir(Dir);
}
bool IsolatedAwardsAllowed(AWarfareFrontlineGameMode* B)
{return B&&IsolatedAwardBattle.Get()==B&&B->IsProgressionVolatile()&&IsolatedAwardAccountV8217();}
void TrackEnteredInfantryV842(const AWarfareFrontlineGameMode* B,FEncounter& S)
{
    if(!B)return;
    for(const AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->Team==0
        &&(U->bFieldEntered||U->IsActiveCombatant())&&U->SquadId!=INDEX_NONE)
        S.EnteredInfantryV842.Add(FIntPoint(U->SquadId,U->MemberIndex));
}
int32 ReturnedInfantryV842(const AWarfareFrontlineGameMode* B,const FEncounter& S)
{
    TSet<FIntPoint> Returned;if(!B)return 0;
    for(const AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->Team==0&&U->IsAlive()
        &&(U->bFieldEntered||U->IsActiveCombatant()))
    {
        const FIntPoint Id(U->SquadId,U->MemberIndex);
        if(S.EnteredInfantryV842.Contains(Id))Returned.Add(Id);
    }
    return Returned.Num();
}
bool PioneerWorkCompletedV842(const AWarfareFrontlineGameMode* B,const FEncounter& S,int32* Qualified=nullptr)
{
    if(Qualified)*Qualified=INDEX_NONE;
    if(!B)return false;
    for(const auto& Work:S.PioneerClearancesV837)
    {
        // Clearance receipts carry the working section's identity at completion.
        // Its later losses must not erase the passage it actually opened.
        if(S.OriginalSectionSizesV84.FindRef(Work.Key)>=2&&Work.Value.Num()>=2)
        {if(Qualified)*Qualified=Work.Key;return true;}
    }
    return false;
}
struct FQuietReturnV8311
{int32 Squad=INDEX_NONE,Original=0,Returned=0,ReturnedShots=0,Kills=0,ClassesV844=0;uint32 RoleMaskV844=0;};
FQuietReturnV8311 QuietReturnV8311(const AWarfareFrontlineGameMode* B,const FEncounter& S)
{
 FQuietReturnV8311 Result;if(!B)return Result;
 for(const auto& Q:B->Squads)if(Q.Team==0)
 {
  const int32 Original=S.OriginalSectionSizesV84.FindRef(Q.Id);
  if(Original<1||Q.Members.Num()!=Original)continue;
  const int32 Role=int32(Q.Role);if(Role<0||Role>=int32(EFrontlineRole::Count)||Role>=32)continue;
  int32 Returned=0,Shots=0;
  for(const auto& U:Q.Members)if(U.IsValid()&&U->Team==0&&U->IsAlive()
      &&(U->bFieldEntered||U->IsActiveCombatant()))
  {++Returned;Shots+=U->ShotsDischarged;}
  // Classify the section, not each crewman's weapon: a machine gun's rifle
  // assistants and research variants do not manufacture another troop type.
  if(Returned>=1)
  {
   Result.RoleMaskV844|=1u<<Role;
   if(Result.Squad==INDEX_NONE){Result.Squad=Q.Id;Result.Original=Original;Result.Returned=Returned;Result.ReturnedShots=Shots;Result.Kills=Q.KillsV65.Total();}
  }
 }
 for(int32 Role=0;Role<int32(EFrontlineRole::Count)&&Role<32;++Role)Result.ClassesV844+=(Result.RoleMaskV844&(1u<<Role))!=0;
 if(Result.ClassesV844<RequiredReturnClassesV844)Result.Squad=INDEX_NONE;
 return Result;
}
bool AllQuietCriterionV8310(const AWarfareFrontlineGameMode* B,const FEncounter& S)
{
    if(!B||B->Phase!=EFrontlinePhase::Victory||B->GetCampaignDifficulty()!=EFrontlineDifficulty::Survivor)return false;
    const auto* Mission=B->GetEncounterCampaignMission();const auto& Missions=B->GetCampaignMissions();
    if(!Mission||Missions.IsEmpty()||Mission->Id!=Missions.Last().Id)return false;
    if(WarfareFRRUCampaignV82::Active(B)
        &&(WarfareFRRUCampaignV82::Nation(B)!=EFRRUNationV81::French||S.NationV82!=EFRRUNationV81::French
            ||!S.Started||S.Id!=B->GetEncounterId()||S.Campaign!=WarfareFRRUCampaignV82::CampaignId(B)
            ||S.MissionV82!=WarfareFRRUCampaignV82::MissionId(B)||S.MissionV82!=Mission->MapKey
            ||S.DateV82!=Mission->CalendarDateV41||S.DifficultyV82!=int32(EFrontlineDifficulty::Survivor)))return false;
    return QuietReturnV8311(B,S).Squad!=INDEX_NONE;
}
struct FAccountLock
{
#if PLATFORM_WINDOWS
    HANDLE Mutex=nullptr;bool Owned=false;
    FAccountLock(){if(ReviewFailLockV81&&IsolatedAwardAccountV8217()&&IsolatedAwardBattle.IsValid())return;const FString Name=FString::Printf(TEXT("Local\\WarfareFrontAchievementsV2_%08x"),GetTypeHash(AccountPath().ToLower()));Mutex=CreateMutexW(nullptr,false,*Name);if(Mutex){const DWORD Wait=WaitForSingleObject(Mutex,3000);Owned=Wait==WAIT_OBJECT_0||Wait==WAIT_ABANDONED;}}
    ~FAccountLock(){if(Owned)ReleaseMutex(Mutex);if(Mutex)CloseHandle(Mutex);}
    bool Valid()const{return Owned;}
#else
    bool Valid()const{return true;}
#endif
};
bool Decode(const FString& Text,FAccount& Out,bool* Unsupported=nullptr)
{
    if(Unsupported)*Unsupported=false;
    TSharedPtr<FJsonObject> J;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J)||!J.IsValid())return false;
    double Schema=0,Mask=0,Routes=0;
    if(!J->TryGetNumberField(TEXT("schema"),Schema))return false;
    if(Schema!=2&&Schema!=3&&Schema!=4&&Schema!=5&&Schema!=6&&Schema!=7&&Schema!=8&&Schema!=AccountSchemaV82){if(Unsupported)*Unsupported=true;return false;}
    Out.ResetGenerationV83=0;
    if(Schema>=7)
    {
        double Generation=0;if(!J->TryGetNumberField(TEXT("reset_generation_v83"),Generation)||!FMath::IsFinite(Generation)
            ||Generation<0||Generation>4294967295.||FMath::FloorToDouble(Generation)!=Generation)return false;
        Out.ResetGenerationV83=uint32(Generation);
    }
    if(!J->TryGetNumberField(TEXT("earned"),Mask)
       ||!FMath::IsFinite(Mask)||Mask<0||Mask>(Schema==2?LegacyAccountMaskV83:Schema<8?(1u<<TributeStartV836)-1u:AllHonoursMaskV82)||FMath::FloorToDouble(Mask)!=Mask||!J->TryGetNumberField(TEXT("routes"),Routes)
       ||Routes<0||Routes>AllCampaignRoutesV82||FMath::FloorToDouble(Routes)!=Routes)return false;
    if((uint32(Mask)&(1u<<HomewardIndexV82))&&uint8(Routes)!=AllCampaignRoutesV82)return false;
    const TArray<TSharedPtr<FJsonValue>>* Tainted=nullptr;if(!J->TryGetArrayField(TEXT("tainted_campaigns"),Tainted)||!Tainted||Tainted->Num()>10000)return false;
    Out.EarnedMask=uint32(Mask);Out.Routes=uint8(Routes);
    for(const auto& V:*Tainted){FString S;FGuid Id;if(!V->TryGetString(S)||!FGuid::Parse(S,Id))return false;Out.Tainted.Add(Id);}
    const TSharedPtr<FJsonObject>* Dates=nullptr;if(!J->TryGetObjectField(TEXT("dates"),Dates)||!Dates||!Dates->IsValid())return false;
    for(const auto& P:(*Dates)->Values){FString S;if(!P.Value->TryGetString(S)||S.Len()>40)return false;Out.Dates.Add(FString(P.Key.ToView()),S);}
    FMemory::Memzero(Out.CrossNationsV83,sizeof(Out.CrossNationsV83));Out.CrossDatesV83.Empty();
    if(Schema>=3)
    {
        const TSharedPtr<FJsonObject>* Feats=nullptr;if(!J->TryGetObjectField(TEXT("cross_feats"),Feats)||!Feats||!Feats->IsValid()||(*Feats)->Values.Num()!=CrossFactionCountV83)return false;
        for(int32 Feat=0;Feat<CrossFactionCountV83;++Feat)
        {
            const TSharedPtr<FJsonObject>* Row=nullptr;const TSharedPtr<FJsonObject>* Nations=nullptr;double NationMask=0;
            if(!(*Feats)->TryGetObjectField(Catalog[CrossFactionStartV83+Feat].Id,Row)||!Row||!Row->IsValid()
                ||!(*Row)->TryGetNumberField(TEXT("mask"),NationMask)||!FMath::IsFinite(NationMask)||NationMask<0||NationMask>15||FMath::FloorToDouble(NationMask)!=NationMask
                ||(uint8(NationMask)&~CrossRequiredNationsV83[Feat])!=0||!(*Row)->TryGetObjectField(TEXT("dates"),Nations)||!Nations||!Nations->IsValid())return false;
            Out.CrossNationsV83[Feat]=uint8(NationMask);int32 Recorded=0;
            for(int32 Nation=0;Nation<4;++Nation)if(Out.CrossNationsV83[Feat]&(1u<<Nation))
            {
                FString Date;FDateTime Parsed;if(!(*Nations)->TryGetStringField(CrossNationKeysV83[Nation],Date)||Date.Len()>40||!FDateTime::ParseIso8601(*Date,Parsed))return false;
                Out.CrossDatesV83.Add(CrossDateKeyV83(Feat,Nation),Date);++Recorded;
            }
            if((*Nations)->Values.Num()!=Recorded)return false;
            if((Out.EarnedMask&(1u<<(CrossFactionStartV83+Feat)))&&Out.CrossNationsV83[Feat]!=CrossRequiredNationsV83[Feat])return false;
        }
    }
    if(Schema>=4)
    {
        const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;const TArray<TSharedPtr<FJsonValue>>* Receipts=nullptr;
        if(!J->TryGetArrayField(TEXT("medal_stats_v8215"),Rows)||!Rows||Rows->Num()!=(Schema>=9?4:2)||!J->TryGetArrayField(TEXT("medal_receipts_v8215"),Receipts)||!Receipts||Receipts->Num()>10000)return false;
        for(int32 N=0;N<Rows->Num();++N)
        {
            const auto Row=(*Rows)[N]->AsObject();if(!Row.IsValid())return false;auto& S=Out.Medals[N];
            auto Read=[&](const TCHAR* Key,int32& V){double D=0;if(!Row->TryGetNumberField(Key,D)||!FMath::IsFinite(D)||D<0||D>1000000000||FMath::FloorToDouble(D)!=D)return false;V=int32(D);return true;};int32 Route=0;
            if(!Read(TEXT("infantry"),S.Infantry)||!Read(TEXT("armour"),S.Armour)||!Read(TEXT("victories"),S.Victories)||!Read(TEXT("clean_victories"),S.CleanVictories)||!Read(TEXT("natural_route"),Route)||Route>3||S.CleanVictories>S.Victories)return false;S.NaturalRoute=uint8(Route);
        }
        for(const auto& V:*Receipts){FString ReceiptText;FGuid ReceiptId;if(!V->TryGetString(ReceiptText)||!FGuid::Parse(ReceiptText,ReceiptId)||!ReceiptId.IsValid()||Out.MedalReceiptsV8215.Contains(ReceiptId))return false;Out.MedalReceiptsV8215.Add(ReceiptId);}
    }
    else {Out.Medals[0].NaturalRoute=(Out.Routes&1)!=0;Out.Medals[1].NaturalRoute=(Out.Routes&2)!=0;}
    auto ReadAwards=[&](const TCHAR* Field,TMap<FString,FrontlineMedalsV8215::FAward>& Target)
    {
        const TSharedPtr<FJsonObject>* Awards=nullptr;if(!J->TryGetObjectField(Field,Awards)||!Awards||!Awards->IsValid()||(*Awards)->Values.Num()>FrontlineMedalsV8215::Count)return false;
        for(const auto& Entry:(*Awards)->Values)
        {
            const FString Id(Entry.Key.ToView());const int32 Index=FrontlineMedalsV8215::IndexForId(Id);const auto Row=Entry.Value->AsObject();if(Index==INDEX_NONE||!Row.IsValid())return false;
            double Grade=0,Repeats=0,Attachments=0;FrontlineMedalsV8215::FAward A;
            auto Integer=[](double V,double Max){return FMath::IsFinite(V)&&V>=0&&V<=Max&&FMath::FloorToDouble(V)==V;};
            if(!Row->TryGetNumberField(TEXT("grade"),Grade)||!Integer(Grade,255)||!Row->TryGetNumberField(TEXT("repeats"),Repeats)||!Integer(Repeats,255)||!Row->TryGetNumberField(TEXT("attachments"),Attachments)||!Integer(Attachments,65535)||!Row->TryGetStringField(TEXT("date"),A.Date))return false;
            FString Campaign;if(!Row->TryGetStringField(TEXT("campaign"),Campaign)||!FGuid::Parse(Campaign,A.Campaign))return false;
            const TArray<TSharedPtr<FJsonValue>>* Deeds=nullptr;if(!Row->TryGetArrayField(TEXT("campaign_deeds"),Deeds)||!Deeds||Deeds->Num()>3)return false;
            for(const auto& Deed:*Deeds){FString Encoded;FGuid DeedId;if(!Deed->TryGetString(Encoded)||!FGuid::Parse(Encoded,DeedId)||!DeedId.IsValid()||A.CampaignDeeds.Contains(DeedId))return false;A.CampaignDeeds.Add(DeedId);}
            A.Grade=uint8(Grade);A.Repeats=uint8(Repeats);A.Attachments=uint16(Attachments);if(!FrontlineMedalsV8215::ValidAward(Index,A))return false;Target.Add(Id,MoveTemp(A));
        }
        return true;
    };
    if(Schema>=5&&!ReadAwards(TEXT("medal_awards_v8216"),Out.MedalAwardsV8216))return false;
    if(Schema>=6&&!ReadAwards(TEXT("cheat_medal_awards_v8222"),Out.CheatMedalAwardsV8222))return false;
    if(J->HasField(TEXT("cheat_honours_v8218")))
    {
        double Cheat=0;if(!J->TryGetNumberField(TEXT("cheat_honours_v8218"),Cheat)||!FMath::IsFinite(Cheat)||Cheat<0||Cheat>(Schema<8?(1u<<TributeStartV836)-1u:AllHonoursMaskV82)||FMath::FloorToDouble(Cheat)!=Cheat)return false;Out.CheatHonoursV8218=uint32(Cheat);
    }
    if(J->HasField(TEXT("cheat_medals_v8218")))
    {
        const TArray<TSharedPtr<FJsonValue>>* Ids=nullptr;if(!J->TryGetArrayField(TEXT("cheat_medals_v8218"),Ids)||!Ids||Ids->Num()>FrontlineMedalsV8215::Count)return false;
        for(const auto& V:*Ids){FString Id;if(!V->TryGetString(Id)||Out.CheatMedalsV8218.Contains(Id)||!(Schema>=6?Out.CheatMedalAwardsV8222.Contains(Id):Out.MedalAwardsV8216.Contains(Id)))return false;Out.CheatMedalsV8218.Add(Id);}
    }
    if(Schema<6)for(const FString& Id:Out.CheatMedalsV8218)
    {
        // Earlier explicit cheat grants overwrote this entry. Retain its exact
        // presentation, but do not feed it to the natural award evaluator.
        Out.CheatMedalAwardsV8222.Add(Id,Out.MedalAwardsV8216.FindChecked(Id));Out.MedalAwardsV8216.Remove(Id);
    }
    if(Out.CheatMedalAwardsV8222.Num()!=Out.CheatMedalsV8218.Num())return false;
    // Schema 4 stored counters, not an actual award ledger. Preserve counters,
    // but do not reinterpret its virtual levels as genuine classes or bars.
    return true;
}
bool ReadFile(const FString& Path,FAccount& Out,bool* Unsupported=nullptr)
{if(Unsupported)*Unsupported=false;const int64 Size=IFileManager::Get().FileSize(*Path);FString Text;return Size>0&&Size<=2097152&&FFileHelper::LoadFileToString(Text,*Path)&&Decode(Text,Out,Unsupported);}
enum class EAccountRead { Empty,Valid,Invalid,Unsupported };
EAccountRead ReadAccount(const FString& Path,FAccount& Out,bool& Recovered)
{
    Recovered=false;Out=FAccount();FAccount Primary,Backup;bool PrimaryUnsupported=false,BackupUnsupported=false;
    if(Path.IsEmpty())return EAccountRead::Invalid;
    const bool PrimaryValid=ReadFile(Path,Primary,&PrimaryUnsupported),BackupValid=ReadFile(Path+TEXT(".bak"),Backup,&BackupUnsupported);
    // A supported backup cannot authorize downgrading a newer primary (or vice
    // versa). Malformed files still use the established supported-backup repair.
    if(PrimaryUnsupported||BackupUnsupported)return EAccountRead::Unsupported;
    if(PrimaryValid){Out=MoveTemp(Primary);return EAccountRead::Valid;}
    if(BackupValid){Out=MoveTemp(Backup);Recovered=true;return EAccountRead::Valid;}
    return IFileManager::Get().FileExists(*Path)||IFileManager::Get().FileExists(*(Path+TEXT(".bak")))?EAccountRead::Invalid:EAccountRead::Empty;
}
void UnsupportedNoticeV81()
{Account.Unsupported=true;Account.Notice=TEXT("Achievement records use an unsupported version. Existing files have been retained. Use a compatible game version.");}
void Load()
{
    if(Account.Loaded)return;Account=FAccount();Account.Loaded=true;
    FAccount Read;bool Recovered=false;const EAccountRead Result=ReadAccount(AccountPath(),Read,Recovered);
    if(Result==EAccountRead::Empty)return;
    if(Result==EAccountRead::Valid){Read.Loaded=true;Read.Recovered=Recovered;Account=MoveTemp(Read);return;}
    if(Result==EAccountRead::Unsupported){UnsupportedNoticeV81();return;}
    Account.Corrupt=true;Account.Notice=TEXT("Achievement records could not be read. Existing files have been retained.");
}
bool Replace(const FString& To,const FString& From)
{
#if PLATFORM_WINDOWS
    return MoveFileExW(*From,*To,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return IFileManager::Get().Move(*To,*From,true,true,false,true);
#endif
}
bool MergeDisk();
#include "WarfareFrontlineArmourTallyStoreV8220.inl"
void InvalidatePreResetHonoursV83()
{
    SettlementsV81.Empty();NotificationsV81.Empty();MedalNotificationsV8217.Empty();NextRetryV81=0;
    PendingArmourTallyV8220.Empty();ArmourKillNotificationsV8220.Empty();ArmourTallyRetryV8220=0;
    ArmourTallyLedgerV8220=FArmourTallyLedgerV8220();
    for(auto& Pair:Encounters){Pair.Value.Eligible=false;Pair.Value.DeferredOutcomeV8219.Reset();Pair.Value.PendingInitialReceiptV8219=false;Pair.Value.PendingTailReceiptV8219.Invalidate();}
}
bool MergeDisk()
{
    FAccount Disk;bool Recovered=false;const EAccountRead Result=ReadAccount(AccountPath(),Disk,Recovered);
    if(Result==EAccountRead::Unsupported){UnsupportedNoticeV81();return false;}
    if(Result!=EAccountRead::Valid)return Result==EAccountRead::Empty;
    if(Account.ResetGenerationV83!=Disk.ResetGenerationV83)
    {
        // A deliberate account reset invalidates cached bits and frozen event
        // proofs in other running instances. Campaign cheat taints still hold.
        for(const auto& Id:RetryTaintedV81)Disk.Tainted.Add(Id);
        Disk.Loaded=true;Disk.Recovered=Recovered;Account=MoveTemp(Disk);
        InvalidatePreResetHonoursV83();return true;
    }
    Account.Recovered|=Recovered;
    // Disk is the sole base for cumulative counters. Pending encounter deltas
    // are applied below only when their durable receipt is absent.
    for(int32 N=0;N<UE_ARRAY_COUNT(Account.Medals);++N)Account.Medals[N]=Disk.Medals[N];Account.MedalReceiptsV8215=Disk.MedalReceiptsV8215;Account.MedalAwardsV8216=Disk.MedalAwardsV8216;
    Account.EarnedMask|=Disk.EarnedMask;Account.Routes|=Disk.Routes;
    // Explicit grants are applied only after this merge. The durable overlay
    // is authoritative, so an older window cannot resurrect a revoked grant.
    Account.CheatHonoursV8218=Disk.CheatHonoursV8218;Account.CheatMedalsV8218=Disk.CheatMedalsV8218;
    Account.CheatMedalAwardsV8222=Disk.CheatMedalAwardsV8222;
    for(int32 Feat=0;Feat<CrossFactionCountV83;++Feat)Account.CrossNationsV83[Feat]|=Disk.CrossNationsV83[Feat];
    for(const auto& D:Disk.CrossDatesV83){FString* Current=Account.CrossDatesV83.Find(D.Key);if(!Current||D.Value<*Current)Account.CrossDatesV83.Add(D.Key,D.Value);}
    for(const auto& Id:Disk.Tainted)Account.Tainted.Add(Id);
    for(const auto& D:Disk.Dates){FString* Current=Account.Dates.Find(D.Key);if(!Current||D.Value<*Current)Account.Dates.Add(D.Key,D.Value);}
    return true;
}
// Caller holds the account mutex and has merged the latest supported disk data.
bool SaveLocked(bool* OutPrimaryCommitted=nullptr)
{
    if(OutPrimaryCommitted)*OutPrimaryCommitted=false;
    // Private deterministic fault, scoped to the disposable event fixture only.
    if(ReviewFailSaveV81&&IsolatedAwardAccountV8217()&&IsolatedAwardBattle.IsValid())return false;
    if(Account.Corrupt||Account.Unsupported)return false;const FString Path=AccountPath();if(Path.IsEmpty())return false;IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("schema"),AccountSchemaV82);J->SetNumberField(TEXT("reset_generation_v83"),Account.ResetGenerationV83);J->SetNumberField(TEXT("earned"),Account.EarnedMask);J->SetNumberField(TEXT("routes"),Account.Routes);
    J->SetNumberField(TEXT("cheat_honours_v8218"),Account.CheatHonoursV8218);TArray<FString> CheatIds=Account.CheatMedalsV8218.Array();CheatIds.Sort();TArray<TSharedPtr<FJsonValue>> CheatMedals;for(const auto& Id:CheatIds)CheatMedals.Add(MakeShared<FJsonValueString>(Id));J->SetArrayField(TEXT("cheat_medals_v8218"),CheatMedals);
    TArray<TSharedPtr<FJsonValue>> MedalRows,MedalReceipts;
    for(int32 N=0;N<UE_ARRAY_COUNT(Account.Medals);++N){const auto& S=Account.Medals[N];auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("infantry"),S.Infantry);Row->SetNumberField(TEXT("armour"),S.Armour);Row->SetNumberField(TEXT("victories"),S.Victories);Row->SetNumberField(TEXT("clean_victories"),S.CleanVictories);Row->SetNumberField(TEXT("natural_route"),S.NaturalRoute);MedalRows.Add(MakeShared<FJsonValueObject>(Row));}
    TArray<FString> MedalIds;for(const auto& Id:Account.MedalReceiptsV8215)MedalIds.Add(Id.ToString(EGuidFormats::Digits));MedalIds.Sort();for(const auto& Id:MedalIds)MedalReceipts.Add(MakeShared<FJsonValueString>(Id));J->SetArrayField(TEXT("medal_stats_v8215"),MedalRows);J->SetArrayField(TEXT("medal_receipts_v8215"),MedalReceipts);
    auto EncodeAwards=[](const TMap<FString,FrontlineMedalsV8215::FAward>& Awards)
    {
        auto Rows=MakeShared<FJsonObject>();for(const auto& Entry:Awards)
        {
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("grade"),Entry.Value.Grade);Row->SetNumberField(TEXT("repeats"),Entry.Value.Repeats);Row->SetNumberField(TEXT("attachments"),Entry.Value.Attachments);Row->SetStringField(TEXT("date"),Entry.Value.Date);Row->SetStringField(TEXT("campaign"),Entry.Value.Campaign.ToString(EGuidFormats::Digits));
            TArray<TSharedPtr<FJsonValue>> Deeds;for(const auto& Id:Entry.Value.CampaignDeeds)Deeds.Add(MakeShared<FJsonValueString>(Id.ToString(EGuidFormats::Digits)));Row->SetArrayField(TEXT("campaign_deeds"),Deeds);Rows->SetObjectField(Entry.Key,Row);
        }
        return Rows;
    };
    J->SetObjectField(TEXT("medal_awards_v8216"),EncodeAwards(Account.MedalAwardsV8216));
    J->SetObjectField(TEXT("cheat_medal_awards_v8222"),EncodeAwards(Account.CheatMedalAwardsV8222));
    auto Feats=MakeShared<FJsonObject>();
    for(int32 Feat=0;Feat<CrossFactionCountV83;++Feat)
    {
        auto Row=MakeShared<FJsonObject>(),Nations=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("mask"),Account.CrossNationsV83[Feat]);
        for(int32 Nation=0;Nation<4;++Nation)if(Account.CrossNationsV83[Feat]&(1u<<Nation))Nations->SetStringField(CrossNationKeysV83[Nation],Account.CrossDatesV83.FindRef(CrossDateKeyV83(Feat,Nation)));
        Row->SetObjectField(TEXT("dates"),Nations);Feats->SetObjectField(Catalog[CrossFactionStartV83+Feat].Id,Row);
    }
    J->SetObjectField(TEXT("cross_feats"),Feats);
    TArray<FString> IDs;for(const FGuid& Id:Account.Tainted)IDs.Add(Id.ToString());IDs.Sort();TArray<TSharedPtr<FJsonValue>> Tainted;
    for(const auto& Id:IDs)Tainted.Add(MakeShared<FJsonValueString>(Id));J->SetArrayField(TEXT("tainted_campaigns"),Tainted);
    auto Dates=MakeShared<FJsonObject>();for(const auto& D:Account.Dates)Dates->SetStringField(D.Key,D.Value);J->SetObjectField(TEXT("dates"),Dates);
    FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));const FString Temp=Path+TEXT(".tmp-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    if(!FFileHelper::SaveStringToFile(Text,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))return false;
    FAccount Verified;if(!ReadFile(Temp,Verified)){IFileManager::Get().Delete(*Temp);return false;}
    if(IFileManager::Get().FileExists(*Path)&&!Account.Recovered)
    {
        const FString BackupTemp=Path+TEXT(".bak.tmp");
        if(IFileManager::Get().Copy(*BackupTemp,*Path,true,true)!=COPY_OK||!Replace(Path+TEXT(".bak"),BackupTemp))return false;
    }
    if(!Replace(Path,Temp))return false;
    if(OutPrimaryCommitted)*OutPrimaryCommitted=true;
    if(ReviewFailMirrorV81&&IsolatedAwardAccountV8217()&&IsolatedAwardBattle.IsValid())return false;
    const FString Mirror=Path+TEXT(".bak.next");
    if(IFileManager::Get().Copy(*Mirror,*Path,true,true)!=COPY_OK||!Replace(Path+TEXT(".bak"),Mirror))return false;
    Account.Recovered=false;Account.Notice.Empty();return true;
}
bool Save()
{Load();if(Account.Corrupt||Account.Unsupported)return false;FAccountLock Lock;return Lock.Valid()&&MergeDisk()&&SaveLocked();}
FEncounter& State(AWarfareFrontlineGameMode* B)
{
    for(auto It=Encounters.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& S=Encounters.FindOrAdd(B);if(B&&S.Id!=B->GetEncounterId()){S=FEncounter();S.Id=B->GetEncounterId();}return S;
}
bool Active(AWarfareFrontlineGameMode* B)
{return B&&B->Phase==EFrontlinePhase::Fighting&&State(B).Started;}
void RecordNationalLossV82(FEncounter& S,int32 VictimTeam,const FFrontlineKillSourceV65& Source,bool Armour)
{
    if(Armour||VictimTeam!=1||Source.Team!=0||Source.SquadId==INDEX_NONE
       ||Source.NationalSourceNationV82!=int32(S.NationV82)||Source.NationalWeaponIdV82.IsEmpty())return;
    const auto* Role=wfexp::RoleFor(static_cast<wfexp::Faction>(uint8(S.NationV82)),Source.Role,S.DateV82);
    const auto* Weapon=wfexp::FindWeapon(TCHAR_TO_UTF8(*Source.NationalWeaponIdV82));
    if(!Role||!Role->available||!Weapon||Weapon->faction!=static_cast<wfexp::Faction>(uint8(S.NationV82))
       ||S.DateV82<Weapon->fromDate||S.DateV82>Weapon->toDate
       ||(Source.NationalWeaponIdV82!=UTF8_TO_TCHAR(Role->weaponId)&&Source.NationalWeaponIdV82!=UTF8_TO_TCHAR(Role->escortWeaponId)))return;
    ++S.WeaponKillsV82.FindOrAdd(Source.SquadId).FindOrAdd(Source.NationalWeaponIdV82);
}
bool RecordNativePassageV82(FEncounter& S,const FString& NodeId)
{
    const auto* Mission=wfexp::tactical::FindMission(TCHAR_TO_UTF8(*S.MissionV82));if(!Mission)return false;
    bool Registered=false;
    for(const auto& Stage:Mission->stages)if(Stage.kind==wfexp::tactical::GateKind::ClearPassage)
    {
        for(const auto& Node:Stage.nodes)Registered|=NodeId==UTF8_TO_TCHAR(Node.c_str());
        for(const auto& Node:Stage.alternateNodes)Registered|=NodeId==UTF8_TO_TCHAR(Node.c_str());
        Registered|=NodeId==UTF8_TO_TCHAR(Stage.fallbackNode.c_str());
    }
    if(!Registered||S.NativeClearancesV82.Contains(NodeId))return false;
    S.NativeClearancesV82.Add(NodeId);return true;
}
bool RecordNativeStageV82(FEncounter& S,const FString& StageId,bool Fallback)
{
    const auto* Mission=wfexp::tactical::FindMission(TCHAR_TO_UTF8(*S.MissionV82));if(!Mission)return false;
    bool Registered=false;for(const auto& Stage:Mission->stages)Registered|=StageId==UTF8_TO_TCHAR(Stage.id.c_str());
    if(!Registered||S.NativeStagesV82.Contains(StageId))return false;
    S.NativeStagesV82.Add(StageId);S.FallbackV82|=Fallback;
    if(S.MissionV82==TEXT("fr.malmaison")&&StageId==TEXT("fr.malmaison.stage.2"))
        for(auto& M:S.Batteries)if(M.Value.Enemy>=12)M.Value.BeforeNationalObjectiveV82=true;
    return true;
}
uint32 NationalMaskV82(const FNationalProofV82& P)
{
    if(!P.CampaignMatches||!P.Eligible||!P.Saved||!P.Won||P.Difficulty<1||P.Difficulty>2
       ||P.Difficulty!=P.StartedDifficulty||P.Nation!=P.StartedNation||P.Mission!=P.StartedMission||P.Date!=P.StartedDate)return 0;
    const auto* Mission=wfexp::tactical::FindMission(TCHAR_TO_UTF8(*P.Mission));
    if(!Mission||Mission->player!=static_cast<wfexp::Faction>(uint8(P.Nation))||Mission->date!=P.Date)return 0;
    const auto Stage=[&](int32 Index){return P.Stages.Contains(P.Mission+FString::Printf(TEXT(".stage.%d"),Index));};
    // Saved native victory already validates the whole objective. Individual
    // receipts below prove the work named by each honour; recovery stays legal.
    if(!Stage(3))return 0;
    const auto Section=[&](EFrontlineRole Role,const TCHAR* Weapon,int32 Kills,bool Original=false)
    {for(const auto& Q:P.Sections)if(Q.Role==int32(Role)&&Q.Weapon==Weapon&&Q.Kills>=Kills&&(!Original||Q.OriginalGunnerAlive))return true;return false;};
    const auto SectionKills=[&](EFrontlineRole Role,int32 Kills)
    {for(const auto& Q:P.Sections)if(Q.Role==int32(Role)&&Q.SectionKillsV84>=Kills)return true;return false;};
    const auto IntactFirst=[&](EFrontlineRole Role)
    {for(const auto& Q:P.Sections)if(Q.Role==int32(Role)&&Q.FirstSectionV84&&Q.OriginalSizeV84>0&&Q.Alive>=Q.OriginalSizeV84)return true;return false;};
    uint32 Mask=0;
    if(P.Nation==EFRRUNationV81::French)
    {
        if(P.Mission==TEXT("fr.mort_homme")&&P.Lost<=4)Mask|=1u<<12;
        if(P.Mission==TEXT("fr.vaux")&&IntactFirst(EFrontlineRole::Signals))Mask|=1u<<13;
        if(P.Mission==TEXT("fr.douaumont")&&Section(EFrontlineRole::RifleGrenadier,TEXT("fr.vb"),12))Mask|=1u<<14;
        if(P.Mission==TEXT("fr.malmaison")&&P.TimelyBatteryV84)Mask|=1u<<15;
        if(P.Mission==TEXT("fr.champagne_defence")&&Section(EFrontlineRole::Lewis,TEXT("fr.hotchkiss"),18,true))Mask|=1u<<16;
        if(P.Mission==TEXT("fr.soissons")&&P.Date>=19180531&&P.FT)Mask|=1u<<17;
    }
    else if(P.Nation==EFRRUNationV81::Russian)
    {
        if(P.Mission==TEXT("ru.galicia")&&P.Cleared&&IntactFirst(EFrontlineRole::Pioneer))Mask|=1u<<18;
        if(P.Mission==TEXT("ru.przemysl")&&SectionKills(EFrontlineRole::Rifle,16))Mask|=1u<<19;
        if(P.Mission==TEXT("ru.gorlice")&&P.Lost<=4)Mask|=1u<<20;
        if(P.Mission==TEXT("ru.lutsk")&&SectionKills(EFrontlineRole::Mills,16))Mask|=1u<<21;
        if(P.Mission==TEXT("ru.kovel")&&Section(EFrontlineRole::Lewis,TEXT("ru.maxim"),18,true))Mask|=1u<<22;
        if(P.Mission==TEXT("ru.riga")&&IntactFirst(EFrontlineRole::Signals))Mask|=1u<<23;
    }
    return Mask;
}
bool NationalSurvivorPresenceV82(int32 Team,bool Alive,bool Entered,bool Active)
{
    // Pending reinforcements are not survivors of this battle. A living soldier
    // who entered and later embarked still belongs to the surviving section.
    return Team==0&&Alive&&(Entered||Active);
}
bool NationalSurvivorPresenceV82(const AWarfareFrontlineSoldier* Soldier)
{return IsValid(Soldier)&&NationalSurvivorPresenceV82(Soldier->Team,Soldier->IsAlive(),Soldier->bFieldEntered,Soldier->IsActiveCombatant());}
struct FCrossBattleProofV83
{
    EFRRUNationV81 Nation=EFRRUNationV81::Invalid,StartedNation=EFRRUNationV81::Invalid;
    int32 Difficulty=0,StartedDifficulty=0,Date=0,StartedDate=0,InfantryKills=0,FireKills=0,Lost=0,Alive=0,EnteredInfantryV842=0;float MinimumMorale=1;
    int32 RoleKillsV84[int32(EFrontlineRole::Count)]={};
    bool CampaignMatches=false,Eligible=false,Saved=false,Won=false,Breakthrough=false;
};
uint8 CrossBattleFeatsV83(const FCrossBattleProofV83& P)
{
    if(!RouteBitV82(P.Nation)||P.Nation!=P.StartedNation||P.Date<=0||P.Date!=P.StartedDate||P.Difficulty!=P.StartedDifficulty
        ||P.Difficulty<1||P.Difficulty>2||!P.CampaignMatches||!P.Eligible||!P.Saved||!P.Won)return 0;
    int32 ProductiveRoles=0;for(const int32 Kills:P.RoleKillsV84)ProductiveRoles+=Kills>=10;
    uint8 Feats=0;
    if(FrontlineMedalsV8215::PreservedForceV842(P.EnteredInfantryV842,P.Alive))Feats|=1u<<0;
    if((RouteBitV82(P.Nation)&CrossRequiredNationsV83[1])&&P.Breakthrough&&P.MinimumMorale<=.10001f)Feats|=1u<<1;
    if(ProductiveRoles>=3)Feats|=1u<<2;
    if(P.InfantryKills>=20&&P.FireKills>=20)Feats|=1u<<3;
    return Feats;
}
FCrossBattleProofV83 CaptureCrossBattleProofV83(AWarfareFrontlineGameMode* B,const FEncounter& S,bool CampaignEligible,bool Breakthrough)
{
    FCrossBattleProofV83 P;const bool Expanded=WarfareFRRUCampaignV82::Active(B);
    P.Nation=WarfareFRRUCampaignV82::Nation(B,0);P.StartedNation=S.NationV82;
    P.Difficulty=int32(B->GetCampaignDifficulty());P.StartedDifficulty=S.DifficultyV82;
    P.Date=WarfareFRRUCampaignV82::CalendarDate(B);P.StartedDate=S.DateV82;
    P.CampaignMatches=S.Campaign.IsValid()&&CampaignEligible&&S.Id==B->GetEncounterId()&&B->GetEncounterCampaignMission();
    P.Eligible=S.Eligible&&CampaignEligible&&!Account.Tainted.Contains(S.Campaign);
    P.Saved=Expanded?WarfareFRRUWorldV82::OutcomePersisted(B):S.LegacySavedV83||FrontlineUltimateArchiveV8215::ReplayEligible(B);
    P.Won=B->Phase==EFrontlinePhase::Victory;P.Breakthrough=Breakthrough;P.MinimumMorale=S.MinimumMorale;
    P.Lost=B->CombatRecordV65[0].InfantryLost;
    for(int32 Role=0;Role<int32(EFrontlineRole::Count);++Role){P.RoleKillsV84[Role]=B->CombatRecordV65[0].InfantrySources[Role].Infantry;P.InfantryKills+=P.RoleKillsV84[Role];}
    for(const auto& Fire:S.Batteries)P.FireKills+=Fire.Value.Enemy;
    P.EnteredInfantryV842=S.EnteredInfantryV842.Num();P.Alive=ReturnedInfantryV842(B,S);
    return P;
}
void CaptureCrossSettlementV83(FSettlementV81& Pending,uint8 Feats,uint8 Nation,const FString& Date,bool Notify)
{
    for(int32 Feat=0;Feat<CrossFactionCountV83;++Feat)if((Feats&(1u<<Feat))&&(Nation&CrossRequiredNationsV83[Feat]))
    {
        const uint8 RequiredNation=Nation&CrossRequiredNationsV83[Feat];Pending.CrossNationsV83[Feat]|=RequiredNation;
        for(int32 N=0;N<4;++N)if(RequiredNation&(1u<<N))
        {const FString Key=CrossDateKeyV83(Feat,N);FString* Existing=Pending.CrossDatesV83.Find(Key);if(!Existing||Date<*Existing)Pending.CrossDatesV83.Add(Key,Date);}
        if(Notify)Pending.NotifyMask|=1u<<(CrossFactionStartV83+Feat);
    }
}
FNationalProofV82 CaptureNationalProofV82(AWarfareFrontlineGameMode* B,const FEncounter& S)
{
    FNationalProofV82 P;P.Nation=WarfareFRRUCampaignV82::Nation(B);P.StartedNation=S.NationV82;
    P.Mission=WarfareFRRUCampaignV82::MissionId(B);P.StartedMission=S.MissionV82;
    P.Date=WarfareFRRUCampaignV82::CalendarDate(B);P.StartedDate=S.DateV82;
    P.Difficulty=int32(WarfareFRRUCampaignV82::Difficulty(B));P.StartedDifficulty=S.DifficultyV82;
    P.CampaignMatches=S.Campaign.IsValid()&&WarfareFRRUCampaignV82::CampaignId(B)==S.Campaign;
    P.Eligible=S.Eligible&&WarfareFRRUCampaignV82::HonourEligible(B)&&!Account.Tainted.Contains(S.Campaign);
    P.Saved=WarfareFRRUWorldV82::OutcomePersisted(B);P.Won=B->Phase==EFrontlinePhase::Victory&&B->BattleOutroWinner==0;
    P.Stages=S.NativeStagesV82;P.Fallback=S.FallbackV82;P.Lost=B->CombatRecordV65[0].InfantryLost;
    for(const AWarfareFrontlineSoldier* Soldier:B->Units)P.Alive+=NationalSurvivorPresenceV82(Soldier);
    P.Cleared=P.Mission==TEXT("ru.galicia")?S.NativeClearancesV82.Contains(TEXT("bridge"))
        :P.Mission==TEXT("ru.kovel")?S.NativeClearancesV82.Contains(TEXT("rail")):!S.NativeClearancesV82.IsEmpty();
    for(const auto& M:S.Batteries)P.TimelyBatteryV84|=M.Value.Enemy>=12&&M.Value.BeforeNationalObjectiveV82;
    for(const auto& Q:B->Squads)if(Q.Team==0)
    {
        const auto* Role=wfexp::RoleFor(static_cast<wfexp::Faction>(uint8(P.Nation)),int32(Q.Role),P.Date);if(!Role||!Role->available)continue;
        FNationalSectionV82 Row;Row.Squad=Q.Id;Row.Role=int32(Q.Role);Row.Weapon=UTF8_TO_TCHAR(Role->weaponId);Row.SectionKillsV84=Q.KillsV65.Infantry;
        Row.OriginalSizeV84=S.OriginalSectionSizesV84.FindRef(Q.Id);if(const int32* First=S.FirstSectionsV84.Find(Row.Role))Row.FirstSectionV84=*First==Q.Id;
        for(const auto& Weak:Q.Members)Row.Alive+=NationalSurvivorPresenceV82(Weak.Get());
        if(const auto* Kills=S.WeaponKillsV82.Find(Q.Id))Row.Kills=Kills->FindRef(Row.Weapon);
        if(const auto* Gunner=S.OriginalGunners.Find(Q.Id))Row.OriginalGunnerAlive=Gunner->IsValid()&&Gunner->Get()->IsActiveCombatant();
        P.Sections.Add(MoveTemp(Row));
    }
    if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const auto& V:Fleet->GetVehicles())
        P.FT|=V&&V->Team==0&&WarfareFRRUArmourV82::Id(V)==TEXT("fr.ft")&&V->KillsV65.Infantry>=18&&V->GetHullIntegrity()>=V->GetMaximumHullIntegrity()*.5f;
    return P;
}
void Unlock(int32 I)
{const uint32 Bit=1u<<I;if(Account.EarnedMask&Bit)return;Account.EarnedMask|=Bit;Account.Dates.Add(Catalog[I].Id,FDateTime::UtcNow().ToIso8601());}
bool HasPendingV81(){return !SettlementsV81.IsEmpty()||!RetryTaintedV81.IsEmpty();}
void RetryNoticeV81(){if(!Account.Unsupported)Account.Notice=TEXT("Achievement records could not be saved. Retrying.");}
void MergeSettlementV8219(FSettlementV81& To,const FSettlementV81& From)
{
    To.Mask|=From.Mask;To.NotifyMask|=From.NotifyMask;To.CommittedNotifyMask|=From.CommittedNotifyMask;
    To.Routes|=From.Routes;To.NotifyRoute|=From.NotifyRoute;To.NotifyMedalsV8217|=From.NotifyMedalsV8217;
    if(!To.CampaignV8215.IsValid())To.CampaignV8215=From.CampaignV8215;
    if(From.RouteDate>To.RouteDate)To.RouteDate=From.RouteDate;
    for(int32 I=0;I<CrossFactionCountV83;++I)To.CrossNationsV83[I]|=From.CrossNationsV83[I];
    for(const auto& P:From.Dates){auto* Date=To.Dates.Find(P.Key);if(!Date||P.Value<*Date)To.Dates.Add(P.Key,P.Value);}
    for(const auto& P:From.CrossDatesV83){auto* Date=To.CrossDatesV83.Find(P.Key);if(!Date||P.Value<*Date)To.CrossDatesV83.Add(P.Key,P.Value);}
    for(const auto& P:From.MedalOutcomesV8215)if(!To.MedalOutcomesV8215.Contains(P.Key))To.MedalOutcomesV8215.Add(P.Key,P.Value);
    for(const auto& P:From.MedalProofsV8216)if(!To.MedalProofsV8216.Contains(P.Key))To.MedalProofsV8216.Add(P.Key,P.Value);
    To.CommittedMedalNoticesV8217.Append(From.CommittedMedalNoticesV8217);
}
void PersistSettlementsV81()
{
    if(!HasPendingV81())return;Load();if(Account.Unsupported)return;NextRetryV81=FPlatformTime::Seconds()+5.;
    FAccountLock Lock;
    if(Account.Corrupt||!Lock.Valid()||!MergeDisk()){RetryNoticeV81();return;}
    // External or locally pending cheat taints are authoritative before any
    // deferred route or award is applied. Earned records are never revoked.
    for(const auto& Id:RetryTaintedV81)Account.Tainted.Add(Id);
    for(auto It=SettlementsV81.CreateIterator();It;++It)if(Account.Tainted.Contains(It.Value().CampaignV8215.IsValid()?It.Value().CampaignV8215:It.Key()))It.RemoveCurrent();
    const auto Before=Account;TMap<FGuid,TArray<FMedalNotificationV8217>> NewMedalNotices;
    uint32 NotifyMask=0,CommittedNotifyMask=0;bool NotifyRoute=false;FString RouteDate;
    for(const auto& P:SettlementsV81)
    {
        for(const auto& Outcome:P.Value.MedalOutcomesV8215)if(!Account.MedalReceiptsV8215.Contains(Outcome.Key))
        {
            if(Account.MedalReceiptsV8215.Num()>=10000){Account=Before;RetryNoticeV81();return;}
            if(Outcome.Value.Key<0||Outcome.Value.Key>=UE_ARRAY_COUNT(Account.Medals)){Account=Before;RetryNoticeV81();return;}
            auto& S=Account.Medals[Outcome.Value.Key];const auto& D=Outcome.Value.Value;
            S.Infantry=FMath::Min(1000000000,S.Infantry+D.Infantry);S.Armour=FMath::Min(1000000000,S.Armour+D.Armour);S.Victories=FMath::Min(1000000000,S.Victories+D.Victories);S.CleanVictories=FMath::Min(1000000000,S.CleanVictories+D.CleanVictories);S.NaturalRoute|=D.NaturalRoute;Account.MedalReceiptsV8215.Add(Outcome.Key);
            if(const auto* Proof=P.Value.MedalProofsV8216.Find(Outcome.Key))
            {
                const auto BeforeAwards=Account.MedalAwardsV8216;
                FrontlineMedalsV8215::ApplyAwards(Account.MedalAwardsV8216,S,*Proof);
                if(P.Value.NotifyMedalsV8217)for(int32 I:FrontlineMedalsV8215::NationIndices(EFRRUNationV81(Proof->Nation)))
                {
                    const FString Id=FrontlineMedalsV8215::Definition(I).Id;
                    const auto Old=BeforeAwards.FindRef(Id),Now=Account.MedalAwardsV8216.FindRef(Id);
                    if(Now.Grade&&(Now.Grade!=Old.Grade||Now.Repeats!=Old.Repeats||Now.Attachments!=Old.Attachments))
                    {FMedalNotificationV8217 Event;Event.Index=I;Event.Grade=Now.Grade;Event.Repeats=Now.Repeats;Event.Attachments=Now.Attachments;Event.Receipt=Outcome.Key;NewMedalNotices.FindOrAdd(P.Key).Add(Event);}
                }
            }
        }
        Account.EarnedMask|=P.Value.Mask;Account.Routes|=P.Value.Routes;
        for(int32 Feat=0;Feat<CrossFactionCountV83;++Feat)Account.CrossNationsV83[Feat]|=P.Value.CrossNationsV83[Feat];
        for(const auto& D:P.Value.CrossDatesV83){FString* Current=Account.CrossDatesV83.Find(D.Key);if(!Current||D.Value<*Current)Account.CrossDatesV83.Add(D.Key,D.Value);}
        NotifyMask|=P.Value.NotifyMask;CommittedNotifyMask|=P.Value.CommittedNotifyMask;NotifyRoute|=P.Value.NotifyRoute;
        if(P.Value.RouteDate>RouteDate)RouteDate=P.Value.RouteDate;
        for(int32 I=0;I<Count;++I)if(I!=HomewardIndexV82&&(P.Value.Mask&~Before.EarnedMask&(1u<<I)))
            if(const FString* Date=P.Value.Dates.Find(Catalog[I].Id))
            {const FString* Current=Account.Dates.Find(Catalog[I].Id);if(!Current||*Date<*Current)Account.Dates.Add(Catalog[I].Id,*Date);}
    }
    for(int32 Feat=0;Feat<CrossFactionCountV83;++Feat)if(Account.CrossNationsV83[Feat]==CrossRequiredNationsV83[Feat])
    {
        const int32 Index=CrossFactionStartV83+Feat;Account.EarnedMask|=1u<<Index;
        FString CompletionDate;for(int32 N=0;N<4;++N)if(CrossRequiredNationsV83[Feat]&(1u<<N))
            if(const FString* Date=Account.CrossDatesV83.Find(CrossDateKeyV83(Feat,N));Date&&*Date>CompletionDate)CompletionDate=*Date;
        FString* Current=Account.Dates.Find(Catalog[Index].Id);if(!Current||CompletionDate<*Current)Account.Dates.Add(Catalog[Index].Id,CompletionDate);
    }
    if(Account.Routes==AllCampaignRoutesV82&&!(Account.EarnedMask&(1u<<HomewardIndexV82)))
    {Unlock(HomewardIndexV82);if(!RouteDate.IsEmpty())Account.Dates.Add(Catalog[HomewardIndexV82].Id,RouteDate);if(NotifyRoute)NotifyMask|=1u<<HomewardIndexV82;}
    const uint32 NewNotifyMask=NotifyMask&~Before.EarnedMask;bool PrimaryCommitted=false;
    if(SaveLocked(&PrimaryCommitted))
    {
        // A different instance may already have earned an ordinary pending bit.
        // Only our own earlier primary commit still needs its delayed banner.
        for(int32 I=0;I<Count;++I)if((NewNotifyMask|CommittedNotifyMask)&Account.EarnedMask&(1u<<I))NotificationsV81.AddUnique(I);
        // Publish only changes produced by our new receipts after both durable
        // files succeeded. A prior primary-only commit keeps its own intent.
        for(const auto& P:SettlementsV81)
        {
            MedalNotificationsV8217.Append(P.Value.CommittedMedalNoticesV8217);
            if(const auto* Events=NewMedalNotices.Find(P.Key))MedalNotificationsV8217.Append(*Events);
        }
        SettlementsV81.Empty();RetryTaintedV81.Empty();
    }
    else
    {
        // The primary can already contain this honour when only the backup
        // mirror failed. Retain its banner intent until the complete save works.
        if(NotifyRoute&&(Account.EarnedMask&~Before.EarnedMask&(1u<<HomewardIndexV82)))
            for(auto& P:SettlementsV81)if(P.Value.NotifyRoute)P.Value.NotifyMask|=1u<<HomewardIndexV82;
        if(PrimaryCommitted)for(auto& P:SettlementsV81)
        {
            P.Value.CommittedNotifyMask|=P.Value.NotifyMask&NewNotifyMask&Account.EarnedMask;
            if(const auto* Events=NewMedalNotices.Find(P.Key))P.Value.CommittedMedalNoticesV8217.Append(*Events);
        }
        Account=Before;RetryNoticeV81();
    }
}
}

const FDefinition& Definition(int32 I){return Catalog[FMath::Clamp(I,0,Count-1)];}
bool SpecialHonourV844(int32 I){return I==TributeStartV836||I==TributeStartV836+1;}
int32 ChapterV82(int32 I){return I>=12&&I<18?1:I>=18&&I<CrossFactionStartV83?2:0;}
FString BadgeObjectPathV82(int32 I)
{
    if(I<0||I>=Count)return FString();
    if(I>=TributeStartV836)
    {const FString Name=FString(TEXT("T_"))+Catalog[I].Id+TEXT("_V837");return FString(TEXT("/Game/Warfare/UI/V837/Achievements/"))+Name+TEXT(".")+Name;}
    const bool New=I>=12;const FString Name=FString(TEXT("T_"))+Catalog[I].Id+(New?TEXT("_V82"):TEXT("_V76"));
    return (New?FString(TEXT("/Game/FRRUIntegration/FormalV82/UI/Achievements/")):FString(TEXT("/Game/Warfare/UI/V76/Achievements/")))+Name+TEXT(".")+Name;
}
bool Earned(int32 I){Load();return I>=0&&I<Count&&((Account.EarnedMask|Account.CheatHonoursV8218)&(1u<<I));}
bool CheatGrantedAchievementV8218(int32 I){Load();return I>=0&&I<Count&&(Account.CheatHonoursV8218&(1u<<I))&&!(Account.EarnedMask&(1u<<I));}
bool CheatGrantedMedalV8218(const FString& Id){Load();return Account.CheatMedalsV8218.Contains(Id)&&!Account.MedalAwardsV8216.Contains(Id);}
int32 EarnedCount(){Load();int32 N=0;for(int32 I=0;I<Count;++I)N+=Earned(I);return N;}
uint8 CompletedCampaignRouteMaskV82(){Load();return Account.Routes;}
FrontlineMedalsV8215::FStats MedalStatsV8215(EFrontlineFaction F)
{return NationalMedalStatsV87(EFRRUNationV81(uint8(F)));}
FrontlineMedalsV8215::FStats NationalMedalStatsV87(EFRRUNationV81 N)
{Load();return uint8(N)<UE_ARRAY_COUNT(Account.Medals)?Account.Medals[uint8(N)]:FrontlineMedalsV8215::FStats{};}
FrontlineMedalsV8215::FAward MedalAwardV8216(const FString& Id)
{Load();if(const auto* Natural=Account.MedalAwardsV8216.Find(Id))return *Natural;return Account.CheatMedalAwardsV8222.FindRef(Id);}
bool CampaignAssistedV8222(const FGuid& Campaign,bool SavedEligible)
{Load();return Campaign.IsValid()&&(!SavedEligible||Account.Tainted.Contains(Campaign)||RetryTaintedV81.Contains(Campaign));}
uint8 CompletedFeatNationMaskV83(int32 I){Load();return I>=CrossFactionStartV83&&I<TributeStartV836?Account.CrossNationsV83[I-CrossFactionStartV83]:0;}
uint8 RequiredFeatNationMaskV83(int32 I){return I>=CrossFactionStartV83&&I<TributeStartV836?CrossRequiredNationsV83[I-CrossFactionStartV83]:0;}
FString Status(int32 I,bool ZH)
{
    Load();if(Account.Corrupt)return ZH?TEXT("记录读取失败，原文件已保留。"):Account.Notice;
    if(Account.Unsupported)return ZH?TEXT("战功记录版本不受支持，原文件已保留。请使用兼容的游戏版本。"):Account.Notice;
    if(!Account.Notice.IsEmpty())return ZH?TEXT("战功记录暂未保存，正在重试。"):Account.Notice;
    if(CheatGrantedAchievementV8218(I))return ZH?TEXT("作弊授予"):TEXT("Granted by cheat");
    if(Earned(I))return (ZH?TEXT("达成 · "):TEXT("Earned · "))+Account.Dates.FindRef(Catalog[I].Id).Left(10);
    const bool NationalClosed=I>=12&&I<CrossFactionStartV83&&!WarfareFRRUCampaignV82::CampaignsAvailable(Catalog[I].Nation);
    if(NationalClosed&&I>=12&&I<CrossFactionStartV83)
        return ZH?TEXT("暂未开放"):TEXT("Not yet available");
    const uint8 ClosedNationalRoutes=(WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::French)?0:4)
        |(WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::Russian)?0:8);
    if(I>=CrossFactionStartV83&&I<TributeStartV836)
    {
        FString Completed,Remaining;const int32 Feat=I-CrossFactionStartV83;
        for(int32 Nation=0;Nation<4;++Nation)if(CrossRequiredNationsV83[Feat]&(1u<<Nation))
        {
            FString& List=(Account.CrossNationsV83[Feat]&(1u<<Nation))?Completed:Remaining;
            if(!List.IsEmpty())List+=ZH?TEXT("、"):TEXT(", ");List+=ZH?CrossNationNamesZHV83[Nation]:CrossNationNamesENV83[Nation];
        }
        if(Completed.IsEmpty())Completed=ZH?TEXT("无"):TEXT("None");
        const bool Waiting=(CrossRequiredNationsV83[Feat]&~Account.CrossNationsV83[Feat]&ClosedNationalRoutes)!=0;
        return ZH?(Waiting?TEXT("暂未开放 · 已完成："):TEXT("尚未达成 · 已完成："))+Completed+TEXT("\n仍需完成：")+Remaining
            :(Waiting?TEXT("Not yet available · Completed: "):TEXT("Not earned · Completed: "))+Completed+TEXT("\nRemaining: ")+Remaining;
    }
    if(I==HomewardIndexV82)
    {
        int32 Routes=0;for(int32 Bit=0;Bit<4;++Bit)Routes+=(Account.Routes&(1u<<Bit))!=0;
        const bool Waiting=(AllCampaignRoutesV82&~Account.Routes&ClosedNationalRoutes)!=0;
        if(ZH)return Waiting?FString::Printf(TEXT("暂未开放 · 已完成战役路线：%d / 4"),Routes)
            :FString::Printf(TEXT("尚未达成 · 已完成战役路线：%d / 4"),Routes);
        return Waiting?FString::Printf(TEXT("Not yet available · Completed campaign routes: %d / 4"),Routes)
            :FString::Printf(TEXT("Not earned · Completed campaign routes: %d / 4"),Routes);
    }
    return ZH?TEXT("尚未达成"):TEXT("Not earned");
}
void ResetEncounter(AWarfareFrontlineGameMode* B){Encounters.Remove(B);}
void WithScopedCombatEventsV839(AWarfareFrontlineGameMode* B,TFunctionRef<void()> Work)
{if(!B||!B->IsProgressionVolatile())return;const auto Before=Encounters;ON_SCOPE_EXIT{Encounters=Before;};Work();}
void WithScopedCombatLedgerV84412(AWarfareFrontlineGameMode* B,TFunctionRef<void()> Work)
{
    FString CurrentDir;
    FString CurrentRoot=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V84414/Runtime"));
    FString NewRoot=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V84415/Runtime"));
    FString V85Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V85/Runtime"));
    FPaths::NormalizeDirectoryName(CurrentRoot);
    FPaths::NormalizeDirectoryName(NewRoot);
    FPaths::NormalizeDirectoryName(V85Root);
    const bool CurrentAllowed=B&&B->GetWorld()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(CurrentDir)
        &&((FParse::Param(FCommandLine::Get(),TEXT("WFV84414Focused"))
            &&CurrentDir.StartsWith(CurrentRoot+TEXT("/"),ESearchCase::IgnoreCase))
           ||(FParse::Param(FCommandLine::Get(),TEXT("WFV84415Focused"))
            &&CurrentDir.StartsWith(NewRoot+TEXT("/"),ESearchCase::IgnoreCase))
           ||(FParse::Param(FCommandLine::Get(),TEXT("WFV85Focused"))
            &&CurrentDir.StartsWith(V85Root+TEXT("/"),ESearchCase::IgnoreCase))
           ||FParse::Param(FCommandLine::Get(),TEXT("WFV851Focused")));
    if((!FrontlineGameplayFocusedV84412::Allowed(B)&&!FrontlineMovementFocusedV84413::Allowed(B)&&!CurrentAllowed)||!B->IsProgressionVolatile()||!DisposableAccount())return;
    const auto PriorAccount=Account;const auto PriorEncounters=Encounters;
    FTransientReviewV81 Transient;
    ON_SCOPE_EXIT{Account=PriorAccount;Encounters=PriorEncounters;};
    Work();
}
bool HeavyTankModelV84415(EFrontlineVehicle Model)
{
    switch(Model)
    {
    case EFrontlineVehicle::MarkIVMale:case EFrontlineVehicle::MarkIVFemale:
    case EFrontlineVehicle::MarkVMale:case EFrontlineVehicle::MarkVStar:
    case EFrontlineVehicle::BeuteMarkIV:case EFrontlineVehicle::BeuteMarkIVFemale:
    case EFrontlineVehicle::MarkIVPioneer:case EFrontlineVehicle::BeutePioneer:
    case EFrontlineVehicle::A7V:case EFrontlineVehicle::A7VGretchen:return true;
    default:return false;
    }
}
bool ChallengeConditionV84415(const AWarfareFrontlineGameMode* B,int32 Index)
{
    if(!B||int32(B->GetCampaignDifficulty())<1)return false;
    const auto* S=Encounters.Find(const_cast<AWarfareFrontlineGameMode*>(B));
    if(!S||!S->Started||S->Id!=B->GetEncounterId())return false;
    if(Index==3)
    {
        for(const auto& Section:S->GunSectionKillsV851)if(Section.Value>=12)return true;
    }
    return Index==6&&!WarfareFRRUCampaignV82::Active(B)&&B->GetPlayerFaction()==EFrontlineFaction::German&&!S->HeavyRifleVictimsV84415.IsEmpty();
}
void ArmourDestroyedV84415(AWarfareFrontlineGameMode* B,const AFrontlineTank* Victim,const FFrontlineKillSourceV65& Source)
{
    // Called after the ordinary death ledger has accepted this fatal hull once.
    // Recheck ownership and source so a tooltip/statistics fixture cannot award it.
    if(!Active(B)||WarfareFRRUCampaignV82::Active(B)||!IsValid(Victim)
        ||FFrontlineArmourKillTallyV8220Review::Owner(Victim)!=B||Victim->Team!=1||Victim->Serial<=0
        ||!FMath::IsFinite(Victim->GetHullIntegrity())||Victim->GetHullIntegrity()>0
        ||Victim->IsRailConvoyV32()||!HeavyTankModelV84415(Victim->Type)
        ||Source.Team!=0||Source.Role!=int32(EFrontlineRole::AntiTankRifle)||Source.SquadId==INDEX_NONE
        ||Source.VehicleSerial!=INDEX_NONE||Source.Support!=INDEX_NONE)return;
    const auto* Q=B->FindSquad(Source.SquadId);
    if(!Q||Q->Team!=0||Q->Role!=EFrontlineRole::AntiTankRifle)return;
    State(B).HeavyRifleVictimsV84415.Add(FIntPoint(Source.SquadId,Victim->Serial));
}
void BeginEncounter(AWarfareFrontlineGameMode* B)
{
    Load();auto& S=State(B);S.Started=true;const auto* C=B?B->GetCurrentCampaign():nullptr;
    const bool Expanded=WarfareFRRUCampaignV82::Active(B);
    S.Campaign=Expanded?WarfareFRRUCampaignV82::CampaignId(B):C?C->Id:FGuid();
    if(B){S.NationV82=WarfareFRRUCampaignV82::Nation(B,0);S.DateV82=WarfareFRRUCampaignV82::CalendarDate(B);S.DifficultyV82=int32(B->GetCampaignDifficulty());}
    if(Expanded)
    {S.NationV82=WarfareFRRUCampaignV82::Nation(B);S.MissionV82=WarfareFRRUCampaignV82::MissionId(B);S.DateV82=WarfareFRRUCampaignV82::CalendarDate(B);S.DifficultyV82=int32(WarfareFRRUCampaignV82::Difficulty(B));}
    const bool CampaignEligible=Expanded?WarfareFRRUCampaignV82::HonourEligible(B):C&&C->bAchievementEligibleV76;
    S.Eligible=B&&(!B->IsProgressionVolatile()||IsolatedAwardsAllowed(B)||FrontlineUltimateArchiveV8215::ReplayEligible(B))&&S.Campaign.IsValid()&&CampaignEligible&&!Account.Corrupt&&!Account.Unsupported&&!Account.Tainted.Contains(S.Campaign);
    if(B&&FrontlineDebugV37::HasCheatState(B))MarkCheat(B);
    S.MinimumMorale=1;
}
void MarkCheat(AWarfareFrontlineGameMode* B)
{
    if(FrontlineUltimateArchiveV8215::Active(B)){FrontlineUltimateArchiveV8215::MarkAssisted(B);State(B).Eligible=false;return;}
    if(!B||(B->IsProgressionVolatile()&&!DisposableAccount()&&!IsolatedAwardsAllowed(B)))return;Load();State(B).Eligible=false;
    if(WarfareFRRUCampaignV82::Active(B))
    {
        const FGuid Id=WarfareFRRUCampaignV82::CampaignId(B);if(!Id.IsValid())return;
        // Account taint remains authoritative even if a campaign write must retry.
        Account.Tainted.Add(Id);WarfareFRRUCampaignV82::MarkHonourAssisted(B);
        RetryTaintedV81.Add(Id);PersistSettlementsV81();return;
    }
    if(const auto* C=B->GetCurrentCampaign())
    {
        Account.Tainted.Add(C->Id);if(B->IsProgressionVolatile())const_cast<FFrontlineCampaignProgress*>(C)->bAchievementEligibleV76=false;else B->MarkAchievementAssistedV76();
        RetryTaintedV81.Add(C->Id);PersistSettlementsV81();
    }
}
void Recruited(AWarfareFrontlineGameMode* B,int32 Role,int32 Team)
{
    if(!Active(B)||Team!=0||Role<0||Role>=int32(EFrontlineRole::Count))return;
    auto& S=State(B);++S.Roles[Role];
    for(int32 I=B->Squads.Num()-1;I>=0;--I)if(const auto& Q=B->Squads[I];Q.Team==0&&int32(Q.Role)==Role)
    {
        if(!S.FirstSectionsV84.Contains(Role))S.FirstSectionsV84.Add(Role,Q.Id);
        if(!S.OriginalSectionSizesV84.Contains(Q.Id))S.OriginalSectionSizesV84.Add(Q.Id,Q.Members.Num());
        if(Role==int32(EFrontlineRole::Lewis)||Role==int32(EFrontlineRole::MobileGun))S.GunSectionsV851.Add(Q.Id);
        if(Role==int32(EFrontlineRole::Lewis))for(const auto& U:Q.Members)if(U.IsValid()&&U->IsMachineGunner()){S.OriginalGunners.Add(Q.Id,U);break;}
        break;
    }
}
void Loss(AWarfareFrontlineGameMode* B,int32 VictimTeam,const FFrontlineKillSourceV65& Source,bool Armour)
{
    if(!Active(B))return;auto& S=State(B);
    if(VictimTeam==0&&Source.Team==0)++S.FriendlyFireV8216;
    RecordNationalLossV82(S,VictimTeam,Source,Armour);
    if(VictimTeam==1&&Source.Team==0&&!Armour&&Source.SquadId!=INDEX_NONE
        &&Source.Role>=0&&Source.Role<int32(EFrontlineRole::Count)
        &&Source.VehicleSerial==INDEX_NONE&&Source.Support==INDEX_NONE&&S.GunSectionsV851.Contains(Source.SquadId))
        // Identity was recorded by the actual recruitment hook. A last
        // discharged round retains its section even after dead actors retire.
        ++S.GunSectionKillsV851.FindOrAdd(Source.SquadId);
    if(Source.Support==int32(EFrontlineSupport::Gas)&&Source.Team==0)
    {if(VictimTeam==0)++S.GasFriendly;else if(!Armour)++S.GasEnemy;}
    if(Source.Team==0&&Source.FireMissionId!=INDEX_NONE&&(Source.Support==int32(EFrontlineSupport::Artillery)||Source.Support==int32(EFrontlineSupport::Mortar)||Source.Support==int32(EFrontlineSupport::CreepingBarrage)))
    {auto& M=S.Batteries.FindOrAdd(Source.FireMissionId);if(VictimTeam==0)++M.Friendly;else if(!Armour)++M.Enemy;}
    if(Source.Team==0&&VictimTeam==1&&Source.Role==int32(EFrontlineRole::AntiTankRifle)&&Armour)S.AntiTankKill=true;
    if(Source.Team==0&&VictimTeam==1&&!Armour&&Source.Role==int32(EFrontlineRole::Mills))
        if(const auto* Q=B->FindSquad(Source.SquadId);Q&&Q->Members.ContainsByPredicate([](const auto& U){return U.IsValid()&&U->IsStormtrooper();}))++S.StormKills;
}
void OfficerOrder(AWarfareFrontlineGameMode* B,AWarfareFrontlineSoldier* O,int32 Squad,float Relief)
{if(Active(B)&&O&&O->Team==0&&O->IsCommissionedOfficer()&&Squad!=O->SquadId&&Relief>0){auto& R=State(B).Officers.FindOrAdd(O->SquadId);R.Actor=O;R.Squads.Add(Squad);R.Relief+=Relief;}}
void ObstacleCleared(AWarfareFrontlineGameMode* B,int32 Team,const FVector& P,int32 ClearingSquad)
{
    if(!Active(B)||Team!=0||!FMath::IsFinite(P.X)||!FMath::IsFinite(P.Y))return;
    auto& S=State(B);const FIntPoint Key(FMath::RoundToInt(P.X),FMath::RoundToInt(P.Y));
    const bool Fresh=!S.Obstacles.Contains(Key);S.Obstacles.Add(Key);
    // The field work completion supplies the actual clearing section. A
    // nearby section or a legacy un-attributed event cannot claim its deed.
    const auto* Q=B->FindSquad(ClearingSquad);
    if(Fresh&&Q&&Q->Team==0&&Q->Role==EFrontlineRole::Pioneer&&Q->LivingCount()>0
        &&Q->Members.ContainsByPredicate([](const auto& U){return U.IsValid()&&U->bFieldEntered&&U->IsActiveCombatant();}))
        S.PioneerClearancesV837.FindOrAdd(ClearingSquad).Add(Key);
}
void EnemyTrenchCaptured(AWarfareFrontlineGameMode* B,int32 Previous,int32 Team)
{
    if(!Active(B)||Previous!=1||Team!=0)return;auto& S=State(B);S.AnyCapture=true;
    if(S.Obstacles.Num()>=4)S.Captured=true;
    for(auto& M:S.Batteries)if(M.Value.Enemy>=8&&M.Value.Friendly==0)M.Value.CapturedAfter=true;
}
void NativePassageClearedV82(AWarfareFrontlineGameMode* B,int32 Team,const FString& NodeId,const FVector& Position)
{
    if(!Active(B)||Team!=0||!WarfareFRRUCampaignV82::Active(B))return;auto& S=State(B);
    if(S.MissionV82!=WarfareFRRUCampaignV82::MissionId(B)||S.Campaign!=WarfareFRRUCampaignV82::CampaignId(B))return;
    if(RecordNativePassageV82(S,NodeId))ObstacleCleared(B,Team,Position);
}
void NativeObjectiveStageV82(AWarfareFrontlineGameMode* B,const FString& StageId,bool Fallback)
{
    if(!Active(B)||!WarfareFRRUCampaignV82::Active(B))return;auto& S=State(B);
    if(S.MissionV82!=WarfareFRRUCampaignV82::MissionId(B)||S.Campaign!=WarfareFRRUCampaignV82::CampaignId(B))return;
    RecordNativeStageV82(S,StageId,Fallback);
}
void RailSectionDeployed(AWarfareFrontlineGameMode* B,AFrontlineTank* T,int32 Role,int32 SquadId)
{
    const auto* Q=B?B->FindSquad(SquadId):nullptr;
    if(Active(B)&&T&&T->Team==0&&Q&&Q->LivingCount()>0)
    {auto& R=State(B).Rails.FindOrAdd(T->Serial);R.Actor=T;const int32 I=Role==int32(EFrontlineRole::Rifle)?0:Role==int32(EFrontlineRole::Pioneer)?1:INDEX_NONE;if(I!=INDEX_NONE){R.Sections|=1u<<I;R.Squads[I]=SquadId;}}
}
void Tick(AWarfareFrontlineGameMode* B,float DT)
{
    if(!Active(B))return;auto& S=State(B);S.MinimumMorale=FMath::Min(S.MinimumMorale,FMath::Clamp(B->Morale[0]/FrontlineMoraleV36::StartingMaximum(B,0),0.f,1.f));
    S.TickAge+=DT;if(S.TickAge<.25f)return;S.TickAge=0;
    TrackEnteredInfantryV842(B,S);
    for(const auto& Q:B->Squads)if(Q.Team==0&&Q.LivingCount()>0)
    {
        auto& M=S.Smoke.FindOrAdd(Q.Id);AWarfareFrontlineSoldier* Men[2]={nullptr,nullptr};int32 N=0;
        for(const auto& U:Q.Members)if(U.IsValid()&&U->IsActiveCombatant()&&U->bFieldEntered&&N<2)Men[N++]=U.Get();
        if(N<2){M.InSmoke=false;continue;}
        bool Covered=true,Same=true;float Gain[2]={0,0};
        for(int32 I=0;I<2;++I)
        {
            const float Along=Men[I]->Feet().X*B->GetTeamDirection(0);
            Same&=M.Men[I].Get()==Men[I];Covered&=B->IsInFriendlySmoke(Men[I]->Feet(),0);
            if(M.Men[I].Get()==Men[I])
            {const float Delta=Along-M.Last[I];if(Delta>=0&&Delta<=150.f)Gain[I]=FMath::Max(0.f,Along-M.High[I]);else if(FMath::Abs(Delta)>150.f)Covered=false;}
            else M.High[I]=Along;
            M.Men[I]=Men[I];M.Last[I]=Along;M.High[I]=FMath::Max(M.High[I],Along);
        }
        if(Covered&&Same&&M.InSmoke)M.Travel+=FMath::Min(Gain[0],Gain[1]);
        M.InSmoke=Covered;
    }
}
void LegacyOutcomePersistedV83(AWarfareFrontlineGameMode* B,int32 Winner,bool Saved)
{
    if(!B||WarfareFRRUCampaignV82::Active(B)||(Winner!=0&&Winner!=1)||(Winner==0?B->Phase!=EFrontlinePhase::Victory:B->Phase!=EFrontlinePhase::Defeat))return;
    auto& S=State(B);const auto* Campaign=B->GetCurrentCampaign();
    S.LegacySavedV83=Saved&&!B->IsProgressionVolatile()&&S.Started&&S.Id==B->GetEncounterId()
        &&Campaign&&Campaign->Id==S.Campaign&&Campaign->Faction==B->GetPlayerFaction()&&Campaign->bAchievementEligibleV76&&B->GetEncounterCampaignMission();
    if(S.LegacySavedV83&&S.DeferredOutcomeV8219.IsSet())
    {
        // Release the original result-page proof, never recapture later actors,
        // losses, morale, route dates or support kills after a disk retry.
        MergeSettlementV8219(SettlementsV81.FindOrAdd(S.Campaign),S.DeferredOutcomeV8219.GetValue());
        S.DeferredOutcomeV8219.Reset();PersistSettlementsV81();
    }
}
void Finish(AWarfareFrontlineGameMode* B,int32 Winner)
{
    // Defeats record real kills but never award victory achievements or route
    // honours. Settlement receipts use the encounter UUID in both paths.
    if(B&&Winner==1&&B->Phase==EFrontlinePhase::Defeat)
    {
        auto& S=State(B);const bool Replay=FrontlineUltimateArchiveV8215::ReplayEligible(B);
        const bool Expanded=WarfareFRRUCampaignV82::Active(B);
        const bool NationalSaved=Expanded&&WarfareFRRUWorldV82::OutcomePersisted(B)&&WarfareFRRUCampaignV82::CampaignId(B)==S.Campaign&&WarfareFRRUCampaignV82::HonourEligible(B);
        if(Expanded&&!NationalSaved)return;
        const auto* Campaign=B->GetCurrentCampaign();
        const bool Bound=Campaign&&Campaign->Id==S.Campaign&&Campaign->Faction==B->GetPlayerFaction()
            &&Campaign->bAchievementEligibleV76&&B->GetEncounterCampaignMission();
        if(S.Started&&S.Eligible&&!S.SettlementCaptured&&(NationalSaved||S.LegacySavedV83||Replay||IsolatedAwardsAllowed(B)||(!B->IsProgressionVolatile()&&Bound)))
        {
            Load();if(!Account.Corrupt&&!Account.Unsupported&&!Account.Tainted.Contains(S.Campaign)&&!Account.MedalReceiptsV8215.Contains(S.Id))
            {
                FSettlementV81 P;P.CampaignV8215=S.Campaign;FrontlineMedalsV8215::FStats D;
                D.Infantry=FMath::Clamp(B->CombatRecordV65[0].Total.Infantry,0,1000000);D.Armour=FMath::Clamp(B->CombatRecordV65[0].Total.Armour,0,1000000);
                P.MedalOutcomesV8215.Add(S.Id,TPair<int32,FrontlineMedalsV8215::FStats>(int32(WarfareFRRUCampaignV82::Nation(B,0)),D));
                S.CapturedInfantryKillsV8217=D.Infantry;S.CapturedArmourKillsV8217=D.Armour;S.SettlementCaptured=true;S.PendingInitialReceiptV8219=true;
                if(!NationalSaved&&!S.LegacySavedV83&&!Replay&&!IsolatedAwardsAllowed(B))S.DeferredOutcomeV8219=MoveTemp(P);
                else{MergeSettlementV8219(SettlementsV81.FindOrAdd(S.Campaign),P);PersistSettlementsV81();}
            }
        }
        return;
    }
    if(!B||Winner!=0||B->Phase!=EFrontlinePhase::Victory)return;auto& S=State(B);const bool Replay=FrontlineUltimateArchiveV8215::ReplayEligible(B);if(!S.Started||!S.Eligible||S.SettlementCaptured||(B->IsProgressionVolatile()&&!IsolatedAwardsAllowed(B)&&!Replay))return;Load();
    const auto* Campaign=B->GetCurrentCampaign();
    const bool Expanded=WarfareFRRUCampaignV82::Active(B);
    if(Expanded&&!WarfareFRRUWorldV82::OutcomePersisted(B))return;
    if(!Expanded&&!S.LegacySavedV83&&!IsolatedAwardsAllowed(B)&&!Replay
        &&(!Campaign||Campaign->Id!=S.Campaign||Campaign->Faction!=B->GetPlayerFaction()||!B->GetEncounterCampaignMission()))return;
    const bool CampaignEligible=Expanded?WarfareFRRUCampaignV82::CampaignId(B)==S.Campaign&&WarfareFRRUCampaignV82::HonourEligible(B):Campaign&&Campaign->Id==S.Campaign&&Campaign->bAchievementEligibleV76;
    if(Account.Tainted.Contains(S.Campaign)||Account.Corrupt||Account.Unsupported||!CampaignEligible)return;
    TrackEnteredInfantryV842(B,S);
    S.MinimumMorale=FMath::Min(S.MinimumMorale,FMath::Clamp(B->Morale[0]/FrontlineMoraleV36::StartingMaximum(B,0),0.f,1.f));
    bool Conditions[Count]={};
    const bool Hard=int32(B->GetCampaignDifficulty())>=1;
    const bool Breakthrough=Expanded?WarfareFRRUWorldV82::IsBreakthroughVictory(B):B->EndReason.Contains(TEXT("突破"));
    Conditions[0]=Breakthrough&&S.Obstacles.Num()>=3;
    Conditions[1]=S.MinimumMorale<=.10001f&&Breakthrough;
    for(const auto& R:S.Officers)Conditions[2]|=R.Value.Squads.Num()>=6&&R.Value.Actor.IsValid()&&R.Value.Actor->IsActiveCombatant();
    Conditions[3]=ChallengeConditionV84415(B,3);
    Conditions[4]=S.Obstacles.Num()>=6;
    Conditions[5]=!Expanded&&B->GetPlayerFaction()==EFrontlineFaction::German&&S.StormKills>=24;
    Conditions[6]=ChallengeConditionV84415(B,6);
    int32 SmokeCrossings=0;for(const auto& M:S.Smoke)SmokeCrossings+=M.Value.Travel>=1200.f;
    Conditions[7]=SmokeCrossings>=3;Conditions[8]=S.GasEnemy>=10&&S.GasFriendly==0;
    for(const auto& M:S.Batteries)Conditions[9]|=M.Value.Enemy>=12&&M.Value.Friendly==0;
    for(const auto& R:S.Rails)Conditions[10]|=R.Value.Sections==3&&R.Value.Actor.IsValid()&&R.Value.Actor->GetHullIntegrity()>=R.Value.Actor->GetMaximumHullIntegrity()*.75f;
    if(!Expanded)Conditions[TributeStartV836]=PioneerWorkCompletedV842(B,S);
    Conditions[TributeStartV836+1]=AllQuietCriterionV8310(B,S);
    uint32 QualifiedMask=0;for(int32 I=0;I<Count;++I)if(I!=HomewardIndexV82&&Conditions[I]&&Hard)QualifiedMask|=1u<<I;
    if(Expanded)QualifiedMask|=NationalMaskV82(CaptureNationalProofV82(B,S));
    QualifiedMask&=~Account.EarnedMask;
    const bool DeferLegacyOutcome=!Expanded&&!S.LegacySavedV83&&!IsolatedAwardsAllowed(B)&&!Replay;
    auto CrossProof=CaptureCrossBattleProofV83(B,S,CampaignEligible,Breakthrough);
    // This frozen local proof can enter the writable queue only after the
    // real campaign save succeeds. All battle evidence stays at outcome time.
    if(DeferLegacyOutcome)CrossProof.Saved=true;
    const uint8 CrossFeats=CrossBattleFeatsV83(CrossProof);
    uint8 Route=0;const int32 RequiredMissions=B->GetCampaignMissions().Num();
    if(RequiredMissions>0&&!Replay)
    {
        if(Expanded)
        {
            // Finish follows the persisted outcome, before the ending's outro gate.
            FFRRUCampaignSummaryV82 Record;
            if(WarfareFRRUCampaignV82::CurrentRecord(B,Record)&&Record.Id==S.Campaign&&Record.Completed>=RequiredMissions)
                Route=RouteBitV82(Record.PlayerNation);
        }
        else if(Campaign&&Campaign->CompletedMissions>=RequiredMissions)
            Route=Campaign->Faction==EFrontlineFaction::British?1:Campaign->Faction==EFrontlineFaction::German?2:0;
    }
    S.SettlementCaptured=true;
    if(QualifiedMask||(Route&~Account.Routes)||CrossFeats||!Account.MedalReceiptsV8215.Contains(S.Id))
    {
        FSettlementV81 Pending;Pending.Mask|=QualifiedMask;
        Pending.CampaignV8215=S.Campaign;
        if(!Account.MedalReceiptsV8215.Contains(S.Id))
        {
            FrontlineMedalsV8215::FStats D;D.Infantry=FMath::Clamp(B->CombatRecordV65[0].Total.Infantry,0,1000000);D.Armour=FMath::Clamp(B->CombatRecordV65[0].Total.Armour,0,1000000);D.Victories=Replay?0:1;D.CleanVictories=!Replay&&Hard&&B->CombatRecordV65[0].ArmourLost==0?1:0;D.NaturalRoute=Route!=0;
            FFRRUCampaignSummaryV82 NationalRecord;const bool NationalRecordBound=Expanded&&WarfareFRRUCampaignV82::CurrentRecord(B,NationalRecord)&&NationalRecord.Id==S.Campaign&&NationalRecord.PlayerNation==S.NationV82;
            const bool Undefeated=Expanded?NationalRecordBound&&NationalRecord.Defeats==0:Campaign&&Campaign->Research.Defeats==0;
            if(Route&&Hard&&Undefeated)D.NaturalRoute|=2;
            Pending.MedalOutcomesV8215.Add(S.Id,TPair<int32,FrontlineMedalsV8215::FStats>(int32(WarfareFRRUCampaignV82::Nation(B,0)),D));
            S.PendingInitialReceiptV8219=true;
            S.CapturedInfantryKillsV8217=D.Infantry;S.CapturedArmourKillsV8217=D.Armour;
            FrontlineMedalsV8215::FProof Proof;Proof.Campaign=S.Campaign;Proof.Nation=int32(WarfareFRRUCampaignV82::Nation(B,0));Proof.Date=WarfareFRRUCampaignV82::CalendarDate(B);Proof.Difficulty=int32(B->GetCampaignDifficulty());Proof.Infantry=D.Infantry;Proof.Armour=D.Armour;Proof.Losses=B->CombatRecordV65[0].InfantryLost;Proof.ArmourLosses=B->CombatRecordV65[0].ArmourLost;Proof.Obstacles=S.Obstacles.Num();Proof.FriendlyFire=S.FriendlyFireV8216;Proof.MinimumMorale=S.MinimumMorale;Proof.Won=true;Proof.Replay=Replay;Proof.Completed=Route!=0;Proof.Undefeated=Route&&Undefeated;
            Proof.EnteredInfantryV842=S.EnteredInfantryV842.Num();Proof.ReturnedInfantryV842=ReturnedInfantryV842(B,S);
            for(int32 K=0;K<UE_ARRAY_COUNT(B->CombatRecordV65[0].SupportSources);++K){const auto& Support=B->CombatRecordV65[0].SupportSources[K];Proof.SupportArmourV84412+=Support.Armour;if(K==int32(EFrontlineSupport::AirStrike)||K==int32(EFrontlineSupport::AirBombing)){Proof.AircraftInfantryV84412+=Support.Infantry;Proof.AircraftArmourV84412+=Support.Armour;}}
            Pending.MedalProofsV8216.Add(S.Id,Proof);
        }
        const bool SilentReview=B->IsProgressionVolatile()&&SilentAwardReviewBattleV836.Get()==B&&DisposableAccount();
        const bool MayNotify=(!B->IsProgressionVolatile()||Replay||IsolatedAwardsAllowed(B))&&!SilentReview;
        if(MayNotify){Pending.NotifyMask|=QualifiedMask;Pending.NotifyRoute|=Route!=0;Pending.NotifyMedalsV8217=true;}
        const FString Date=FDateTime::UtcNow().ToIso8601();
        CaptureCrossSettlementV83(Pending,CrossFeats,RouteBitV82(CrossProof.Nation),Date,MayNotify);
        if(Route&~Pending.Routes)Pending.RouteDate=Date;Pending.Routes|=Route;
        for(int32 I=0;I<Count;++I)if(I!=HomewardIndexV82&&(QualifiedMask&(1u<<I))&&!Pending.Dates.Contains(Catalog[I].Id))Pending.Dates.Add(Catalog[I].Id,Date);
        if(DeferLegacyOutcome)S.DeferredOutcomeV8219=MoveTemp(Pending);
        else MergeSettlementV8219(SettlementsV81.FindOrAdd(S.Campaign),Pending);
    }
    PersistSettlementsV81();
}
bool FinalizeBattleKillsV8217(AWarfareFrontlineGameMode* B)
{
 if(B&&!(WarfareFRRUCampaignV82::Active(B)?WarfareFRRUCampaignV82::FinalizeCampaignCombatRecordV87(B):B->FinalizeCampaignCombatRecordV8217())){UE_LOG(LogTemp,Warning,TEXT("Final campaign combat record could not be saved; result retained for retry."));return false;}
 if(!B||(B->Phase!=EFrontlinePhase::Victory&&B->Phase!=EFrontlinePhase::Defeat))return true;
 auto& S=State(B);const bool Replay=FrontlineUltimateArchiveV8215::ReplayEligible(B);
 const auto Nation=WarfareFRRUCampaignV82::Nation(B,0);
 if(!S.Started||!S.Eligible||!S.SettlementCaptured||S.Id!=B->GetEncounterId()||S.CapturedInfantryKillsV8217<0||S.CapturedArmourKillsV8217<0||uint8(Nation)>3||Nation!=S.NationV82||(B->IsProgressionVolatile()&&!IsolatedAwardsAllowed(B)&&!Replay))return true;
 Load();if(Account.Tainted.Contains(S.Campaign))return true;
 if(S.DeferredOutcomeV8219.IsSet()){B->LastMessage=TEXT("结局存档尚未保存，请重试。");return false;}
 if(S.PendingInitialReceiptV8219)
 {
  if(Account.Corrupt||Account.Unsupported){B->LastMessage=Account.Notice;return false;}
  PersistSettlementsV81();const auto* Initial=SettlementsV81.Find(S.Campaign);
  if(!Account.MedalReceiptsV8215.Contains(S.Id)||(Initial&&Initial->MedalOutcomesV8215.Contains(S.Id)))
  {B->LastMessage=Account.Notice;return false;}
  S.PendingInitialReceiptV8219=false;
 }
 if(Account.Corrupt||Account.Unsupported)return true;
 auto CommitPending=[&]()
 {
  PersistSettlementsV81();const auto* Pending=SettlementsV81.Find(S.Campaign);
  // A primary-only save can already be visible after MergeDisk. Its pending
  // mirror remains authoritative: do not retire the result or alter its range.
  if(!Account.MedalReceiptsV8215.Contains(S.PendingTailReceiptV8219)||(Pending&&Pending->MedalOutcomesV8215.Contains(S.PendingTailReceiptV8219)))
  {B->LastMessage=Account.Notice;return false;}
  S.CapturedInfantryKillsV8217=S.PendingTailInfantryV8219;S.CapturedArmourKillsV8217=S.PendingTailArmourV8219;
  S.PendingTailReceiptV8219.Invalidate();return true;
 };
 if(S.PendingTailReceiptV8219.IsValid()&&!CommitPending())return false;
 const int32 Infantry=FMath::Clamp(B->CombatRecordV65[0].Total.Infantry,0,1000000),Armour=FMath::Clamp(B->CombatRecordV65[0].Total.Armour,0,1000000);
 FrontlineMedalsV8215::FStats Delta;
 Delta.Infantry=FMath::Max(0,Infantry-S.CapturedInfantryKillsV8217);Delta.Armour=FMath::Max(0,Armour-S.CapturedArmourKillsV8217);
 if(!Delta.Infantry&&!Delta.Armour){S.OutroKillsFinalizedV8217=true;return true;}
 // Each successfully flushed range receives one deterministic receipt. An
 // ending screen may remain open while already committed support keeps landing.
 // These rows carry no proof, so awards and victory counts stay frozen.
 FGuid Receipt;const FString Digest=FMD5::HashAnsiString(*FString::Printf(TEXT("%s:outro-kills-v8219:%d:%d:%d:%d"),*S.Id.ToString(EGuidFormats::Digits),S.CapturedInfantryKillsV8217,S.CapturedArmourKillsV8217,Infantry,Armour));
 if(!FGuid::ParseExact(Digest,EGuidFormats::Digits,Receipt))return false;
 auto& Pending=SettlementsV81.FindOrAdd(S.Campaign);Pending.CampaignV8215=S.Campaign;
 Pending.MedalOutcomesV8215.Add(Receipt,TPair<int32,FrontlineMedalsV8215::FStats>(int32(Nation),Delta));
 S.PendingTailReceiptV8219=Receipt;S.PendingTailInfantryV8219=Infantry;S.PendingTailArmourV8219=Armour;
 S.OutroKillsFinalizedV8217=CommitPending();
 return S.OutroKillsFinalizedV8217;
}
int32 TakeNotificationV81()
{
    if(!Account.Unsupported&&HasPendingV81()&&FPlatformTime::Seconds()>=NextRetryV81)PersistSettlementsV81();
    if(NotificationsV81.IsEmpty())return INDEX_NONE;
    const int32 I=NotificationsV81[0];NotificationsV81.RemoveAt(0);return I;
}

FMedalNotificationV8217 TakeMedalNotificationV8217()
{
    if(!Account.Unsupported&&HasPendingV81()&&FPlatformTime::Seconds()>=NextRetryV81)PersistSettlementsV81();
    if(MedalNotificationsV8217.IsEmpty())return {};
    const auto Event=MedalNotificationsV8217[0];MedalNotificationsV8217.RemoveAt(0);return Event;
}

#include "WarfareFrontlineHonoursLifecycleV83.inl"
#include "WarfareFrontlineCheatHonoursV8218.inl"
#include "WarfareFrontlineCheatCampaignChecksV8222.inl"
#include "WarfareFrontlineHonoursLifecycleChecksV83.inl"
#include "WarfareFrontlineMedalsReviewV8215.inl"
#include "WarfareFrontlineMedalNotificationChecksV8217.inl"
#include "WarfareFrontlineAwardReceiptChecksV84414.inl"
#include "WarfareFrontlineRestartTailChecksV8219.inl"
#include "WarfareFrontlineInitialOutcomeChecksV8219.inl"

void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    Check(TEXT("v76_achievement_review_uses_disposable_account"),B&&B->IsProgressionVolatile()&&DisposableAccount());if(!B||!B->IsProgressionVolatile()||!DisposableAccount())return;
    FTransientReviewV81 NotificationScope;
    Load();const auto Saved=Account;const auto Previous=State(B);const auto* C=B->GetCurrentCampaign();const bool CampaignEligible=C?C->bAchievementEligibleV76:false;
    ON_SCOPE_EXIT{Account=Saved;Encounters.Add(B,Previous);if(C)const_cast<FFrontlineCampaignProgress*>(C)->bAchievementEligibleV76=CampaignEligible;FString Response;FrontlineDebugV37::Execute(B,TEXT("reset"),Response);};
    const uint32 Before=Account.EarnedMask;BeginEncounter(B);Finish(B,0);Check(TEXT("v76_review_guard_keeps_earned_mask_unchanged"),Account.EarnedMask==Before);
    TSet<FString> OriginalIds;for(int32 I=0;I<12;++I)OriginalIds.Add(Catalog[I].Id);Check(TEXT("v76_twelve_stable_unique_achievement_ids"),OriginalIds.Num()==12&&FString(Catalog[11].Id)==TEXT("long_homeward"));
    TSet<FString> Ids;int32 France=0,Russia=0;for(int32 I=0;I<CrossFactionStartV83;++I){const auto& D=Catalog[I];Ids.Add(D.Id);France+=D.Nation==EFRRUNationV81::French;Russia+=D.Nation==EFRRUNationV81::Russian;}
    Check(TEXT("v82_twenty_four_unique_ids_six_honours_for_each_new_nation"),Ids.Num()==CrossFactionStartV83&&France==6&&Russia==6);
    const TCHAR* CrossIds[]={TEXT("cross_roll_call"),TEXT("cross_both_sides"),TEXT("cross_whole_section"),TEXT("cross_guns_and_men")};bool CrossStable=true;
    for(int32 Feat=0;Feat<CrossFactionCountV83;++Feat){const auto& D=Catalog[CrossFactionStartV83+Feat];Ids.Add(D.Id);CrossStable&=FString(D.Id)==CrossIds[Feat]&&D.Nation==EFRRUNationV81::Invalid;}
    Check(TEXT("v83_twenty_eight_unique_ids_append_four_stable_cross_army_challenges"),Ids.Num()==TributeStartV836&&CrossStable&&HomewardIndexV82==11);
    const TCHAR* TributeIds[]={TEXT("hearts_homeward"),TEXT("all_quiet_at_last")};bool TributeStable=true;
    for(int32 Feat=0;Feat<UE_ARRAY_COUNT(TributeIds);++Feat)
    {const auto& D=Catalog[TributeStartV836+Feat];Ids.Add(D.Id);TributeStable&=FString(D.Id)==TributeIds[Feat]&&D.Nation==EFRRUNationV81::Invalid;}
    Check(TEXT("v836_thirty_unique_ids_preserve_original_twenty_eight_and_append_two_tributes"),Ids.Num()==Count&&TributeStable);
    Check(TEXT("v836_tribute_indices_are_not_cross_nation_array_offsets"),RequiredFeatNationMaskV83(TributeStartV836)==0&&RequiredFeatNationMaskV83(TributeStartV836+1)==0);
    Check(TEXT("v82_four_distinct_player_campaign_route_bits"),RouteBitV82(EFRRUNationV81::British)==1&&RouteBitV82(EFRRUNationV81::German)==2&&RouteBitV82(EFRRUNationV81::French)==4&&RouteBitV82(EFRRUNationV81::Russian)==8&&RouteBitV82(EFRRUNationV81::AustroHungarian)==0&&RouteBitV82(EFRRUNationV81::Invalid)==0);
    RunNationalArmourTallyCodecChecksV87(Check);
    FAccount FourRoutes;
    Check(TEXT("v82_four_route_account_codec_accepts_completed_fresh_account"),Decode(TEXT("{\"schema\":2,\"earned\":2048,\"routes\":15,\"tainted_campaigns\":[],\"dates\":{}}"),FourRoutes)&&FourRoutes.Routes==AllCampaignRoutesV82);
    Check(TEXT("v82_two_routes_cannot_encode_four_route_homeward_award"),!Decode(TEXT("{\"schema\":2,\"earned\":2048,\"routes\":3,\"tainted_campaigns\":[],\"dates\":{}}"),FourRoutes));
    Check(TEXT("v82_non_player_route_bit_rejected"),!Decode(TEXT("{\"schema\":2,\"earned\":0,\"routes\":16,\"tainted_campaigns\":[],\"dates\":{}}"),FourRoutes));
    FAccount Invalid;Check(TEXT("v76_corrupt_account_rejected"),!Decode(TEXT("{}"),Invalid));
    Account=FAccount();Account.Loaded=true;Account.EarnedMask=1;Account.Dates.Add(Catalog[0].Id,TEXT("2026-10-01T00:00:00Z"));
    const FGuid Synthetic=FGuid::NewGuid();Account.Tainted.Add(Synthetic);const bool Wrote=Save();Account=FAccount();Load();
    Check(TEXT("v76_atomic_account_save_and_reload_preserve_earned_and_taint"),Wrote&&Account.EarnedMask==1&&Account.Tainted.Contains(Synthetic));
    {
        // Mutate serialized text in memory only; the guarded review's account
        // and both durable files retain their existing counters.
        FString Text;TSharedPtr<FJsonObject> Json;FAccount Four,Old,Bad;
        const bool Read=FFileHelper::LoadFileToString(Text,*AccountPath())&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json)&&Json.IsValid();
        if(Read)
        {
            auto Rows=Json->GetArrayField(TEXT("medal_stats_v8215"));
            const bool HasFour=Rows.Num()==4;
            if(HasFour)
            {
                Rows[2]->AsObject()->SetNumberField(TEXT("infantry"),23);Rows[3]->AsObject()->SetNumberField(TEXT("infantry"),47);
                Json->SetArrayField(TEXT("medal_stats_v8215"),Rows);FString Encoded;FJsonSerializer::Serialize(Json.ToSharedRef(),TJsonWriterFactory<>::Create(&Encoded));
                Check(TEXT("v87_national_medal_account_codec_four_independent_counters"),Decode(Encoded,Four)&&Four.Medals[0].Infantry==Account.Medals[0].Infantry&&Four.Medals[1].Infantry==Account.Medals[1].Infantry&&Four.Medals[2].Infantry==23&&Four.Medals[3].Infantry==47);
                Rows.SetNum(2);Json->SetArrayField(TEXT("medal_stats_v8215"),Rows);Json->SetNumberField(TEXT("schema"),8);Encoded.Empty();FJsonSerializer::Serialize(Json.ToSharedRef(),TJsonWriterFactory<>::Create(&Encoded));
                Check(TEXT("v87_old_two_country_medal_account_keeps_uk_de_without_forging_fr_ru"),Decode(Encoded,Old)&&Old.Medals[0].Infantry==Account.Medals[0].Infantry&&Old.Medals[1].Infantry==Account.Medals[1].Infantry&&Old.Medals[2].Infantry==0&&Old.Medals[3].Infantry==0);
                Json->SetNumberField(TEXT("schema"),9);Encoded.Empty();FJsonSerializer::Serialize(Json.ToSharedRef(),TJsonWriterFactory<>::Create(&Encoded));
                Check(TEXT("v87_four_country_schema_rejects_missing_national_rows"),!Decode(Encoded,Bad));
            }
            else Check(TEXT("v87_current_account_writer_emits_four_national_rows"),false);
        }
        else Check(TEXT("v87_current_account_codec_fixture_readable"),false);
    }
    const auto Current=Account;Account=FAccount();Account.Loaded=true;Account.EarnedMask=2;Save();
    Check(TEXT("v76_stale_snapshot_merge_preserves_other_honour_and_taint"),Account.EarnedMask==3&&Account.Tainted.Contains(Synthetic));
    Account=Current;Save();
    FFileHelper::SaveStringToFile(TEXT("{broken"),*AccountPath());Account=FAccount();Load();
    Check(TEXT("v76_bad_primary_recovers_matching_backup"),Account.Recovered&&!Account.Corrupt&&Account.EarnedMask==3&&Account.Tainted.Contains(Synthetic));
    Save();State(B).Campaign=FGuid::NewGuid();Check(TEXT("v76_new_campaign_keeps_account_honours"),Earned(0)&&Account.Tainted.Contains(Synthetic));
    FString Response;FrontlineDebugV37::Execute(B,TEXT("research on"),Response);FrontlineDebugV37::Execute(B,TEXT("research off"),Response);FrontlineDebugV37::Execute(B,TEXT("reset"),Response);
    const bool Tainted=C&&!C->bAchievementEligibleV76&&Account.Tainted.Contains(C->Id);Account=FAccount();Load();
    Check(TEXT("v76_disabled_research_cheat_cannot_restore_campaign_eligibility"),Tainted&&C&&Account.Tainted.Contains(C->Id)&&!FrontlineDebugV37::HasCheatState(B));
    RunIsolatedChecks(B,Check);
    RunNationalIsolatedChecksV82(B,Check);
}

#include "WarfareFrontlineAchievementsReviewV82.inl"

bool SeedHonoursPresentationV837(AWarfareFrontlineGameMode* B,EFrontlineFaction Faction)
{
    FString Dir;
    if(!B||!B->IsProgressionVolatile()||!B->GetWorld()||!B->GetWorld()->IsGameWorld()
        ||uint8(Faction)>uint8(EFrontlineFaction::German)||WarfareFRRUCampaignV82::Active(B)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV837Review"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!DisposableAccount()||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)
        ||Dir.IsEmpty()||FPaths::IsRelative(Dir))return false;
    const FString Expected=FPaths::Combine(Dir,TEXT("AccountReview/achievements-v2.json"));
    if(!FPaths::IsSamePath(AccountPath(),Expected))return false;
    // UI presentation uses the existing private caches, never a second store.
    // Deliberately do not Load, Save, MarkCheat, fabricate an encounter proof,
    // or run the ordinary award evaluator. This is not award acceptance.
    FAccount DisplayAccount;DisplayAccount.Loaded=true;
    const FGuid FixtureCampaign=FGuid::NewGuid();const FString Date=TEXT("2026-10-07T00:00:00Z");
    const auto Items=FrontlineMedalsV8215::NationIndices(EFRRUNationV81(uint8(Faction)));
    if(Items.Num()!=9)return false;
    for(int32 I:Items)
    {
        const auto& D=FrontlineMedalsV8215::Definition(I);
        const auto A=FrontlineMedalsV8215::CompleteCheatAwardV8218(I,FixtureCampaign,Date);
        if(!FrontlineMedalsV8215::ValidAward(I,A))return false;
        DisplayAccount.CheatMedalAwardsV8222.Add(D.Id,A);DisplayAccount.CheatMedalsV8218.Add(D.Id);
    }
    FArmourTallyLedgerV8220 DisplayTally;DisplayTally.Loaded=true;
    const int32 N=int32(Faction);DisplayTally.Totals[N]=5;
    for(int32 I=0;I<5;++I)DisplayTally.Receipts.Add(FGuid::NewGuid(),uint8(N));
    Account=MoveTemp(DisplayAccount);ArmourTallyLedgerV8220=MoveTemp(DisplayTally);
    // No receipt is published: the screenshot must not acquire live banners.
    NotificationsV81.Empty();MedalNotificationsV8217.Empty();ArmourKillNotificationsV8220.Empty();
    PendingArmourTallyV8220.Empty();SettlementsV81.Empty();NextRetryV81=ArmourTallyRetryV8220=0;
    return FrontlineArmourValourV8221::Display(EFRRUNationV81(N),5).Distinctions==2;
}

void RunAllQuietChecksV8310(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B)&&!WarfareFRRUCampaignV82::Active(B)&&B->GetCurrentCampaign();
    Check(TEXT("v844_quiet_criterion_strict_disposable_ukde"),Safe);if(!Safe)return;
    const auto Units=B->Units;const auto Squads=B->Squads;const auto Records=B->CombatRecordV65[0];
    const auto EncounterStates=Encounters;const auto Phase=B->Phase;
    FFrontlineAchievementReviewV76 CampaignScope(B);TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Actors;
    ON_SCOPE_EXIT
    {for(const auto& A:Actors)if(A.IsValid())A->Destroy();B->Units=Units;B->Squads=Squads;B->CombatRecordV65[0]=Records;B->Phase=Phase;Encounters=EncounterStates;};
    B->Units.Reset();B->Squads.Reset();CampaignScope.Campaign(EFrontlineFaction::British);
    CampaignScope.Run->Difficulty=EFrontlineDifficulty::Survivor;B->Phase=EFrontlinePhase::Victory;
    FEncounter Probe;
    const auto Make=[&](int32 Id,EFrontlineRole Role)
    {
        FFrontlineSquad Q;Q.Id=Id;Q.Team=0;Q.Role=Role;
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
        if(U){Actors.Add(U);U->Initialize(B,Id,0,Role,0);U->SetActorTickEnabled(false);U->SetActorHiddenInGame(true);
            U->SetActorEnableCollision(false);U->bFieldEntered=true;B->Units.Add(U);Q.Members.Add(U);}
        B->Squads.Add(Q);Probe.OriginalSectionSizesV84.Add(Id,Q.Members.Num());return U;
    };
    auto* Rifle=Make(8440001,EFrontlineRole::Rifle);auto* Gun=Make(8440002,EFrontlineRole::Lewis);
    auto* Pioneer=Make(8440003,EFrontlineRole::Pioneer);auto* LaterRifle=Make(8440004,EFrontlineRole::Rifle);
    Check(TEXT("v844_quiet_actual_original_single_member_fixtures"),Rifle&&Gun&&Pioneer&&LaterRifle);if(!Rifle||!Gun||!Pioneer||!LaterRifle)return;
    const float Health=Rifle->Health;const int32 Direction=B->GetTeamDirection(0);
    const float Outside=-Direction*(FrontlinePacingV24::CombatHalfLength()+100.f);
    Check(TEXT("v844_quiet_three_categories_can_return_without_shot_or_kill_quotas"),AllQuietCriterionV8310(B,Probe)&&QuietReturnV8311(B,Probe).ClassesV844==3);
    Pioneer->Health=0;
    Check(TEXT("v844_quiet_two_categories_and_duplicate_rifles_cannot_make_three"),!AllQuietCriterionV8310(B,Probe)&&QuietReturnV8311(B,Probe).ClassesV844==2);
    Pioneer->Health=Health;Pioneer->bFieldEntered=false;Pioneer->SetActorLocation(FVector(Outside,0,88));
    Check(TEXT("v844_quiet_live_pending_third_category_cannot_qualify"),!AllQuietCriterionV8310(B,Probe));
    Pioneer->SetActorLocation(FVector(0,0,88));
    Check(TEXT("v844_quiet_actual_live_crossing_counts_before_own_tick"),AllQuietCriterionV8310(B,Probe));
    Pioneer->bFieldEntered=true;Rifle->Health=0;Probe.FirstSectionsV84.Add(int32(EFrontlineRole::Rifle),Rifle->SquadId);
    Check(TEXT("v844_quiet_later_section_can_supply_its_class_without_first_section_gate"),AllQuietCriterionV8310(B,Probe));
    const auto Member=B->Squads[2].Members.Pop();
    Check(TEXT("v844_quiet_reduced_roster_cannot_forge_the_third_class"),!AllQuietCriterionV8310(B,Probe));B->Squads[2].Members.Add(Member);
    Gun->Health=0;Check(TEXT("v844_quiet_dead_gunner_does_not_supply_a_live_class"),!AllQuietCriterionV8310(B,Probe));Gun->Health=Health;
    Check(TEXT("v844_single_member_classes_are_legal_without_hidden_two_member_minimum"),AllQuietCriterionV8310(B,Probe));
    B->CombatRecordV65[0].InfantryLost=90;
    Check(TEXT("v844_quiet_other_casualties_do_not_add_a_no_loss_gate"),AllQuietCriterionV8310(B,Probe));
    CampaignScope.Run->Difficulty=EFrontlineDifficulty::Veteran;
    Check(TEXT("v844_quiet_veteran_does_not_replace_highest_difficulty"),!AllQuietCriterionV8310(B,Probe));CampaignScope.Run->Difficulty=EFrontlineDifficulty::Survivor;
    const int32 Final=B->GetCampaignMissions().Last().Id;FFrontlineAchievementReviewV76::ArchiveBindV8215(B,Final-1);
    Check(TEXT("v844_quiet_penultimate_battle_does_not_replace_final"),!AllQuietCriterionV8310(B,Probe));FFrontlineAchievementReviewV76::ArchiveBindV8215(B,Final);
    B->Phase=EFrontlinePhase::Defeat;Check(TEXT("v844_quiet_defeat_never_qualifies"),!AllQuietCriterionV8310(B,Probe));B->Phase=EFrontlinePhase::Victory;
    Probe.OriginalSectionSizesV84.Reset();Check(TEXT("v844_quiet_missing_infantry_rosters_cannot_substitute_armour"),!AllQuietCriterionV8310(B,Probe));
    Check(TEXT("v844_only_two_tributes_have_the_single_special_honour_marker"),SpecialHonourV844(28)&&SpecialHonourV844(29)&&!SpecialHonourV844(27)&&!SpecialHonourV844(-1));
}

void RunBalanceChecksV842(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B)&&!WarfareFRRUCampaignV82::Active(B)&&B->GetCurrentCampaign();
    Check(TEXT("v842_honour_balance_strict_disposable_ukde"),Safe);if(!Safe)return;
    using FrontlineMedalsV8215::PreservedForceV842;
    Check(TEXT("v842_force_exact_quarter_loss_allows_twenty_four_returns"),PreservedForceV842(32,24));
    Check(TEXT("v842_force_more_than_quarter_loss_does_not_qualify"),!PreservedForceV842(33,24));
    Check(TEXT("v842_force_needs_twenty_four_actual_returns"),!PreservedForceV842(24,23));
    Check(TEXT("v842_force_zero_default_and_invalid_denominator_do_not_qualify"),!PreservedForceV842(0,24)&&!PreservedForceV842(24,25));
    Check(TEXT("v842_force_large_career_values_do_not_overflow"),PreservedForceV842(1000000000,750000000));
    FCrossBattleProofV83 Cross;Cross.Nation=Cross.StartedNation=EFRRUNationV81::British;
    Cross.Date=Cross.StartedDate=19171120;Cross.Difficulty=Cross.StartedDifficulty=1;
    Cross.CampaignMatches=Cross.Eligible=Cross.Saved=Cross.Won=true;Cross.EnteredInfantryV842=32;Cross.Alive=24;Cross.Lost=8;
    Check(TEXT("v842_cross_roll_call_production_evaluator_accepts_proven_quarter_loss"),(CrossBattleFeatsV83(Cross)&1u)!=0);
    Cross.EnteredInfantryV842=33;
    Check(TEXT("v842_cross_roll_call_production_evaluator_blocks_excess_loss"),(CrossBattleFeatsV83(Cross)&1u)==0);
    Cross.EnteredInfantryV842=32;Cross.Saved=false;
    Check(TEXT("v842_cross_roll_call_unsaved_victory_cannot_add_progress"),(CrossBattleFeatsV83(Cross)&1u)==0);
    FrontlineMedalsV8215::FStats Stats;Stats.Infantry=1200;Stats.Armour=12;
    FrontlineMedalsV8215::FProof Proof;Proof.Won=true;Proof.Nation=1;Proof.Date=19171120;Proof.Difficulty=1;
    Proof.Campaign=FGuid::NewGuid();Proof.EnteredInfantryV842=32;Proof.ReturnedInfantryV842=24;Proof.Losses=8;Proof.Armour=2;
    TMap<FString,FrontlineMedalsV8215::FAward> Awards;FrontlineMedalsV8215::ApplyAwards(Awards,Stats,Proof);
    const auto Silver=Awards.FindRef(TEXT("de.wurttemberg_merit"));
    Check(TEXT("v842_wurttemberg_accepts_eight_losses_with_proven_preservation"),Silver.Grade==1);
    Stats.Infantry=5000;FrontlineMedalsV8215::ApplyAwards(Awards,Stats,Proof);
    Check(TEXT("v842_wurttemberg_advances_to_existing_real_gold_class"),Awards.FindRef(TEXT("de.wurttemberg_merit")).Grade==2);
    auto EmptyProof=Proof;EmptyProof.EnteredInfantryV842=EmptyProof.ReturnedInfantryV842=0;Awards.Empty();
    FrontlineMedalsV8215::ApplyAwards(Awards,Stats,EmptyProof);
    Check(TEXT("v842_wurttemberg_legacy_no_losses_does_not_forge_roster_evidence"),!Awards.Contains(TEXT("de.wurttemberg_merit")));
    Stats.Infantry=0;Stats.Armour=11;Awards.Empty();FrontlineMedalsV8215::ApplyAwards(Awards,Stats,Proof);
    Check(TEXT("v842_bavarian_below_first_target_stays_locked"),!Awards.Contains(TEXT("de.bavarian_merit_order")));
    const int32 Targets[]={12,24,40,65,100,150};Awards.Empty();bool Sequential=true;
    for(int32 I=0;I<UE_ARRAY_COUNT(Targets);++I)
    {Stats.Armour=Targets[I];FrontlineMedalsV8215::ApplyAwards(Awards,Stats,Proof);const auto Award=Awards.FindRef(TEXT("de.bavarian_merit_order"));Sequential&=Award.Grade==I+1&&FrontlineMedalsV8215::ValidAward(20,Award);}
    Check(TEXT("v842_bavarian_six_real_classes_use_matching_reachable_targets"),Sequential);
    Proof.Won=false;Awards.Empty();FrontlineMedalsV8215::ApplyAwards(Awards,Stats,Proof);
    Check(TEXT("v842_honour_changes_do_not_award_on_defeat"),Awards.IsEmpty());

    const auto Units=B->Units;const auto Squads=B->Squads;const auto EncountersBefore=Encounters;const auto Phase=B->Phase;
    const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];
    const float Damage0=B->DamageDealt[0],Damage1=B->DamageDealt[1],FriendlyDamage=B->FriendlyFireDamage,PeakPressure=B->PeakSuppression;
    TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Actors;
    ON_SCOPE_EXIT{for(const auto& Actor:Actors)if(Actor.IsValid())Actor->Destroy();B->Units=Units;B->Squads=Squads;B->Phase=Phase;Encounters=EncountersBefore;
        B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->DamageDealt[0]=Damage0;B->DamageDealt[1]=Damage1;B->FriendlyFireDamage=FriendlyDamage;B->PeakSuppression=PeakPressure;};
    B->Units.Reset();B->Squads.Reset();B->Phase=EFrontlinePhase::Fighting;
    auto& S=State(B);S=FEncounter();S.Id=B->GetEncounterId();S.Started=true;
    FFrontlineSquad Q;Q.Team=0;Q.Id=8420001;Q.Role=EFrontlineRole::Pioneer;
    const int32 Direction=B->GetTeamDirection(0);const float Entry=-Direction*FrontlinePacingV24::CombatHalfLength();
    const auto Place=[&](AWarfareFrontlineSoldier* U,float X){U->SetActorLocation(FVector(X,0,AWarfareFrontlineGround::HeightAt(X,0)+88.f));};
    for(int32 I=0;I<6;++I)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)continue;
        Actors.Add(U);U->Initialize(B,Q.Id,0,Q.Role,I);U->SetActorTickEnabled(false);U->SetActorHiddenInGame(true);U->SetActorEnableCollision(false);
        U->bFieldEntered=I<4;if(I>=4)Place(U,Entry-Direction*(40.f+I*10.f));B->Units.Add(U);Q.Members.Add(U);
    }
    B->Squads.Add(Q);Recruited(B,int32(EFrontlineRole::Pioneer),0);TrackEnteredInfantryV842(B,S);
    Check(TEXT("v842_reserves_are_not_in_entered_roster"),Actors.Num()==6&&S.EnteredInfantryV842.Num()==4&&ReturnedInfantryV842(B,S)==4);
    if(Actors.Num()!=6)return;
    Actors[0]->Health=0;TrackEnteredInfantryV842(B,S);
    Check(TEXT("v842_casualty_keeps_original_denominator"),S.EnteredInfantryV842.Num()==4&&ReturnedInfantryV842(B,S)==3);
    B->Units.Remove(Actors[0].Get());TrackEnteredInfantryV842(B,S);
    Check(TEXT("v842_released_corpse_does_not_rewrite_participation"),S.EnteredInfantryV842.Num()==4&&ReturnedInfantryV842(B,S)==3);
    auto* FirstFrame=Actors[4].Get();Place(FirstFrame,Entry+Direction*20.f);
    Check(TEXT("v842_crossed_member_can_receive_fire_before_own_entry_tick"),!FirstFrame->bFieldEntered&&FirstFrame->IsActiveCombatant());
    TrackEnteredInfantryV842(B,S);
    Check(TEXT("v842_live_crossing_counts_even_when_action_defers_entry_flag"),!FirstFrame->bFieldEntered
        &&S.EnteredInfantryV842.Num()==5&&ReturnedInfantryV842(B,S)==4);
    FirstFrame->ReceiveHit(100.f,0.f,1,FVector::ForwardVector,EFrontlineDamageKind::Bullet,.01f,FFrontlineKillSourceV65());
    TrackEnteredInfantryV842(B,S);
    Check(TEXT("v842_first_frame_kia_keeps_frozen_living_crossing"),!FirstFrame->IsAlive()&&FirstFrame->bFieldEntered
        &&S.EnteredInfantryV842.Contains(FIntPoint(Q.Id,4))&&S.EnteredInfantryV842.Num()==5&&ReturnedInfantryV842(B,S)==3);
    auto* Reserve=Actors[5].Get();Place(Reserve,Entry-Direction*20.f);
    TrackEnteredInfantryV842(B,S);
    Check(TEXT("v842_live_unentered_reserve_cannot_improve_survival"),Reserve->IsAlive()&&!Reserve->bFieldEntered
        &&!S.EnteredInfantryV842.Contains(FIntPoint(Q.Id,5))&&ReturnedInfantryV842(B,S)==3);
    Reserve->ReceiveHit(100.f,0.f,1,FVector::ForwardVector,EFrontlineDamageKind::Bullet,.01f,FFrontlineKillSourceV65());
    TrackEnteredInfantryV842(B,S);
    Check(TEXT("v842_unentered_reserve_cannot_receive_field_fire_or_count"),Reserve->IsAlive()&&!Reserve->bFieldEntered
        &&!S.EnteredInfantryV842.Contains(FIntPoint(Q.Id,5))&&S.EnteredInfantryV842.Num()==5);
    Reserve->Health=0.f;Place(Reserve,Entry+Direction*100.f);TrackEnteredInfantryV842(B,S);
    Check(TEXT("v842_later_corpse_position_cannot_manufacture_entry"),!S.EnteredInfantryV842.Contains(FIntPoint(Q.Id,5))&&S.EnteredInfantryV842.Num()==5);
    ObstacleCleared(B,0,FVector(0,0,0),Q.Id);ObstacleCleared(B,0,FVector(0,0,0),Q.Id);
    Check(TEXT("v842_duplicate_clearance_is_not_second_deed"),!PioneerWorkCompletedV842(B,S));
    ObstacleCleared(B,0,FVector(100,0,0),Q.Id);
    Check(TEXT("v842_two_actual_attributed_clearances_qualify_work"),PioneerWorkCompletedV842(B,S));
    for(const auto& Actor:Actors)if(Actor.IsValid())Actor->Health=0;
    Check(TEXT("v842_later_casualties_do_not_erase_completed_field_work"),PioneerWorkCompletedV842(B,S));
    S.PioneerClearancesV837.Reset();S.Obstacles.Reset();
    ObstacleCleared(B,0,FVector(200,0,0));ObstacleCleared(B,0,FVector(300,0,0));
    Check(TEXT("v842_unattributed_destruction_does_not_forge_engineer_work"),!PioneerWorkCompletedV842(B,S));
    S.GasEnemy=S.GasFriendly=0;
    const auto PlayerGas=FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::Gas));
    const auto EnemyGas=FFrontlineKillSourceV65::FireSupport(1,int32(EFrontlineSupport::Gas));
    for(int32 I=0;I<10;++I)Loss(B,1,PlayerGas,false);
    Loss(B,0,EnemyGas,false);
    Check(TEXT("v842_enemy_gas_death_does_not_block_player_safe_gas_deed"),S.GasEnemy>=10&&S.GasFriendly==0);
    Loss(B,0,PlayerGas,false);
    Check(TEXT("v842_player_gas_friendly_death_blocks_safe_gas_deed"),S.GasEnemy>=10&&S.GasFriendly==1);
}

void RunIsolatedChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    // This is a bounded event-pipeline fixture, not a natural campaign replay.
    // Its only eligibility exception is scoped to this volatile battle and its
    // absolute review UserDir; normal review/cheat games still cannot earn honours.
    const bool AllowedV836=B&&B->IsProgressionVolatile()&&DisposableAccount();
    Check(TEXT("v836_actual_achievement_fixture_requires_disposable_account"),AllowedV836);if(!AllowedV836)return;
    auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());
    const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
    Check(TEXT("v76_award_fixture_has_campaign_and_zero_prior_armour"),Run&&(!Fleet||Fleet->GetDeploymentCountV76(0)==0));
    if(!Run||(Fleet&&Fleet->GetDeploymentCountV76(0)!=0))return;
    FTransientReviewV81 NotificationScope;

    const FString Path=AccountPath(),Backup=Path+TEXT(".bak");
    const bool HadPrimary=IFileManager::Get().FileExists(*Path),HadBackup=IFileManager::Get().FileExists(*Backup);
    TArray<uint8> PrimaryBytes,BackupBytes;
    const bool SnapshotOK=(!HadPrimary||FFileHelper::LoadFileToArray(PrimaryBytes,*Path))&&(!HadBackup||FFileHelper::LoadFileToArray(BackupBytes,*Backup));
    Check(TEXT("v76_award_fixture_snapshots_only_disposable_files"),SnapshotOK);if(!SnapshotOK)return;
    const FAccount BeforeAccount=Account;const FEncounter BeforeEncounter=State(B);
    const auto BeforeAwardBattle=IsolatedAwardBattle;
    const auto BeforeSilentReview=SilentAwardReviewBattleV836;SilentAwardReviewBattleV836=B;
    const auto BeforeUnits=B->Units;const auto BeforeSquads=B->Squads;const auto BeforeSmoke=B->SmokeZones;
    const auto BeforePhase=B->Phase;const int32 BeforeOutro=B->BattleOutroWinner;
    const FString BeforeReason=B->EndReason;const float BeforeElapsed=B->Elapsed,BeforeMorale[2]={B->Morale[0],B->Morale[1]};
    const FFrontlineCombatRecordV65 BeforeCombat[2]={B->CombatRecordV65[0],B->CombatRecordV65[1]};
    FFrontlineAchievementReviewV76 CampaignScope(B);
    TArray<TWeakObjectPtr<AActor>> Actors;
    auto Retire=[&]()
    {
        for(const auto& A:Actors)if(A.IsValid())A->Destroy();Actors.Empty();
        B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();
    };
    ON_SCOPE_EXIT
    {
        SilentAwardReviewBattleV836=BeforeSilentReview;IsolatedAwardBattle=BeforeAwardBattle;Retire();
        B->Units=BeforeUnits;B->Squads=BeforeSquads;B->SmokeZones=BeforeSmoke;B->Phase=BeforePhase;
        B->BattleOutroWinner=BeforeOutro;B->EndReason=BeforeReason;B->Elapsed=BeforeElapsed;
        for(int32 I=0;I<2;++I){B->Morale[I]=BeforeMorale[I];B->CombatRecordV65[I]=BeforeCombat[I];}
        Account=BeforeAccount;Encounters.Add(B,BeforeEncounter);
        if(HadPrimary)FFileHelper::SaveArrayToFile(PrimaryBytes,*Path);else IFileManager::Get().Delete(*Path);
        if(HadBackup)FFileHelper::SaveArrayToFile(BackupBytes,*Backup);else IFileManager::Get().Delete(*Backup);
        TArray<uint8> RestoredPrimary,RestoredBackup;
        const bool PrimaryRestored=HadPrimary?FFileHelper::LoadFileToArray(RestoredPrimary,*Path)&&RestoredPrimary==PrimaryBytes:!IFileManager::Get().FileExists(*Path);
        const bool BackupRestored=HadBackup?FFileHelper::LoadFileToArray(RestoredBackup,*Backup)&&RestoredBackup==BackupBytes:!IFileManager::Get().FileExists(*Backup);
        Check(TEXT("v76_award_fixture_restores_disposable_account_bytes"),PrimaryRestored&&BackupRestored&&IsolatedAwardBattle==BeforeAwardBattle);
    };
    // Detach the held review scene; never destroy its actors.
    B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();
    auto EmptyFiles=[&]()
    {IFileManager::Get().Delete(*Path);IFileManager::Get().Delete(*Backup);Account=FAccount();};
    RunAccountSchemaChecksV836(B,Check);
    // Criteria change only future settlements. Existing account bit positions
    // and date keys remain authoritative; no player record is re-qualified.
    EmptyFiles();Account.Loaded=true;Account.EarnedMask=(1u<<TributeStartV836)|(1u<<(TributeStartV836+1));
    const FString PriorDate=TEXT("2026-10-07T01:02:03Z");
    Account.Dates.Add(Catalog[TributeStartV836].Id,PriorDate);Account.Dates.Add(Catalog[TributeStartV836+1].Id,PriorDate);
    const bool PriorSaved=Save();Account=FAccount();Load();
    Check(TEXT("v837_changed_tribute_rules_preserve_both_existing_earned_bits_and_dates"),PriorSaved
        &&Earned(TributeStartV836)&&Earned(TributeStartV836+1)
        &&Account.Dates.FindRef(Catalog[TributeStartV836].Id)==PriorDate&&Account.Dates.FindRef(Catalog[TributeStartV836+1].Id)==PriorDate);
    EmptyFiles();
    auto Begin=[&](EFrontlineFaction Faction=EFrontlineFaction::British,bool KeepRecords=false)
    {
        Retire();if(!KeepRecords){EmptyFiles();SettlementsV81.Empty();RetryTaintedV81.Empty();NotificationsV81.Empty();NextRetryV81=0;}CampaignScope.Campaign(Faction);
        B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=10;B->EndReason=TEXT("我军已突破敌军端线。");
        for(int32 I=0;I<2;++I){B->Morale[I]=FrontlineMoraleV36::StartingMaximum(B,I);B->CombatRecordV65[I]={};}
        IsolatedAwardBattle=B;ResetEncounter(B);BeginEncounter(B);
    };
    auto Squad=[&](EFrontlineRole Role,int32 Members=2)
    {
        const int32 Id=760100+B->Squads.Num();FFrontlineSquad Q;Q.Id=Id;Q.Team=0;Q.Role=Role;
        for(int32 I=0;I<Members;++I)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)continue;
            Actors.Add(U);U->Initialize(B,Id,0,Role,I);U->SetActorTickEnabled(false);U->SetActorHiddenInGame(true);
            U->SetActorLocation(FVector(-400.f+I*20.f,100.f+B->Squads.Num()*140.f,3088.f));U->bFieldEntered=true;
            B->Units.Add(U);Q.Members.Add(U);
        }
        B->Squads.Add(Q);return Id;
    };
    auto Member=[&](int32 Id,int32 Index=0)->AWarfareFrontlineSoldier*
    {const auto* Q=B->FindSquad(Id);return Q&&Q->Members.IsValidIndex(Index)?Q->Members[Index].Get():nullptr;};
    auto InfantryLosses=[&](int32 LossCount,const FFrontlineKillSourceV65& Source,int32 Team=1)
    {for(int32 I=0;I<LossCount;++I)FrontlineCombatRecordV65::InfantryLost(B,Team,Source);};
    auto Outcome=[&](int32 Id,const TCHAR* Name,bool Expected,int32 Winner=0)
    {
        B->Phase=Winner==0?EFrontlinePhase::Victory:EFrontlinePhase::Defeat;Finish(B,Winner);
        bool Result=Earned(Id)==Expected;
        if(Expected){FAccount Disk;Result&=ReadFile(Path,Disk)&&(Disk.EarnedMask&(1u<<Id))!=0;}
        Check(Name,Result);
    };
    auto Wire=[&](int32 Cleared=3,int32 Lost=0)
    {for(int32 I=0;I<Cleared;++I)ObstacleCleared(B,0,FVector(I*100,0,0));B->CombatRecordV65[0].InfantryLost=Lost;};

    Begin();const int32 FirstSignals=Squad(EFrontlineRole::Signals,3);Recruited(B,int32(EFrontlineRole::Signals),0);
    const int32 LaterSignals=Squad(EFrontlineRole::Signals,3);Recruited(B,int32(EFrontlineRole::Signals),0);
    Check(TEXT("v84_real_recruit_hook_preserves_first_signals_id_and_original_size"),State(B).FirstSectionsV84.FindRef(int32(EFrontlineRole::Signals))==FirstSignals&&FirstSignals!=LaterSignals&&State(B).OriginalSectionSizesV84.FindRef(FirstSignals)==3);
    Begin();const int32 FirstEngineers=Squad(EFrontlineRole::Pioneer,4);Recruited(B,int32(EFrontlineRole::Pioneer),0);
    Squad(EFrontlineRole::Pioneer,4);Recruited(B,int32(EFrontlineRole::Pioneer),0);
    Check(TEXT("v84_real_recruit_hook_preserves_first_engineer_id_and_original_size"),State(B).FirstSectionsV84.FindRef(int32(EFrontlineRole::Pioneer))==FirstEngineers&&State(B).OriginalSectionSizesV84.FindRef(FirstEngineers)==4);

    // Missing, malformed and new-account states use the same reader as MergeDisk.
    EmptyFiles();Account.Loaded=true;Account.EarnedMask=1;Account.Routes=1;
    const FGuid BackupTaint=FGuid::NewGuid();Account.Tainted.Add(BackupTaint);Account.Dates.Add(Catalog[0].Id,TEXT("2026-10-01T00:00:00Z"));
    const bool Seeded=Save();IFileManager::Get().Delete(*Path);Account=FAccount();Load();
    Check(TEXT("v76_missing_primary_recovers_valid_backup_including_dates_and_taint"),Seeded&&Account.Recovered&&!Account.Corrupt&&Account.EarnedMask==1&&Account.Routes==1&&Account.Tainted.Contains(BackupTaint)&&Account.Dates.Contains(Catalog[0].Id));
    EmptyFiles();Load();Check(TEXT("v76_both_account_files_missing_start_empty_without_corruption"),Account.Loaded&&!Account.Corrupt&&!Account.Recovered&&Account.EarnedMask==0&&Account.Routes==0&&Account.Tainted.IsEmpty());
    FFileHelper::SaveStringToFile(TEXT("{broken-backup"),*Backup);Account=FAccount();Load();const bool InvalidSave=Save();FString Retained;FFileHelper::LoadFileToString(Retained,*Backup);
    Check(TEXT("v76_missing_primary_bad_backup_is_retained_and_cannot_be_overwritten"),Account.Corrupt&&!InvalidSave&&!IFileManager::Get().FileExists(*Path)&&Retained==TEXT("{broken-backup"));

    Begin();Wire(3,12);Outcome(0,TEXT("v84_three_clearances_breakthrough_unlocks"),true);
    Check(TEXT("v81_disposable_awards_do_not_queue_player_notifications"),NotificationsV81.IsEmpty());
    Begin();Wire(2);Outcome(0,TEXT("v84_two_clearances_do_not_complete_wire_crossing"),false);
    Begin();Wire(3,20);Outcome(0,TEXT("v84_wire_crossing_has_no_hidden_loss_quota"),true);
    Begin();Wire();B->EndReason=TEXT("敌军士气瓦解。");Outcome(0,TEXT("v76_wire_crossing_morale_victory_does_not_unlock"),false);
    Begin();Wire();Run->Difficulty=EFrontlineDifficulty::Rookie;Outcome(0,TEXT("v76_higher_difficulty_condition_rejects_rookie"),false);
    Begin();Wire();Outcome(0,TEXT("v76_qualified_conditions_do_not_unlock_on_defeat"),false,1);
    Begin();Wire();MarkCheat(B);BeginEncounter(B);Outcome(0,TEXT("v76_cheat_taint_survives_next_encounter_and_blocks_qualified_win"),false);
    Begin();Wire();IsolatedAwardBattle.Reset();Outcome(0,TEXT("v76_regular_automatic_review_still_cannot_award"),false);

    Begin();B->Morale[0]=FrontlineMoraleV36::StartingMaximum(B,0)*.09f;Tick(B,.01f);B->Morale[0]=FrontlineMoraleV36::StartingMaximum(B,0);Outcome(1,TEXT("v84_last_redoubt_records_ten_percent_morale_before_recovery"),true);
    Begin();B->Morale[0]=FrontlineMoraleV36::StartingMaximum(B,0)*.11f;Tick(B,.01f);Outcome(1,TEXT("v84_last_redoubt_rejects_above_ten_percent"),false);
    Begin();B->Morale[0]=FrontlineMoraleV36::StartingMaximum(B,0)*.1f;Tick(B,.01f);B->EndReason=TEXT("敌军士气瓦解。");Outcome(1,TEXT("v84_last_redoubt_still_requires_breakthrough"),false);

    auto Orders=[&](int32 Recipients,bool Duplicate)
    {
        const int32 Id=Squad(EFrontlineRole::Officer,1);auto* O=Member(Id);Check(TEXT("v76_officer_fixture_uses_commissioned_asset"),O&&O->IsCommissionedOfficer());
        int32 First=INDEX_NONE;for(int32 I=0;I<Recipients;++I){const int32 Recipient=Squad(EFrontlineRole::Rifle);if(I==0)First=Recipient;OfficerOrder(B,O,Recipient,1.5f);}
        if(Duplicate)OfficerOrder(B,O,First,1.5f);
    };
    Begin();Orders(6,false);Outcome(2,TEXT("v84_six_distinct_rallied_sections_and_surviving_officer_unlock"),true);
    Begin();Orders(5,false);Outcome(2,TEXT("v84_five_rallied_sections_remain_locked"),false);
    Begin();Orders(5,true);Outcome(2,TEXT("v84_repeated_recipient_cannot_substitute_sixth_section"),false);
    Begin();Orders(6,false);if(auto* O=Member(B->Squads[0].Id))O->Health=0;Outcome(2,TEXT("v84_officer_must_survive_the_rallied_sections"),false);
    Begin();{const int32 Id=Squad(EFrontlineRole::Officer,1);for(int32 I=0;I<6;++I)OfficerOrder(B,Member(Id),Squad(EFrontlineRole::Rifle),0);}Outcome(2,TEXT("v84_orders_without_actual_rally_relief_do_not_count"),false);

    auto GunKills=[&](int32 KillCount,bool GunnerDead)
    {
        const int32 Id=Squad(EFrontlineRole::Lewis);Recruited(B,int32(EFrontlineRole::Lewis),0);auto* U=Member(Id);
        InfantryLosses(KillCount,FFrontlineKillSourceV65::Soldier(U));if(GunnerDead&&U)U->Health=0;
    };
    Begin();GunKills(12,false);Outcome(3,TEXT("v851_one_section_twelve_actual_kill_receipts_unlock"),true);
    Begin();GunKills(11,false);Outcome(3,TEXT("v851_eleven_section_kills_remain_locked"),false);
    Begin();GunKills(12,false);if(auto* U=Member(B->Squads[0].Id,1))U->Health=0;Outcome(3,TEXT("v851_one_gun_has_no_hidden_companion_quota"),true);
    Begin();GunKills(12,true);Outcome(3,TEXT("v851_completed_gun_deed_survives_gunner_loss"),true);
    Begin();GunKills(12,false);Squad(EFrontlineRole::MobileGun);Recruited(B,int32(EFrontlineRole::MobileGun),0);Outcome(3,TEXT("v851_recruiting_another_gun_does_not_revoke_personal_deed"),true);
    Begin();GunKills(6,false);GunKills(6,false);Outcome(3,TEXT("v851_two_six_kill_sections_cannot_pool_the_deed"),false);
    Begin();GunKills(12,true);B->Squads.Reset();Outcome(3,TEXT("v851_actual_fatal_receipts_survive_section_cleanup"),true);
    Begin();GunKills(12,false);Run->Difficulty=EFrontlineDifficulty::Rookie;Outcome(3,TEXT("v851_one_gun_still_requires_higher_difficulty"),false);
    Begin();GunKills(12,false);Outcome(3,TEXT("v851_one_gun_still_requires_victory"),false,1);
    Begin();GunKills(12,false);MarkCheat(B);Outcome(3,TEXT("v851_one_gun_cheat_receipts_are_not_eligible"),false);

    auto Obstacles=[&](int32 ObstacleCount)
    {for(int32 I=0;I<ObstacleCount;++I)ObstacleCleared(B,0,FVector(100*I,200,0));};
    Begin();Obstacles(6);Outcome(4,TEXT("v84_six_real_clearances_unlock"),true);
    Begin();Obstacles(5);ObstacleCleared(B,0,FVector(0,200,0));Outcome(4,TEXT("v84_duplicate_clearance_cannot_substitute_sixth_obstacle"),false);

    auto Storm=[&](int32 Kills,bool RifleRecruited)
    {
        const int32 Id=Squad(EFrontlineRole::Mills);auto* U=Member(Id);Check(TEXT("v76_storm_fixture_uses_real_1918_storm_equipment"),U&&U->IsStormtrooper());
        if(RifleRecruited){Squad(EFrontlineRole::Rifle);Recruited(B,int32(EFrontlineRole::Rifle),0);}
        InfantryLosses(Kills,FFrontlineKillSourceV65::Soldier(U));
    };
    Begin(EFrontlineFaction::German);Storm(24,false);Outcome(5,TEXT("v84_twenty_four_real_storm_kill_events_unlock"),true);
    Begin(EFrontlineFaction::German);Storm(23,false);Outcome(5,TEXT("v84_twenty_three_storm_kills_remain_locked"),false);
    Begin(EFrontlineFaction::German);Storm(24,true);Outcome(5,TEXT("v84_storm_challenge_has_no_hidden_rifle_recruitment_ban"),true);

    Begin(EFrontlineFaction::German);{const int32 Id=Squad(EFrontlineRole::AntiTankRifle);for(int32 I=0;I<3;++I)FrontlineCombatRecordV65::ArmourLost(B,1,FFrontlineKillSourceV65::Soldier(Member(Id)));}Outcome(6,TEXT("v84415_model_free_statistics_do_not_substitute_heavy_tank_receipt"),false);
    Begin();for(int32 I=0;I<3;++I){const int32 Id=Squad(EFrontlineRole::AntiTankRifle);FrontlineCombatRecordV65::ArmourLost(B,1,FFrontlineKillSourceV65::Soldier(Member(Id)));}Outcome(6,TEXT("v84_anti_tank_sections_cannot_pool_required_armour_kills"),false);
    Begin();{const int32 Id=Squad(EFrontlineRole::AntiTankRifle);FrontlineCombatRecordV65::ArmourLost(B,1,FFrontlineKillSourceV65::Soldier(Member(Id)));}Outcome(6,TEXT("v84412_one_armoured_vehicle_remains_locked"),false);
    Begin();{const int32 Id=Squad(EFrontlineRole::Rifle);for(int32 I=0;I<3;++I)FrontlineCombatRecordV65::ArmourLost(B,1,FFrontlineKillSourceV65::Soldier(Member(Id)));}Outcome(6,TEXT("v76_other_weapon_armour_loss_cannot_complete_iron_answer"),false);

    auto SmokeTravel=[&](bool AllSections)
    {
        const int32 A=Squad(EFrontlineRole::Rifle),C=Squad(EFrontlineRole::Pioneer),D=Squad(EFrontlineRole::Mills);
        FFrontlineSmokeZone Zone;Zone.Team=0;Zone.Centre=FVector::ZeroVector;Zone.Radius=5000;Zone.StartedAt=0;Zone.ExpiresAt=1000;B->SmokeZones.Add(Zone);
        Tick(B,.25f);for(int32 Step=0;Step<12;++Step)
        {for(const int32 Id:{A,C,D})if(AllSections||Id==A)for(const auto& U:B->FindSquad(Id)->Members)if(U.IsValid())U->AddActorWorldOffset(FVector(100*B->GetTeamDirection(0),0,0));Tick(B,.25f);}
    };
    Begin();SmokeTravel(true);Outcome(7,TEXT("v84_three_sections_twelve_metres_in_friendly_smoke_unlock"),true);
    Begin();SmokeTravel(true);State(B).Smoke.FindChecked(B->Squads.Last().Id).Travel=1199.f;Outcome(7,TEXT("v84_one_incomplete_smoke_crossing_blocks_three_section_objective"),false);
    Begin();SmokeTravel(false);Outcome(7,TEXT("v76_one_smoke_crossing_squad_does_not_count_twice"),false);

    auto EngineerPassagesV837=[&](int32 Clearances,bool LaterInstead=false,bool Unattributed=false)
    {
        const int32 First=Squad(EFrontlineRole::Pioneer,4);Recruited(B,int32(EFrontlineRole::Pioneer),0);
        int32 Clearing=First;if(LaterInstead){Clearing=Squad(EFrontlineRole::Pioneer,4);Recruited(B,int32(EFrontlineRole::Pioneer),0);}
        for(int32 I=0;I<Clearances;++I)ObstacleCleared(B,0,FVector(100*I,200,0),Unattributed?INDEX_NONE:Clearing);
        return First;
    };
    for(const auto Nation:{EFrontlineFaction::British,EFrontlineFaction::German})
    {Begin(Nation);EngineerPassagesV837(2);Outcome(TributeStartV836,Nation==EFrontlineFaction::British?TEXT("v842_hearts_uk_two_personal_passages_saved"):TEXT("v842_hearts_de_two_personal_passages_saved"),true);}
    Begin();EngineerPassagesV837(0);Outcome(TributeStartV836,TEXT("v838_hearts_survival_without_real_clearance_remains_locked"),false);
    Begin();{const int32 First=EngineerPassagesV837(1);ObstacleCleared(B,0,FVector(0,200,0),First);Check(TEXT("v838_hearts_duplicate_does_not_inflate_personal_work"),State(B).PioneerClearancesV837.FindRef(First).Num()==1);}Outcome(TributeStartV836,TEXT("v842_hearts_duplicate_cannot_substitute_second_passage"),false);
    Begin();EngineerPassagesV837(2,true);Outcome(TributeStartV836,TEXT("v842_hearts_later_engineers_can_complete_their_own_work"),true);
    Begin();{const int32 First=EngineerPassagesV837(2);for(int32 I=0;I<4;++I)if(auto* U=Member(First,I))U->Health=0;}Outcome(TributeStartV836,TEXT("v842_hearts_losses_do_not_erase_two_completed_passages"),true);
    Begin();{const int32 Id=Squad(EFrontlineRole::Pioneer,1);Recruited(B,int32(EFrontlineRole::Pioneer),0);for(int32 I=0;I<2;++I)ObstacleCleared(B,0,FVector(100*I,0,0),Id);}Outcome(TributeStartV836,TEXT("v838_hearts_original_section_must_have_at_least_two_members"),false);
    Begin();{const int32 Id=Squad(EFrontlineRole::Rifle,4);Recruited(B,int32(EFrontlineRole::Rifle),0);for(int32 I=0;I<2;++I)ObstacleCleared(B,0,FVector(100*I,0,0),Id);}Outcome(TributeStartV836,TEXT("v838_hearts_wrong_role_cannot_claim_clearance"),false);
    Begin();{const int32 First=EngineerPassagesV837(1);const int32 Second=Squad(EFrontlineRole::Pioneer,4);Recruited(B,int32(EFrontlineRole::Pioneer),0);ObstacleCleared(B,0,FVector(100,200,0),Second);Check(TEXT("v842_hearts_each_section_has_only_one_passage"),State(B).PioneerClearancesV837.FindRef(First).Num()==1&&State(B).PioneerClearancesV837.FindRef(Second).Num()==1);}Outcome(TributeStartV836,TEXT("v842_hearts_cannot_pool_work_from_two_sections"),false);
    Begin();{const int32 Id=Squad(EFrontlineRole::Pioneer,4);Recruited(B,int32(EFrontlineRole::Pioneer),0);for(int32 I=0;I<4;++I)if(auto* U=Member(Id,I))U->bFieldEntered=false;for(int32 I=0;I<2;++I)ObstacleCleared(B,0,FVector(100*I,200,0),Id);}Outcome(TributeStartV836,TEXT("v842_hearts_pending_reinforcements_cannot_do_field_work"),false);
    Begin();EngineerPassagesV837(2,false,true);Outcome(TributeStartV836,TEXT("v838_hearts_unattributed_work_does_not_credit_any_section"),false);
    Begin();EngineerPassagesV837(2);Run->Difficulty=EFrontlineDifficulty::Rookie;Outcome(TributeStartV836,TEXT("v838_hearts_rookie_difficulty_does_not_unlock"),false);
    Begin();EngineerPassagesV837(2);Outcome(TributeStartV836,TEXT("v838_hearts_defeat_does_not_unlock"),false,1);
    Begin();EngineerPassagesV837(2);MarkCheat(B);Outcome(TributeStartV836,TEXT("v838_hearts_assisted_campaign_does_not_unlock"),false);
    Begin();{const FString OriginalDate=TEXT("2026-10-06T00:00:00Z");Account.EarnedMask|=1u<<TributeStartV836;Account.Dates.Add(Catalog[TributeStartV836].Id,OriginalDate);const bool Stored=Save();B->Phase=EFrontlinePhase::Victory;Finish(B,0);Account=FAccount();Load();Check(TEXT("v838_hearts_condition_revision_preserves_preexisting_earned_bit_and_date"),Stored&&Earned(TributeStartV836)&&Account.Dates.FindRef(Catalog[TributeStartV836].Id)==OriginalDate);}

    auto LastRollCallV836=[&](EFrontlineFaction Nation,int32 Men,EFrontlineDifficulty Difficulty,int32 Lost=0)
    {
        Begin(Nation);Run->Difficulty=Difficulty;ResetEncounter(B);BeginEncounter(B);
        const EFrontlineRole Classes[]={EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Pioneer};int32 NextClass=0;
        for(int32 Remaining=Men;Remaining>0;Remaining-=2)
        {const auto Role=Classes[(NextClass++)%UE_ARRAY_COUNT(Classes)];const int32 Id=Squad(Role,FMath::Min(Remaining,2));Recruited(B,int32(Role),0);}
        B->CombatRecordV65[0].InfantryLost=Lost;
    };
    for(const auto Nation:{EFrontlineFaction::British,EFrontlineFaction::German})
    {LastRollCallV836(Nation,6,EFrontlineDifficulty::Survivor);Outcome(TributeStartV836+1,Nation==EFrontlineFaction::British?TEXT("v837_quiet_uk_actual_last_mission_first_rifles_intact_saved"):TEXT("v837_quiet_de_actual_last_mission_first_rifles_intact_saved"),true);}
    LastRollCallV836(EFrontlineFaction::British,0,EFrontlineDifficulty::Survivor);Outcome(TributeStartV836+1,TEXT("v837_quiet_armour_only_cannot_replace_first_rifle_section"),false);
    LastRollCallV836(EFrontlineFaction::British,12,EFrontlineDifficulty::Survivor);if(auto* U=Member(B->Squads[0].Id))U->Health=0;
    Outcome(TributeStartV836+1,TEXT("v8311_quiet_later_fighting_section_with_own_survivors_qualifies"),true);
    LastRollCallV836(EFrontlineFaction::British,24,EFrontlineDifficulty::Veteran);Outcome(TributeStartV836+1,TEXT("v836_quiet_veteran_cannot_substitute_survivor"),false);
    LastRollCallV836(EFrontlineFaction::British,24,EFrontlineDifficulty::Survivor);
    Check(TEXT("v836_quiet_earlier_mission_fixture_binds_actual_penultimate_mission"),FFrontlineAchievementReviewV76::ArchiveBindV8215(B,18));
    Outcome(TributeStartV836+1,TEXT("v836_quiet_actual_earlier_mission_cannot_substitute_final_battle"),false);
    LastRollCallV836(EFrontlineFaction::British,24,EFrontlineDifficulty::Survivor,1);Outcome(TributeStartV836+1,TEXT("v8310_quiet_other_sections_may_pay_the_cost_of_cover"),true);
    LastRollCallV836(EFrontlineFaction::British,6,EFrontlineDifficulty::Survivor);for(int32 I=0;I<6;++I)if(auto* U=Member(B->Squads[0].Id,I))
    {U->bFieldEntered=false;U->SetActorLocation(FVector(-B->GetTeamDirection(0)*(FrontlinePacingV24::CombatHalfLength()+100.f),0,88));}
    Outcome(TributeStartV836+1,TEXT("v8311_quiet_pending_reinforcements_are_not_returned_survivors"),false);
    LastRollCallV836(EFrontlineFaction::British,6,EFrontlineDifficulty::Survivor);if(auto* Q=B->FindSquad(B->Squads[0].Id))Q->Members.Pop();
    Outcome(TributeStartV836+1,TEXT("v8310_quiet_roster_cannot_shrink_to_redefine_intact"),false);
    LastRollCallV836(EFrontlineFaction::British,24,EFrontlineDifficulty::Survivor);Outcome(TributeStartV836+1,TEXT("v836_quiet_defeat_cannot_complete_last_roll_call"),false,1);
    LastRollCallV836(EFrontlineFaction::British,24,EFrontlineDifficulty::Survivor);MarkCheat(B);Outcome(TributeStartV836+1,TEXT("v836_quiet_assisted_campaign_cannot_award_tribute"),false);

    Begin();InfantryLosses(10,FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::Gas)));Outcome(8,TEXT("v84_ten_enemy_gas_casualties_no_friendly_gas_death_unlock"),true);
    Begin();InfantryLosses(9,FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::Gas)));Outcome(8,TEXT("v84_nine_enemy_gas_casualties_remain_locked"),false);
    Begin();{const auto Source=FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::Gas));InfantryLosses(10,Source);InfantryLosses(1,Source,0);}Outcome(8,TEXT("v76_any_friendly_gas_death_blocks_safe_gas_achievement"),false);

    auto Battery=[&](int32 Id){auto Source=FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::Mortar));Source.FireMissionId=Id;return Source;};
    Begin();InfantryLosses(6,Battery(7601));InfantryLosses(6,Battery(7601));Outcome(9,TEXT("v84_one_fire_mission_twelve_kills_unlock"),true);
    Begin();InfantryLosses(11,Battery(7601));Outcome(9,TEXT("v84_eleven_single_mission_kills_remain_locked"),false);
    Begin();InfantryLosses(6,Battery(7601));InfantryLosses(6,Battery(7602));Outcome(9,TEXT("v76_different_fire_missions_cannot_pool_achievement_kills"),false);
    Begin();InfantryLosses(12,Battery(7601));InfantryLosses(1,Battery(7601),0);Outcome(9,TEXT("v76_friendly_casualty_in_any_mission_round_blocks_steel_storm"),false);

    auto Train=[&](int32 Serial)->AFrontlineTank*
    {
        auto* T=B->GetWorld()->SpawnActor<AFrontlineTank>();if(!T)return nullptr;Actors.Add(T);T->SetActorHiddenInGame(true);T->SetActorTickEnabled(false);
        if(!T->Configure(B,EFrontlineVehicle::A7V,0,Serial)||!T->ConfigureRailV32())return nullptr;return T;
    };
    auto RailChallenge=[&]()->AFrontlineTank*
    {
        auto* T=Train(7601);Check(TEXT("v81_rail_fixture_has_actual_convoy"),T&&T->IsRailConvoyV32()&&T->IsOperational());
        const int32 A=Squad(EFrontlineRole::Rifle),C=Squad(EFrontlineRole::Pioneer);
        RailSectionDeployed(B,T,int32(EFrontlineRole::Rifle),A);RailSectionDeployed(B,T,int32(EFrontlineRole::Pioneer),C);return T;
    };
    Begin(EFrontlineFaction::German);
    {auto* T=RailChallenge();if(T)T->ReceiveModuleHit(EFrontlineTankModule::Hull,T->GetHullIntegrity()*.25f);}Outcome(10,TEXT("v84_both_sections_same_train_seventy_five_percent_hull_unlock"),true);
    Begin(EFrontlineFaction::German);
    {auto* T=RailChallenge();if(T)T->ReceiveModuleHit(EFrontlineTankModule::Hull,T->GetHullIntegrity()*.251f);}Outcome(10,TEXT("v84_train_below_seventy_five_percent_hull_remains_locked"),false);
    Begin(EFrontlineFaction::German);RailChallenge();Run->Difficulty=EFrontlineDifficulty::Rookie;Outcome(10,TEXT("v81_train_challenge_rejects_rookie_even_when_other_conditions_met"),false);
    Begin(EFrontlineFaction::German);
    {auto* A=Train(7601);auto* C=Train(7602);const int32 Rifle=Squad(EFrontlineRole::Rifle),Pioneer=Squad(EFrontlineRole::Pioneer);RailSectionDeployed(B,A,int32(EFrontlineRole::Rifle),Rifle);RailSectionDeployed(B,C,int32(EFrontlineRole::Pioneer),Pioneer);}Outcome(10,TEXT("v76_two_different_trains_cannot_pool_carried_sections"),false);
    Begin();Run->CompletedMissions=B->GetCampaignMissions().Num();Outcome(11,TEXT("v82_one_completed_campaign_route_is_not_four"),false);
    CampaignScope.Campaign(EFrontlineFaction::German);Run->CompletedMissions=B->GetCampaignMissions().Num();B->Phase=EFrontlinePhase::Fighting;ResetEncounter(B);BeginEncounter(B);Outcome(11,TEXT("v82_british_german_pair_is_not_four_campaign_routes"),false);
    Check(TEXT("v82_pair_records_only_british_and_german_bits"),Account.Routes==3);
    // Codec/settlement fixture only: seed the two national route bits without
    // pretending these UK/DE actors have completed an FR/RU campaign.
    Begin();Account.Routes=12;Check(TEXT("v82_disposable_national_route_bits_save"),Save());
    Run->CompletedMissions=B->GetCampaignMissions().Num();Outcome(11,TEXT("v82_three_routes_leave_homeward_locked"),false);
    Begin(EFrontlineFaction::German,true);Run->CompletedMissions=B->GetCampaignMissions().Num();Outcome(11,TEXT("v82_four_saved_route_bits_unlock_homeward"),true);
    Begin();Run->CompletedMissions=B->GetCampaignMissions().Num()-1;Outcome(11,TEXT("v76_incomplete_campaign_route_does_not_record_completion"),false);

    // A failed write is kept pending, never displayed as an earned record.
    Begin();Wire();ReviewFailSaveV81=true;Outcome(0,TEXT("v81_failed_award_write_does_not_report_earned"),false);
    Check(TEXT("v81_failed_award_has_readable_notice_and_no_notification"),!Status(0,false).IsEmpty()&&Status(0,true).Contains(TEXT("重试"))&&NotificationsV81.IsEmpty());
    // Exercise the production notification branch inside this disposable fixture.
    if(auto* Pending=SettlementsV81.Find(Run->Id))Pending->NotifyMask=1;ReviewFailSaveV81=false;NextRetryV81=0;
    const int32 RetriedNotice=TakeNotificationV81();FAccount RetriedDisk;
    Check(TEXT("v81_retry_persists_before_emitting_one_notification"),RetriedNotice==0&&ReadFile(Path,RetriedDisk)&&(RetriedDisk.EarnedMask&1)&&RetriedDisk.Dates.Contains(Catalog[0].Id)&&!HasPendingV81());
    Check(TEXT("v81_successful_retry_does_not_repeat_notification"),TakeNotificationV81()==INDEX_NONE);

    Begin();ReviewFailSaveV81=true;MarkCheat(B);const FGuid AssistedId=Run->Id;
    Check(TEXT("v81_failed_cheat_taint_write_stays_ineligible_and_pending"),!Run->bAchievementEligibleV76&&Account.Tainted.Contains(AssistedId)&&RetryTaintedV81.Contains(AssistedId));
    ReviewFailSaveV81=false;NextRetryV81=0;const int32 TaintNotice=TakeNotificationV81();FAccount TaintedDisk;
    Check(TEXT("v81_retry_persists_cheat_taint_without_earned_banner"),TaintNotice==INDEX_NONE&&ReadFile(Path,TaintedDisk)&&TaintedDisk.Tainted.Contains(AssistedId)&&TaintedDisk.EarnedMask==0&&RetryTaintedV81.IsEmpty());

    Begin();Run->CompletedMissions=B->GetCampaignMissions().Num();ReviewFailSaveV81=true;Outcome(11,TEXT("v81_first_route_save_failure_does_not_complete_both"),false);
    Check(TEXT("v81_failed_route_save_restores_unpersisted_route_bits"),Account.Routes==0);
    ReviewFailSaveV81=false;Begin(EFrontlineFaction::German,true);Run->CompletedMissions=B->GetCampaignMissions().Num();Outcome(11,TEXT("v82_second_route_settles_first_without_four_route_award"),false);
    NextRetryV81=0;TakeNotificationV81();FAccount BothRoutesDisk;
    Check(TEXT("v82_deferred_route_pair_persists_without_homeward"),ReadFile(Path,BothRoutesDisk)&&BothRoutesDisk.Routes==3&&!(BothRoutesDisk.EarnedMask&(1u<<11))&&!HasPendingV81()&&NotificationsV81.IsEmpty());

    // Complete the primary replacement, then fault the backup mirror. The
    // retry merges an already earned bit11 but must still announce it once.
    Begin();Account.Routes=12;Check(TEXT("v82_route_retry_fixture_seeds_saved_national_bits"),Save());Run->CompletedMissions=B->GetCampaignMissions().Num();ReviewFailSaveV81=true;
    Outcome(11,TEXT("v81_partial_route_write_fixture_defers_first_route"),false);
    if(auto* Pending=SettlementsV81.Find(Run->Id))Pending->NotifyRoute=true;
    ReviewFailSaveV81=false;ReviewFailMirrorV81=true;const FGuid PartialSecondCampaign=FGuid::NewGuid();Run->Id=PartialSecondCampaign;Begin(EFrontlineFaction::German,true);
    Check(TEXT("v81_partial_route_fixture_selects_new_registered_campaign"),B->GetCurrentCampaign()==Run&&B->GetCurrentCampaign()->Id==PartialSecondCampaign&&B->GetCurrentCampaign()->Faction==EFrontlineFaction::German);
    Run->CompletedMissions=B->GetCampaignMissions().Num();Outcome(11,TEXT("v81_primary_only_route_pair_is_not_reported_before_complete_save"),false);
    FAccount PartialRouteDisk;
    Check(TEXT("v82_partial_four_route_write_commits_primary_without_earned_banner"),ReadFile(Path,PartialRouteDisk)&&PartialRouteDisk.Routes==AllCampaignRoutesV82&&(PartialRouteDisk.EarnedMask&(1u<<11))&&NotificationsV81.IsEmpty());
    ReviewFailMirrorV81=false;NextRetryV81=0;const int32 RouteRetryNotice=TakeNotificationV81();FAccount MirroredRouteDisk;
    Check(TEXT("v82_partial_four_route_write_retry_emits_one_banner"),RouteRetryNotice==11&&ReadFile(Backup,MirroredRouteDisk)&&MirroredRouteDisk.Routes==AllCampaignRoutesV82&&(MirroredRouteDisk.EarnedMask&(1u<<11))&&MirroredRouteDisk.Dates.FindRef(Catalog[11].Id)==PartialRouteDisk.Dates.FindRef(Catalog[11].Id)&&!HasPendingV81()&&TakeNotificationV81()==INDEX_NONE);

#if PLATFORM_WINDOWS
    // Two independent in-memory account snapshots share these real disposable
    // files. Instance B earns the pending bit before A can enter its mutex.
    Begin();Wire();ReviewFailLockV81=true;
    Outcome(0,TEXT("v81_second_instance_fixture_defers_first_instance_award"),false);
    if(auto* Pending=SettlementsV81.Find(Run->Id))Pending->NotifyMask=1;
    const FAccount FirstInstance=Account;ReviewFailLockV81=false;Account=FAccount();Load();Unlock(0);
    const FString SecondInstanceDate=TEXT("2026-10-02T00:00:00Z");Account.Dates.Add(Catalog[0].Id,SecondInstanceDate);
    const bool SecondInstanceSaved=Save();FAccount SecondInstanceDisk;
    Check(TEXT("v81_second_instance_saves_same_award_with_its_own_date"),SecondInstanceSaved&&ReadFile(Path,SecondInstanceDisk)&&SecondInstanceDisk.EarnedMask==1&&SecondInstanceDisk.Dates.FindRef(Catalog[0].Id)==SecondInstanceDate);
    Account=FirstInstance;NextRetryV81=0;const int32 FirstInstanceNotice=TakeNotificationV81();FAccount MergedInstancesDisk,MergedInstancesBackup;
    Check(TEXT("v81_second_instance_merge_preserves_saved_date_without_duplicate_banner"),FirstInstanceNotice==INDEX_NONE&&ReadFile(Path,MergedInstancesDisk)&&ReadFile(Backup,MergedInstancesBackup)&&MergedInstancesDisk.EarnedMask==1&&MergedInstancesBackup.EarnedMask==1&&MergedInstancesDisk.Dates.FindRef(Catalog[0].Id)==SecondInstanceDate&&MergedInstancesBackup.Dates.FindRef(Catalog[0].Id)==SecondInstanceDate&&Account.Dates.FindRef(Catalog[0].Id)==SecondInstanceDate&&NotificationsV81.IsEmpty());
    Check(TEXT("v81_second_instance_merge_drains_pending_without_repeated_notification"),!HasPendingV81()&&TakeNotificationV81()==INDEX_NONE&&EarnedCount()==1);

    // Fault the initial mutex entrance, before a disk read or a write occurs.
    // Start a different campaign and destroy the original surviving gun crew:
    // recovery must use the completed victory, not the replacement battlefield.
    Begin();GunKills(24,false);Run->CompletedMissions=B->GetCampaignMissions().Num();ReviewFailLockV81=true;
    Outcome(3,TEXT("v81_initial_mutex_failure_keeps_award_unearned"),false);
    const FGuid FinishedCampaign=Run->Id;
    const auto* FinishedSettlement=SettlementsV81.Find(FinishedCampaign);
    const FString FinishedDate=FinishedSettlement?FinishedSettlement->Dates.FindRef(Catalog[3].Id):FString();
    Check(TEXT("v84_mutex_fixture_captures_qualified_crew_award_before_replacement"),FinishedSettlement&&(FinishedSettlement->Mask&(1u<<3))&&!FinishedDate.IsEmpty());
    if(auto* Pending=SettlementsV81.Find(FinishedCampaign)){Pending->NotifyMask=1u<<3;Pending->NotifyRoute=true;}
    Check(TEXT("v81_initial_mutex_failure_has_retry_notice_without_disk_or_banner"),Status(3,true).Contains(TEXT("重试"))&&!IFileManager::Get().FileExists(*Path)&&NotificationsV81.IsEmpty());
    const FGuid ReplacementCampaign=FGuid::NewGuid();Run->Id=ReplacementCampaign;Begin(EFrontlineFaction::German,true);Run->Difficulty=EFrontlineDifficulty::Rookie;
    Check(TEXT("v81_replacement_battle_fixture_selects_new_registered_campaign"),ReplacementCampaign!=FinishedCampaign&&B->GetCurrentCampaign()==Run&&B->GetCurrentCampaign()->Id==ReplacementCampaign&&B->GetCurrentCampaign()->Faction==EFrontlineFaction::German&&B->GetCampaignDifficulty()==EFrontlineDifficulty::Rookie);
    Wire();InfantryLosses(6,FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::Gas)));
    Outcome(0,TEXT("v81_replacement_rookie_battle_cannot_add_qualified_awards"),false);
    NextRetryV81=0;const int32 StillBlocked=TakeNotificationV81();
    Check(TEXT("v81_persistent_mutex_failure_keeps_snapshot_unannounced"),StillBlocked==INDEX_NONE&&Account.EarnedMask==0&&Account.Routes==0&&!IFileManager::Get().FileExists(*Path));
    ReviewFailLockV81=false;NextRetryV81=0;const int32 RecoveredGunNotice=TakeNotificationV81();FAccount RecoveredGunDisk;
    Check(TEXT("v81_mutex_recovery_saves_original_crew_award_date_and_british_route"),RecoveredGunNotice==3&&ReadFile(Path,RecoveredGunDisk)&&RecoveredGunDisk.EarnedMask==(1u<<3)&&RecoveredGunDisk.Routes==1&&!FinishedDate.IsEmpty()&&RecoveredGunDisk.Dates.FindRef(Catalog[3].Id)==FinishedDate);
    Check(TEXT("v81_mutex_recovery_does_not_repeat_award_or_complete_other_route"),TakeNotificationV81()==INDEX_NONE&&!Earned(11)&&!HasPendingV81());
    // The German route completes later; the pair still cannot substitute for
    // the French and Russian completion bits.
    Begin(EFrontlineFaction::German,true);Run->CompletedMissions=B->GetCampaignMissions().Num();
    Outcome(11,TEXT("v82_deferred_and_later_completed_pair_remains_short_of_four"),false);

    // A repeated finish cannot turn a later-mutated encounter into a second
    // settlement. Separate incomplete victories also cannot pool clearances.
    Begin();Wire();ReviewFailLockV81=true;Outcome(0,TEXT("v81_duplicate_finish_fixture_starts_with_deferred_wire_award"),false);
    InfantryLosses(6,FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::Gas)));Finish(B,0);
    ReviewFailLockV81=false;NextRetryV81=0;TakeNotificationV81();FAccount DuplicateDisk;
    Check(TEXT("v81_repeated_finish_does_not_recompute_later_gas_kills"),ReadFile(Path,DuplicateDisk)&&DuplicateDisk.EarnedMask==1&&NotificationsV81.IsEmpty());
    Begin();const FGuid FirstIncompleteReceipt=B->GetEncounterId();Wire(1);ReviewFailLockV81=true;Outcome(0,TEXT("v81_first_incomplete_victory_cannot_queue_wire_award"),false);
    Begin(EFrontlineFaction::British,true);const FGuid SecondIncompleteReceipt=B->GetEncounterId();Wire(1);Outcome(0,TEXT("v81_second_incomplete_victory_cannot_pool_obstacle_counts"),false);
    ReviewFailLockV81=false;NextRetryV81=0;TakeNotificationV81();
    // Every completed battle legitimately writes its medal service receipt,
    // even when neither encounter qualifies for an achievement or a route.
    FAccount IncompleteDisk,IncompleteMirror;
    const bool IncompleteSaved=ReadFile(Path,IncompleteDisk)&&ReadFile(Backup,IncompleteMirror);
    Check(TEXT("v81_separate_incomplete_victories_leave_account_empty"),IncompleteSaved&&!Earned(0)
        &&Account.EarnedMask==0&&Account.CheatHonoursV8218==0&&Account.Routes==0
        &&IncompleteDisk.EarnedMask==0&&IncompleteMirror.EarnedMask==0&&IncompleteDisk.Routes==0&&IncompleteMirror.Routes==0);
    Check(TEXT("v836_separate_incomplete_victories_preserve_two_real_medal_receipts"),IncompleteSaved
        &&FirstIncompleteReceipt.IsValid()&&SecondIncompleteReceipt.IsValid()&&FirstIncompleteReceipt!=SecondIncompleteReceipt
        &&IncompleteDisk.MedalReceiptsV8215.Num()==2&&IncompleteMirror.MedalReceiptsV8215.Num()==2
        &&IncompleteDisk.MedalReceiptsV8215.Contains(FirstIncompleteReceipt)&&IncompleteDisk.MedalReceiptsV8215.Contains(SecondIncompleteReceipt)
        &&IncompleteMirror.MedalReceiptsV8215.Contains(FirstIncompleteReceipt)&&IncompleteMirror.MedalReceiptsV8215.Contains(SecondIncompleteReceipt)
        &&IncompleteDisk.Medals[0].Victories==2&&IncompleteMirror.Medals[0].Victories==2
        &&IncompleteDisk.Medals[1].Victories==0&&IncompleteMirror.Medals[1].Victories==0);

    // An independently saved taint discovered on the first successful merge
    // disqualifies this campaign's pending route/award while keeping old honours.
    Begin();Wire();Run->CompletedMissions=B->GetCampaignMissions().Num();ReviewFailLockV81=true;
    Outcome(0,TEXT("v81_external_taint_fixture_defers_award_and_route"),false);
    const FGuid ExternalTaintId=Run->Id;const FAccount BeforeExternalTaint=Account;
    ReviewFailLockV81=false;Account=FAccount();Account.Loaded=true;Account.EarnedMask=1u<<1;Account.Routes=2;
    Account.Dates.Add(Catalog[1].Id,TEXT("2026-09-30T00:00:00Z"));Account.Tainted.Add(ExternalTaintId);
    const bool ExternalTaintSaved=Save();Account=BeforeExternalTaint;NextRetryV81=0;TakeNotificationV81();FAccount ExternalTaintDisk;
    Check(TEXT("v81_disk_taint_blocks_deferred_award_and_route_preserves_old_honour"),ExternalTaintSaved&&ReadFile(Path,ExternalTaintDisk)&&ExternalTaintDisk.EarnedMask==(1u<<1)&&ExternalTaintDisk.Routes==2&&ExternalTaintDisk.Tainted.Contains(ExternalTaintId)&&ExternalTaintDisk.Dates.FindRef(Catalog[1].Id)==TEXT("2026-09-30T00:00:00Z")&&!HasPendingV81()&&NotificationsV81.IsEmpty());
#endif

    // Both bad files remain byte-for-byte intact across failed initial merges
    // and retries. Restoring supported files then recovers the exact settlement.
    for(const bool Unsupported:{false,true})
    {
        const FString Prefix=Unsupported?TEXT("v81_unsupported_account_"):TEXT("v81_malformed_account_");
        Begin();Account.EarnedMask=1u<<1;Account.Routes=2;Account.Dates.Add(Catalog[1].Id,TEXT("2026-09-29T00:00:00Z"));
        const bool ExistingSaved=Save();TArray<uint8> SupportedPrimary,SupportedBackup;
        const bool SupportedSnapshot=ExistingSaved&&FFileHelper::LoadFileToArray(SupportedPrimary,*Path)&&FFileHelper::LoadFileToArray(SupportedBackup,*Backup);
        const FString Bad=Unsupported?TEXT("{\"schema\":99,\"earned\":4095,\"routes\":3,\"tainted_campaigns\":[],\"dates\":{}}"):TEXT("{broken-account");
        const bool BadWritten=FFileHelper::SaveStringToFile(Bad,*Path)&&FFileHelper::SaveStringToFile(Bad,*Backup);
        TArray<uint8> BadPrimary,BadBackup;FFileHelper::LoadFileToArray(BadPrimary,*Path);FFileHelper::LoadFileToArray(BadBackup,*Backup);
        Wire();Outcome(0,*(Prefix+TEXT("initial_merge_failure_does_not_report_award")),false);
        const auto* Pending=SettlementsV81.Find(Run->Id);const FString AwardDate=Pending?Pending->Dates.FindRef(Catalog[0].Id):FString();
        bool NoPrematureAward=true;for(int32 Attempt=0;Attempt<2;++Attempt){NextRetryV81=0;NoPrematureAward&=TakeNotificationV81()==INDEX_NONE&&!Earned(0);}
        TArray<uint8> RetainedPrimary,RetainedBackup;FFileHelper::LoadFileToArray(RetainedPrimary,*Path);FFileHelper::LoadFileToArray(RetainedBackup,*Backup);
        Check(Prefix+TEXT("failed_merges_retain_primary_backup_and_old_honour"),SupportedSnapshot&&BadWritten&&NoPrematureAward&&RetainedPrimary==BadPrimary&&RetainedBackup==BadBackup&&Earned(1)&&Account.Routes==2&&Status(0,true).Contains(Unsupported?TEXT("不受支持"):TEXT("重试")));
        const bool Restored=FFileHelper::SaveArrayToFile(SupportedPrimary,*Path)&&FFileHelper::SaveArrayToFile(SupportedBackup,*Backup);
        // Unsupported records stop automatic writes. An explicit compatible
        // reload is required; the fixture retains its in-process snapshot only.
        if(Unsupported){Account=FAccount();Load();}
        NextRetryV81=0;const int32 MergeNotice=TakeNotificationV81();FAccount MergeRecovered;
        Check(Prefix+TEXT("supported_restore_recovers_award_and_original_dates"),Restored&&MergeNotice==INDEX_NONE&&ReadFile(Path,MergeRecovered)&&MergeRecovered.EarnedMask==3&&MergeRecovered.Routes==2&&!AwardDate.IsEmpty()&&MergeRecovered.Dates.FindRef(Catalog[0].Id)==AwardDate&&MergeRecovered.Dates.FindRef(Catalog[1].Id)==TEXT("2026-09-29T00:00:00Z")&&!HasPendingV81());
        Check(Prefix+TEXT("successful_restore_retry_is_idempotent"),TakeNotificationV81()==INDEX_NONE&&EarnedCount()==2);
    }

    // A supported backup must never downgrade an unsupported primary. Check the
    // symmetric case too, because an atomic save replaces both account files.
    for(const bool UnsupportedPrimary:{true,false})
    {
        const FString Prefix=UnsupportedPrimary?TEXT("v81_future_primary_"):TEXT("v81_future_backup_");
        Begin();Account.EarnedMask=1u<<1;Account.Dates.Add(Catalog[1].Id,TEXT("2026-09-28T00:00:00Z"));
        const bool ExistingSaved=Save();const FString Future=TEXT("{\"schema\":99,\"earned\":4095,\"routes\":3,\"tainted_campaigns\":[],\"dates\":{}}");
        const bool FutureWritten=FFileHelper::SaveStringToFile(Future,*(UnsupportedPrimary?Path:Backup));
        TArray<uint8> OriginalPrimary,OriginalBackup;FFileHelper::LoadFileToArray(OriginalPrimary,*Path);FFileHelper::LoadFileToArray(OriginalBackup,*Backup);
        Wire();Outcome(0,*(Prefix+TEXT("supported_other_file_cannot_authorize_downgrade")),false);
        const double BlockedRetryAt=NextRetryV81;NextRetryV81=0;
        const int32 BlockedVersionNotice=TakeNotificationV81();const bool VersionSave=Save();
        TArray<uint8> AfterPrimary,AfterBackup;FFileHelper::LoadFileToArray(AfterPrimary,*Path);FFileHelper::LoadFileToArray(AfterBackup,*Backup);
        Check(Prefix+TEXT("blocks_automatic_retry_and_preserves_both_files"),ExistingSaved&&FutureWritten&&Account.Unsupported&&!VersionSave&&BlockedVersionNotice==INDEX_NONE&&NextRetryV81==0&&BlockedRetryAt>0&&Status(0,true).Contains(TEXT("不受支持"))&&!Status(0,true).Contains(TEXT("重试"))&&AfterPrimary==OriginalPrimary&&AfterBackup==OriginalBackup&&NotificationsV81.IsEmpty());
        Account=FAccount();Load();
        Check(Prefix+TEXT("fresh_load_reports_unsupported_without_backup_recovery"),Account.Unsupported&&!Account.Recovered&&!Account.Corrupt&&Status(0,false).Contains(TEXT("unsupported version")));
    }
}
}

#include "WarfareFrontlineArmourKillTallyV8220.inl"
#include "WarfareFrontlineAchievementFeasibilityV838.inl"
namespace FrontlineAchievementsV76
{
#include "WarfareFrontlineHonourChecksV84412.inl"
#include "WarfareFrontlineHonourChecksV84415.inl"
}
#include "WarfareFrontlineChallengeFeasibilityV84415.inl"
#include "WarfareFrontlineNationalParityAccountV87.inl"
