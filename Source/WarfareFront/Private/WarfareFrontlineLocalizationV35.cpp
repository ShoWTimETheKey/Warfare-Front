#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineUIAssetsV8213.h"
#include "Misc/Crc.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace FrontlineLocalizationV35
{
namespace
{
struct FMessage { const TCHAR* Chinese; const TCHAR* English; const TCHAR* Source=nullptr; };
const FMessage Messages[] = {
#include "WarfareFrontlineTextCopyV844.generated.inl"
#include "WarfareFrontlineTextCopyV842.generated.inl"
#include "WarfareFrontlineTextCopyV841.generated.inl"
#include "WarfareFrontlineInterfaceCopyV839.inl"
#include "WarfareFrontlineInterfaceCopyV836.inl"
#include "WarfareFrontlineInterfaceCopyV8219.inl"
#include "WarfareFrontlineEditorialCopyV8219.inl"
#include "WarfareFrontlineArmourCopyV8218.generated.inl"
#include "WarfareFrontlineCombatCopyV8215.generated.inl"
#include "WarfareFrontlineTextReviewV829.generated.inl"
#include "WarfareFrontlineInterfaceCopyV75.inl"
#include "WarfareFrontlineInterfaceCopyV72.inl"
#include "WarfareFrontlineNarrativeCopyV66.inl"
#include "WarfareFrontlineInterfaceCopyV66.inl"
{TEXT("Mark V 雄型"),TEXT("Mark V Male")},
{TEXT("Mark IV 工兵伴随"),TEXT("Mark IV Engineer Escort")},
{TEXT("缴获雌型 · 工兵伴随"),TEXT("Beute Pioneer Escort")},
// V60 complete player-facing messages.
{TEXT("点击兵种肖像召集小队。步兵共用整备队列；整备完成且前方有位置时可再次召集。军士提高附近友军命中率，停步后缓解压制。"),TEXT("Select a unit portrait to recruit a section. Infantry share a readiness queue; recruit again when it is ready and space is available ahead. NCOs improve nearby aim and rally troops while halted.")},
{TEXT("每壕容纳3支小队。点击驻军头像令该队出击，或下令整壕前进。关闭入壕时，后续部队从旁通过，驻军留守；满员友壕也可通过。敌壕须先清除守军。"),TEXT("Each trench holds 3 sections. Select a garrison portrait to advance that section, or order the whole trench forward. Closing entry lets following troops pass while the garrison stays; full friendly trenches can also be passed. Clear defenders before entering an enemy trench.")},
{TEXT("选择支援，再标记目标地域；炮火经过传令后抵达。野炮、烟幕与迫击炮分别整备。炮弹、毒气和扫射也会伤及友军；烟幕遮挡双方观察，无法挡住炮弹。"),TEXT("Select support, then mark its target area; fire arrives after the order is relayed. Artillery, smoke and mortars prepare separately. Shells, gas and strafing also harm friendly troops. Smoke obscures both sides' sight but does not stop shells.")},
{TEXT("清除敌方末端防区，让一支小队的全部生还者或整辆战车穿过后方交通线，即可获胜。击溃敌军士气也能取胜。己方炮火、毒气与近距离装甲会动摇士气；占壕、推进和相互支援可恢复少量士气。"),TEXT("Clear the enemy rear sector, then bring every survivor of a section or an entire vehicle through the rear line to win. Breaking enemy morale also wins. Friendly shellfire, gas and nearby armour shake morale; captured trenches, advances and mutual support restore a little.")},
{TEXT("连队作战须知"),TEXT("ORDERS FOR THE COMPANY")},

{TEXT("步兵整备"),TEXT("INFANTRY DRILL")},
{TEXT("火力与观察"),TEXT("FIRE AND OBSERVATION")},
{TEXT("突击与指挥"),TEXT("ASSAULT AND COMMAND")},
{TEXT("突击队与装甲"),TEXT("ASSAULT AND ARMOUR")},
{TEXT("炮兵联络"),TEXT("ARTILLERY LIAISON")},
// V60 current dispatch bodies.
{TEXT("阿拉斯前哨的电话没有回应。连部重新标出枪位与备用路口，联络兵沿交通壕寻找失联分队。"),TEXT("The Arras outpost telephone gives no answer. Headquarters marks the guns and reserve junctions again, while runners search the communication trenches for the missing sections.")},
{TEXT("阿拉斯地道里，部队等着出发。炮兵的测地表送到连部，军士逐一核对各班的行军方向。"),TEXT("Troops wait to move in the Arras tunnels. The battery survey reaches company headquarters and NCOs check each section's route.")},
{TEXT("爆破震断了山脊电话，反斜面的接线箱仍然通话。联络兵找到观察员，把尚存的枪位报回炮兵。"),TEXT("Explosions cut the ridge telephone, but a junction box on the reverse slope still works. Runners find an observer and report the surviving posts to the guns.")},
{TEXT("梅西讷攻势前，测绘员核对爆破点与炮兵表。澳新邻段送来接合处草图，观察员据此标出首轮目标。"),TEXT("Before the Messines attack, surveyors check the mine positions against the fire plan. A sketch arrives from the neighbouring Anzac sector, helping observers mark the opening targets.")},
{TEXT("炮车陷在皮尔克姆的泥里。携弹队把壕炮弹分到肩上，先送往仍能通行的前沿。"),TEXT("Gun wagons sink into the Pilckem mud. Bearers divide the mortar bombs between them and carry them towards the accessible forward posts.")},
{TEXT("佛兰德的排水沟被炮击打断。重炮弹留在后方卸载，壕炮弹由携弹队沿木板路运送。"),TEXT("Shellfire has broken the Flanders drains. Heavy shells are unloaded behind the line while bearers take mortar bombs along the duckboards.")},
{TEXT("反击队在路口等到炮兵观察员。新运来的气弹堆在炮位旁，双方沿同一张地图划定进攻方向。"),TEXT("The counterattack party meets the artillery observer at a junction. Fresh gas shells wait beside the guns as both parties mark their approach on the same map.")},
{TEXT("梅宁路的进攻线已在炮兵图上标出。步兵夺取林缘后就地固守，预备队等待德军反击。"),TEXT("The limited objective at Menin Road is marked on the fire plan. Infantry are to hold the woodland edge once taken, with reserves waiting for the German counterattack.")},
{TEXT("十一月三十日，德军反击康布雷突出部。野炮监视坦克接近线，机枪掩护步兵；缴获车辆仍在修理场。"),TEXT("On 30 November, German troops counterattack the Cambrai salient. Field guns watch the tank approaches while machine guns cover the infantry; captured vehicles remain in the workshops.")},
{TEXT("康布雷的硬地托住了履带。炮兵按测算数据开火，步兵随坦克出发，携弹队沿原路跟进。"),TEXT("Firm ground at Cambrai carries the tracks. The guns fire from calculated data, infantry follow the tanks and carrying parties come up the same routes.")},
{TEXT("前沿需要查清对面的接防情况。轻装巡逻带上烟弹，沿低地接近前哨；重炮今晚不接令。"),TEXT("The front needs word of the enemy relief. A light patrol takes smoke shells along the low approach to the outpost; heavy batteries are not taking orders tonight.")},
{TEXT("冬夜的巡逻册上列着几支轻装分队。重炮联络被划去，士兵带着烟弹出发，准备查探对面的前哨。"),TEXT("A few light sections appear on the winter patrol roll. Battery liaison has been struck out; the men take smoke and move to investigate the opposing outpost.")},
{TEXT("东线部队陆续运到西线。新的突击操练、弹药与出发时刻送到连部，携弹队开始向前集中。"),TEXT("Troops from the Eastern Front arrive in the west. Assault instructions, ammunition allocations and departure times reach company headquarters as the carrying parties gather forward.")},
{TEXT("米夏埃尔攻势迫使英军第五集团军后撤。法军向受威胁的接合部靠拢，补充兵带着新联络表赶到连部。"),TEXT("Operation Michael forces the British Fifth Army back. French troops approach the threatened junction between the armies, and replacements reach the company with a revised contact list.")},
{TEXT("八月八日，邻段枪位接连失去联络。修复的缴获坦克、反坦克步枪与炮兵列入应急命令，掩护后卫通路。"),TEXT("On 8 August, neighbouring posts fall out of contact. Repaired captured tanks, tank rifles and batteries receive emergency orders to cover the rearguard's route.")},
{TEXT("亚眠的炮兵表列着相邻各部的出发时刻。澳大利亚、加拿大、英国与法国部队在各自地段前进，坦克和飞机支援步兵。"),TEXT("The Amiens fire plan lists the departure times of neighbouring formations. Australian, Canadian, British and French troops advance in their sectors with tanks and aircraft supporting the infantry.")},
{TEXT("运河两侧坡岸已被炮火打坏，残存纤道仍有车辆通过。烟弹与炮兵联络优先送到这些路口，供后卫继续使用。"),TEXT("Shellfire has broken the canal banks, but traffic still passes along the remaining towpath. Smoke ammunition and battery contacts are sent first to these junctions for the rear guard.")},
{TEXT("北面的贝利库尔仍在作战，本连在里克瓦尔明渠旁等待消息。工兵察看纤道，炮兵观察员沿岸寻找射界。"),TEXT("Fighting continues around Bellicourt to the north as the company waits beside the open canal at Riqueval. Engineers inspect the towpath and observers look for a view along the bank.")},
{TEXT("车站还堆着未装车的物资。铁路炮组留在塞勒河谷，步兵守住近侧接近线，观察员监视河岸。"),TEXT("Supplies still wait to be loaded at the station. The railway gun crew remains in the Selle valley while infantry hold the near approaches and observers watch the bank.")},
{TEXT("追击沿塞勒河展开，铁路火力守住一段切口。本连需要清出河谷通路，让后续部队继续前进。"),TEXT("The pursuit reaches the Selle, where railway guns hold a cutting. The company must clear a route through the valley for the following troops.")},
{TEXT("军车与平民手推车挤在城外道路上。后卫缩小炮火覆盖区，老兵随补充兵守住路口。"),TEXT("Army wagons and civilian handcarts crowd the road outside town. The rear guard narrows its fire zones and posts veterans beside replacements at the junctions.")},
{TEXT("瓦朗谢讷仍有居民，加拿大军正向城边推进。炮兵标出禁射地段，老兵带着补充兵沿浅沟接近。"),TEXT("Valenciennes still has inhabitants as Canadian troops approach its outskirts. The guns mark restricted fire zones and veterans lead replacements along the shallow ditches.")},
{TEXT("南方停战的消息到了，西线仍在开火。军官把电报放回口袋，军士清点渡口旁等待撤离的分队。"),TEXT("News of the southern armistice arrives while the Western Front still fires. The officer pockets the telegram and the NCO counts the sections waiting beside the crossing.")},
{TEXT("新西兰部队向勒凯努瓦推进，桑布尔渡口仍有枪声。连部抄下接替名单，携弹队沿城外道路前行。"),TEXT("New Zealand troops advance towards Le Quesnoy while firing continues at the Sambre crossings. Headquarters copies the relief list as bearers move along the road outside town.")},
#define WFV43_MESSAGE(Key,Zh,En) {TEXT(Zh),TEXT(En)},
#include "WarfareFrontlinePlayerGuideDataV43.inl"
#undef WFV43_MESSAGE
#define WFV51_MESSAGE(Key,Zh,En) {TEXT(Zh),TEXT(En)},
#include "WarfareFrontlineCampaignCopyV51.inl"
#undef WFV51_MESSAGE
{TEXT("突破敌军后沿，或瓦解敌军士气。占壕建立通路，不会立刻结束战斗。"),TEXT("Break through the enemy rear, or break their morale. Capturing trenches opens a route; it does not end the battle by itself.")},
{TEXT("肃清敌军末端防区，让整支小队或整辆战车突破后方交通线；也可击溃敌军士气。"),TEXT("Clear the enemy rear sector, then bring a whole surviving section or vehicle through the rear line. Breaking enemy morale also wins.")},
{TEXT("清除最后一道敌壕及后沿的守军，让整支小队的生还者或整辆战车突破后方交通线。零散越线不能绕过末端守军。击溃敌军士气也能取胜。友军误伤、毒气和装甲威胁动摇士气；推进与互相支援恢复信心。"),TEXT("Clear the last enemy trench and its rear approaches, then bring every survivor of a section or the whole vehicle through the rear line. Crossing alone cannot bypass the rear guard. Breaking enemy morale also wins. Friendly shellfire, gas and armour shake morale; an advance and mutual support restore confidence.")},
#include "WarfareFrontlineLocalizationCatalogV35.inl"
#include "WarfareFrontlineLocalizationCampaignV41.inl"
#include "WarfareFrontlineLocalizationExtraV41.inl"
};
// Formation designations are stable English proper names in both interfaces.
// Match complete authored names/aliases only: never replace words inside prose.
struct FFormationNameV70 {const TCHAR* Source;const TCHAR* English;};
const FFormationNameV70 FormationNamesV70[]={
    {TEXT("SMLE 步枪班"),TEXT("SMLE Rifle Section")},
    {TEXT("Lewis 支援组"),TEXT("Lewis Gun Section")},
    {TEXT("Mills 突击班"),TEXT("Mills Bombing Section")},
    {TEXT("SMLE 精确射手"),TEXT("SMLE Marksmen")},
    {TEXT("英军军士组"),TEXT("British NCO Section")},
    {TEXT("Gewehr 98 步枪班"),TEXT("Gewehr 98 Rifle Section")},
    {TEXT("MG 08/15 机枪组"),TEXT("MG 08/15 Gun Section")},
    {TEXT("Stielhandgranate 突击班"),TEXT("Stielhandgranate Bombing Section")},
    {TEXT("Gewehr 98 精确射手"),TEXT("Gewehr 98 Marksmen")},
    {TEXT("德军军士组"),TEXT("German NCO Section")},
    {TEXT("SMLE 狙击手"),TEXT("SMLE Snipers")},
    {TEXT("Webley 军官观察组"),TEXT("Webley Officer Section")},
    {TEXT("P08 突击军官组"),TEXT("P08 Assault Officer Section")},
    {TEXT("MP 18 突击队"),TEXT("MP 18 Assault Section")},
    {TEXT("MP 18 风暴突击队"),TEXT("MP 18 Assault Section")},
    {TEXT("No.23 枪榴弹组"),TEXT("No.23 Rifle-Grenade Section")},
    {TEXT("Tankgewehr 反坦克步枪组"),TEXT("Tankgewehr Anti-Tank Rifle Team")},
    {TEXT("Tankgewehr M1918 反坦克枪组"),TEXT("Tankgewehr Anti-Tank Rifle Team")},
    {TEXT("Schilt 法军喷火分遣队"),TEXT("French Schilt Flame Detachment")},
    {TEXT("Wex 喷火工兵"),TEXT("Wex Flame Pioneers")},
    {TEXT("皇家工兵清障组"),TEXT("Royal Engineers Clearance Section")},
    {TEXT("Pioniere 排障组"),TEXT("Pioniere Clearance Section")},
    {TEXT("Lewis 机动枪组"),TEXT("Lewis Mobile Gun Section")},
    {TEXT("MG 08/15 伴随枪组"),TEXT("MG 08/15 Escort Section")},
    {TEXT("炮兵前沿观测组"),TEXT("Forward Artillery Observers")},
    {TEXT("Artilleriebeobachter 观测组"),TEXT("Artillery Observer Section")},
    {TEXT("前沿观测组"),TEXT("Forward Artillery Observers")},
    {TEXT("喷火分队"),TEXT("Flame Detachment")},
    {TEXT("工兵排障队"),TEXT("Pioneer Clearance Section")},
    {TEXT("Mark IV 雄型"),TEXT("Mark IV Male")},
    {TEXT("Mark IV 雌型"),TEXT("Mark IV Female")},
    {TEXT("Whippet 快速坦克"),TEXT("Whippet Medium Tank")},
    {TEXT("A7V 突击战车"),TEXT("A7V Assault Tank")},
    {TEXT("缴获 Mark IV"),TEXT("Captured Mark IV Male")},
    {TEXT("缴获 Mark IV 雌型"),TEXT("Captured Mark IV Female")},
    {TEXT("Mark V 雄型"),TEXT("Mark V Male")},
    {TEXT("Mark IV 工兵伴随"),TEXT("Mark IV Engineer Escort")},
    {TEXT("A7V 501 · Gretchen"),TEXT("A7V 501 · Gretchen")},
    {TEXT("缴获雌型 · 工兵伴随"),TEXT("Beute Pioneer Escort")},
    {TEXT("装甲列车"),TEXT("Armoured Train")},
    {TEXT("Feuerwalze 徐进弹幕"),TEXT("Feuerwalze Creeping Barrage")},
    {TEXT("步兵协同徐进弹幕"),TEXT("Infantry Creeping Barrage")},
    {TEXT("7.7 cm 榴霰弹齐射"),TEXT("7.7 cm Shrapnel Salvo")},
    {TEXT("18 磅榴霰弹齐射"),TEXT("18-pdr Shrapnel Salvo")},
    {TEXT("航空炸弹袭击"),TEXT("Aerial Bombing Raid")},
    {TEXT("低空投弹编队"),TEXT("Low-Level Bombing Flight")},
    {TEXT("7.7 cm 野炮齐射"),TEXT("7.7 cm Field-Gun Salvo")},
    {TEXT("18 磅野炮齐射"),TEXT("18-pounder Salvo")},
    {TEXT("烟幕弹幕"),TEXT("Smoke Barrage")},
    {TEXT("Minenwerfer 壕炮"),TEXT("Minenwerfer Trench Mortar")},
    {TEXT("Stokes 迫击炮"),TEXT("Stokes Mortar")},
    {TEXT("毒气弹幕"),TEXT("Gas Barrage")},
    {TEXT("野炮直射 · 反装甲"),TEXT("Field Gun · Direct Anti-Armour Fire")},
    {TEXT("18 磅炮 · 反装甲"),TEXT("18-pounder · Anti-Armour Fire")},
    {TEXT("Halberstadt CL.II 扫射"),TEXT("Halberstadt CL.II Strafing Run")},
    {TEXT("Bristol F.2B 扫射"),TEXT("Bristol F.2B Strafing Run")},
};
FString LanguageFile() { return FPaths::Combine(FPaths::GeneratedConfigDir(),TEXT("WarfareLanguage.ini")); }
struct FEnglishCacheKeyFuncsV8213 : TDefaultMapKeyFuncs<FString,FString,false>
{
    static bool Matches(KeyInitType A,KeyInitType B){return A.Equals(B,ESearchCase::CaseSensitive);}
    static uint32 GetKeyHash(KeyInitType Key){return FCrc::StrCrc32(*Key);}
};
using FEnglishCacheV8213=TMap<FString,FString,FDefaultSetAllocator,FEnglishCacheKeyFuncsV8213>;
struct FState
{
    ELanguage Language=ELanguage::English;
    TMap<FString,const FMessage*> Exact;
    TMap<FString,const FMessage*> Folded;
    TArray<const FMessage*> Templates;
    // The original ignore-case map is used only by the isolated baseline.
    TMap<FString,FString> Cache;
    FEnglishCacheV8213 TextCacheV8213,EnglishCache;
    TArray<FString> CacheOrderV8213,EnglishCacheOrderV8213;
    int32 CacheNextV8213=0,EnglishCacheNextV8213=0;
    TMap<FString,FString> FormationNames;
    TSet<FString> Missing;
    TSet<FString> ChineseFallbacks;
    FState()
    {
        FString Choice;FConfigFile Config;Config.Read(LanguageFile());Config.GetString(TEXT("Interface"),TEXT("Language"),Choice);
        FParse::Value(FCommandLine::Get(),TEXT("WFLanguage="),Choice);
        Language=Choice==TEXT("zh")?ELanguage::Chinese:ELanguage::English;
        for(const auto& M:Messages)
        {
            // The first source entry is canonical. Later English aliases may
            // provide an alternative heading, never change a Chinese word
            // into a faction-specific caption such as PANZER.
            for(const TCHAR* Key:{M.Source,M.Chinese,M.English})if(Key&&*Key)
            {
                if(!Exact.Contains(Key))Exact.Add(Key,&M);
                const FString Normal=FString(Key).TrimStartAndEnd().ToLower();
                if(!Folded.Contains(Normal))Folded.Add(Normal,&M);
            }
            if(FString(M.Chinese).Contains(TEXT("%")) || FString(M.English).Contains(TEXT("%")) || (M.Source&&FString(M.Source).Contains(TEXT("%"))))Templates.Add(&M);
        }
        for(const auto& N:FormationNamesV70)
        {
            FormationNames.Add(FString(N.Source).TrimStartAndEnd().ToLower(),N.English);
            FormationNames.Add(FString(N.English).ToLower(),N.English);
            // Existing translated names remain aliases, including title-case
            // or legacy wording carried by saves and authored introduction cards.
            if(const FMessage* const* M=Exact.Find(N.Source))
                for(const TCHAR* Alias:{(*M)->Chinese,(*M)->English,(*M)->Source})
                    if(Alias&&*Alias)FormationNames.Add(FString(Alias).TrimStartAndEnd().ToLower(),N.English);
        }
    }
};
FState& State(){static FState S;return S;}
const FMessage* FindMessageV66(const FString& Source)
{
    auto& S=State();if(const auto* M=S.Exact.Find(Source))return *M;
    if(const auto* M=S.Folded.Find(Source.TrimStartAndEnd().ToLower()))return *M;
    return nullptr;
}
struct FTemplate { TArray<FString> Parts;TArray<TCHAR> Types; };
FTemplate ParseTemplate(const FString& Format)
{
    FTemplate T;FString Part;
    for(int32 I=0;I<Format.Len();++I)
    {
        if(Format[I]!=TCHAR('%')){Part+=Format[I];continue;}
        if(I+1<Format.Len() && Format[I+1]==TCHAR('%')){Part+=TCHAR('%');++I;continue;}
        // Authored prose such as "10% sooner" contains a literal percentage,
        // not a space-flagged %s conversion. Preserve its ordinary typography.
        if(I>0 && FChar::IsDigit(Format[I-1]) && I+1<Format.Len() && FChar::IsWhitespace(Format[I+1]))
        {Part+=TCHAR('%');continue;}
        int32 J=I+1;while(J<Format.Len() && (FChar::IsDigit(Format[J]) || Format[J]=='.' || Format[J]=='-' || Format[J]=='+' || Format[J]==' '))++J;
        if(J>=Format.Len() || !FString(TEXT("disufg" )).Contains(FString::Chr(Format[J]),ESearchCase::CaseSensitive)){Part+=Format[I];continue;}
        T.Parts.Add(Part);Part.Empty();T.Types.Add(Format[J]);I=J;
    }
    T.Parts.Add(Part);return T;
}
struct FParsedMessageV8213
{
    const FMessage* Message=nullptr;
    FTemplate From[3];
};
const TArray<FParsedMessageV8213>& ParsedTemplatesV8213()
{
    static const TArray<FParsedMessageV8213> Parsed=[]()
    {
        TArray<FParsedMessageV8213> Result;
        for(const FMessage* M:State().Templates)
        {
            FParsedMessageV8213 Entry;Entry.Message=M;
            Entry.From[0]=ParseTemplate(M->Chinese);Entry.From[1]=ParseTemplate(M->English);
            if(M->Source)Entry.From[2]=ParseTemplate(M->Source);
            Result.Add(MoveTemp(Entry));
        }
        return Result;
    }();
    return Parsed;
}
bool NumericDisplayCharactersV8213(const FString& Value)
{
    // Deliberately limited to ASCII counter/time punctuation. Newlines, text,
    // non-ASCII scripts and other symbols still use the complete resolver.
    for(TCHAR C:Value)
    {
        if(C>='0'&&C<='9')continue;
        switch(C)
        {
        case ' ':case '\t':case ':':case '.':case ',':case '+':case '-':
        case '/':case '%':case '(':case ')':case '[':case ']':case '|':
            break;
        default:return false;
        }
    }
    return true;
}
bool PlainNumericDisplayV8213(const FString& Value)
{
    if(!NumericDisplayCharactersV8213(Value))return false;
    for(TCHAR C:Value)if(C>='0'&&C<='9')return true;
    return false;
}
bool NumericFastPathSafeV8213()
{
    static const bool Safe=[]()
    {
        const auto& S=State();
        // Preserve authored numeric literals and complete formation aliases.
        // If future copy adds one, the conservative shortcut disables itself.
        for(const auto& Pair:S.Exact)if(PlainNumericDisplayV8213(Pair.Key))return false;
        for(const auto& Pair:S.FormationNames)if(PlainNumericDisplayV8213(Pair.Key))return false;
        for(const auto& Entry:ParsedTemplatesV8213())for(int32 Direction=0;Direction<3;++Direction)
        {
            if(Direction==2&&!Entry.Message->Source)continue;
            const auto& From=Entry.From[Direction];if(From.Types.IsEmpty())continue;
            bool CouldMatch=true;
            for(const auto& Part:From.Parts)CouldMatch&=NumericDisplayCharactersV8213(Part);
            if(CouldMatch)return false;
        }
        return true;
    }();
    return Safe;
}
template<class TCache> void AddBoundedResultV8213(TCache& Cache,TArray<FString>& Order,int32& Next,const FString& Source,const FString& Result)
{
    constexpr int32 Capacity=4096;
    if(Order.Num()<Capacity)Order.Add(Source);
    else
    {
        Cache.Remove(Order[Next]);Order[Next]=Source;Next=(Next+1)%Capacity;
    }
    Cache.Add(Source,Result);
}
bool Capture(const FString& Value,const FTemplate& T,TArray<FString>& Args)
{
    // UE's StartsWith intentionally returns false for an empty search value.
    // Numeric-first counters and name-first support orders have no prefix.
    if(T.Types.IsEmpty() || (!T.Parts[0].IsEmpty()&&!Value.StartsWith(T.Parts[0])))return false;
    int32 Position=T.Parts[0].Len();
    for(int32 I=0;I<T.Types.Num();++I)
    {
        const FString& Delimiter=T.Parts[I+1];
        const int32 End=Delimiter.IsEmpty()?Value.Len():Value.Find(Delimiter,ESearchCase::CaseSensitive,ESearchDir::FromStart,Position);
        if(End==INDEX_NONE || End<Position)return false;
        const FString Arg=Value.Mid(Position,End-Position);
        if(Arg.IsEmpty())return false;
        if(T.Types[I]!='s')for(TCHAR C:Arg)if(!FChar::IsDigit(C)&&C!='.'&&C!='-'&&C!='+'&&C!=' ')return false;
        Args.Add(Arg);Position=End+Delimiter.Len();
    }
    return Position==Value.Len();
}
FString Resolve(const FString& Source,int32 Depth,ELanguage Language)
{
    auto& S=State();
    if(!FrontlineUIAssetsV8213::BaselineMode()&&PlainNumericDisplayV8213(Source)&&NumericFastPathSafeV8213())return Source;
    if(const FString* Name=S.FormationNames.Find(Source.TrimStartAndEnd().ToLower()))return *Name;
    if(Language==ELanguage::Chinese)
    {
        // Dates are authored as day/month/year on orders and map labels.
        TArray<FString> Words;Source.TrimStartAndEnd().ParseIntoArray(Words,TEXT(" "),true);
        static const TCHAR* Months[]={TEXT("JANUARY"),TEXT("FEBRUARY"),TEXT("MARCH"),TEXT("APRIL"),TEXT("MAY"),TEXT("JUNE"),TEXT("JULY"),TEXT("AUGUST"),TEXT("SEPTEMBER"),TEXT("OCTOBER"),TEXT("NOVEMBER"),TEXT("DECEMBER")};
        if(Words.Num()==3&&Words[0].IsNumeric()&&Words[2].IsNumeric())
            for(int32 M=0;M<12;++M)if(Words[1].Equals(Months[M],ESearchCase::IgnoreCase))
            {
                const int32 Day=FCString::Atoi(*Words[0]),Year=FCString::Atoi(*Words[2]);
                if(Day>=1&&Day<=31&&Year>=1900&&Year<=2100)return FString::Printf(TEXT("%d年%d月%d日"),Year,M+1,Day);
            }
    }
    if(const FMessage* M=FindMessageV66(Source))
    {
        const FString Translation=Language==ELanguage::Chinese?M->Chinese:M->English;
        // A control may deliberately supply an engraved all-capitals English
        // label. Preserve its typography when the catalog differs only in
        // letter case; Chinese sources and actual translations still resolve.
        if(Language==ELanguage::English && !Source.Equals(Translation,ESearchCase::CaseSensitive)
            && Source.Equals(Translation,ESearchCase::IgnoreCase)
            && Source.Equals(Source.ToUpper(),ESearchCase::CaseSensitive))return Source;
        return Translation;
    }
    if(Depth<4)
    {
        if(FrontlineUIAssetsV8213::BaselineMode())for(const FMessage* M:S.Templates)
        {
            for(int32 Direction=0;Direction<3;++Direction)
            {
                const TCHAR* Pattern=Direction==0?M->Chinese:Direction==1?M->English:M->Source;
                if(!Pattern)continue;
                const FTemplate From=ParseTemplate(Pattern);
                TArray<FString> Args;if(!Capture(Source,From,Args))continue;
                const FTemplate To=ParseTemplate(Language==ELanguage::Chinese?M->Chinese:M->English);
                if(To.Types.Num()!=Args.Num())continue;
                FString Out=To.Parts[0];
                for(int32 I=0;I<Args.Num();++I){Out+=From.Types[I]=='s'?Resolve(Args[I],Depth+1,Language):Args[I];Out+=To.Parts[I+1];}
                return Out;
            }
        }
        else for(const auto& Entry:ParsedTemplatesV8213())
        {
            for(int32 Direction=0;Direction<3;++Direction)
            {
                if(Direction==2&&!Entry.Message->Source)continue;
                const FTemplate& From=Entry.From[Direction];
                TArray<FString> Args;if(!Capture(Source,From,Args))continue;
                const FTemplate& To=Entry.From[Language==ELanguage::Chinese?0:1];
                if(To.Types.Num()!=Args.Num())continue;
                FString Out=To.Parts[0];
                for(int32 I=0;I<Args.Num();++I){Out+=From.Types[I]=='s'?Resolve(Args[I],Depth+1,Language):Args[I];Out+=To.Parts[I+1];}
                return Out;
            }
        }
        // A dispatch may contain separate authored paragraphs. This is a
        // paragraph boundary, never a substring or word-by-word replacement.
        if(Source.Contains(TEXT("\n")))
        {
            TArray<FString> Lines;Source.ParseIntoArray(Lines,TEXT("\n"),false);
            for(FString& Line:Lines)Line=Resolve(Line,Depth+1,Language);
            return FString::Join(Lines,TEXT("\n"));
        }
    }
    if(Language==ELanguage::English)for(TCHAR C:Source)if(C>=0x4e00 && C<=0x9fff){S.Missing.Add(Source);break;}
    if(Language==ELanguage::Chinese)
    {
        bool CJK=false;int32 Letters=0;
        for(TCHAR C:Source){CJK|=C>=0x4e00&&C<=0x9fff;Letters+=(C>='a'&&C<='z')||(C>='A'&&C<='Z');}
        // Unknown Latin text is reported for inspection; weapon names, brands,
        // hardware labels and shortcuts remain legitimate exceptions.
        if(!CJK&&Letters>1)S.ChineseFallbacks.Add(Source);
    }
    return Source;
}
}
ELanguage GetLanguage(){return State().Language;}
bool SetLanguage(ELanguage Language)
{
    auto& S=State();S.Language=Language;S.Cache.Empty();S.TextCacheV8213.Empty();
    S.CacheOrderV8213.Empty();S.CacheNextV8213=0;
    FConfigFile Config;Config.SetString(TEXT("Interface"),TEXT("Language"),Language==ELanguage::Chinese?TEXT("zh"):TEXT("en"));
    const bool Volatile=FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||FParse::Param(FCommandLine::Get(),TEXT("WFV35Review"));
    const bool Saved=Volatile||Config.Write(LanguageFile());
    UE_LOG(LogTemp,Display,TEXT("WF_V35_LANGUAGE language=%s persisted=%d"),Language==ELanguage::Chinese?TEXT("zh"):TEXT("en"),Saved);
    return Saved;
}
FString Text(const FString& Source)
{
    if(Source.IsEmpty())return Source;
    if(!FrontlineUIAssetsV8213::BaselineMode()&&PlainNumericDisplayV8213(Source)&&NumericFastPathSafeV8213())return Source;
    auto& S=State();
    if(FrontlineUIAssetsV8213::BaselineMode())
    {
        if(const FString* Found=S.Cache.Find(Source))return *Found;
        FString Result=Resolve(Source,0,S.Language);if(S.Cache.Num()>4096)S.Cache.Empty();S.Cache.Add(Source,Result);return Result;
    }
    if(const FString* Found=S.TextCacheV8213.Find(Source))return *Found;
    FString Result=Resolve(Source,0,S.Language);
    AddBoundedResultV8213(S.TextCacheV8213,S.CacheOrderV8213,S.CacheNextV8213,Source,Result);
    return Result;
}
FString English(const FString& Source)
{
    if(FrontlineUIAssetsV8213::BaselineMode())return Resolve(Source,0,ELanguage::English);
    if(PlainNumericDisplayV8213(Source)&&NumericFastPathSafeV8213())return Source;
    auto& S=State();if(const FString* Found=S.EnglishCache.Find(Source))return *Found;
    FString Result=Resolve(Source,0,ELanguage::English);
    AddBoundedResultV8213(S.EnglishCache,S.EnglishCacheOrderV8213,S.EnglishCacheNextV8213,Source,Result);return Result;
}
void RunFormationNameChecksV70(TFunctionRef<void(const FString&,bool)> Check)
{
    bool Stable=true,EnglishOnly=true;
    for(const auto& N:FormationNamesV70)
    {
        const FString EN=Resolve(N.Source,0,ELanguage::English),ZH=Resolve(N.Source,0,ELanguage::Chinese);
        Stable&=EN==N.English&&ZH==EN&&Resolve(ZH,0,ELanguage::Chinese)==EN;
        for(TCHAR C:ZH)EnglishOnly&=!(C>=0x2E80&&C<=0x9FFF);
    }
    Check(TEXT("v70_all_formation_names_identical_in_both_languages"),Stable);
    Check(TEXT("v70_formation_names_never_contain_chinese_glyphs"),EnglishOnly);
    Check(TEXT("v70_names_do_not_translate_surrounding_chinese_prose"),
        Resolve(TEXT("两侧六磅炮压住阵位，刘易斯机枪掩护步兵。履带受损后仍可留在原地射击。"),0,ELanguage::Chinese).Contains(TEXT("机枪")));
}
void RunCopyChecksV66(TFunctionRef<void(const FString&,bool)> Check)
{
    if((!FParse::Param(FCommandLine::Get(),TEXT("WFV73Review"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV72Review"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV66Review")))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    Check(TEXT("menu_case_alias_resolves_to_chinese"),Resolve(TEXT("GERMAN CAMPAIGN"),0,ELanguage::Chinese)==TEXT("德军战役"));
    Check(TEXT("trailing_space_does_not_break_translation"),Resolve(TEXT("THE WESTERN FRONT "),0,ELanguage::Chinese)==TEXT("西线战场"));
    Check(TEXT("new_copy_wins_over_legacy_english"),Resolve(TEXT("The enemy has occupied the forward trench. Watch the pressure on the line."),0,ELanguage::English)==TEXT("The enemy has taken the forward trench."));
    Check(TEXT("dynamic_source_alias_rewrites_values"),Resolve(TEXT("将删除所选的 3 条战役及其研究和进度。此操作无法在游戏中撤销。"),0,ELanguage::English)==TEXT("Delete the selected 3 campaign records, including research and progress. This cannot be undone."));
    Check(TEXT("hud_english_resolver_ignores_selected_locale"),English(TEXT("步兵"))==TEXT("INFANTRY"));
    Check(TEXT("operation_dates_follow_selected_language"),Resolve(TEXT("9 APRIL 1917"),0,ELanguage::Chinese)==TEXT("1917年4月9日")&&Resolve(TEXT("9 APRIL 1917"),0,ELanguage::English)==TEXT("9 APRIL 1917"));
    const ELanguage PreviousLanguage=GetLanguage();
    if(!FrontlineUIAssetsV8213::BaselineMode())
    {
        SetLanguage(ELanguage::English);
        const FString TitleFirst=Text(TEXT("Credits")),UpperSecond=Text(TEXT("CREDITS"));
        Check(TEXT("v8213_text_cache_preserves_title_then_uppercase"),
            TitleFirst.Equals(TEXT("Credits"),ESearchCase::CaseSensitive)&&UpperSecond.Equals(TEXT("CREDITS"),ESearchCase::CaseSensitive));
        SetLanguage(ELanguage::English);
        const FString UpperFirst=Text(TEXT("CREDITS")),TitleSecond=Text(TEXT("Credits"));
        Check(TEXT("v8213_text_cache_preserves_uppercase_then_title"),
            UpperFirst.Equals(TEXT("CREDITS"),ESearchCase::CaseSensitive)&&TitleSecond.Equals(TEXT("Credits"),ESearchCase::CaseSensitive));
    }
    const FString NumericDisplay=TEXT("01:23 / 89.0%"),Date=TEXT("9 APRIL 1917");
    const FString Dynamic=TEXT("将删除所选的 3 条战役及其研究和进度。此操作无法在游戏中撤销。");
    const FString DynamicEnglish=Resolve(Dynamic,0,ELanguage::English),DynamicChinese=Resolve(Dynamic,0,ELanguage::Chinese);
    SetLanguage(ELanguage::English);
    const bool CachedEnglish=Text(NumericDisplay).Equals(NumericDisplay,ESearchCase::CaseSensitive)
        &&Text(Date).Equals(Date,ESearchCase::CaseSensitive)
        &&Text(Dynamic).Equals(DynamicEnglish,ESearchCase::CaseSensitive);
    SetLanguage(ELanguage::Chinese);
    const bool CachedChinese=Text(NumericDisplay).Equals(NumericDisplay,ESearchCase::CaseSensitive)
        &&Text(Date).Equals(TEXT("1917年4月9日"),ESearchCase::CaseSensitive)
        &&Text(Dynamic).Equals(DynamicChinese,ESearchCase::CaseSensitive)
        &&English(Dynamic).Equals(DynamicEnglish,ESearchCase::CaseSensitive);
    SetLanguage(ELanguage::English);
    Check(TEXT("v8213_numeric_date_template_cache_tracks_selected_language"),CachedEnglish&&CachedChinese
        &&Text(Date).Equals(Date,ESearchCase::CaseSensitive)&&Text(Dynamic).Equals(DynamicEnglish,ESearchCase::CaseSensitive));
    SetLanguage(PreviousLanguage);
    int32 Formats=0,Aliases=0;TSet<FString> Seen;TArray<TSharedPtr<FJsonValue>> Current,Errors;
    for(const auto& M:Messages)
    {
        Formats+=ParseTemplate(M.Chinese).Types!=ParseTemplate(M.English).Types;
        if(M.Source&&ParseTemplate(M.Source).Types!=ParseTemplate(M.Chinese).Types)
        {++Aliases;auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("source"),M.Source);J->SetStringField(TEXT("chinese"),M.Chinese);J->SetStringField(TEXT("english"),M.English);Errors.Add(MakeShared<FJsonValueObject>(J));}
        const FString Source=M.Source?M.Source:M.Chinese;if(Seen.Contains(Source))continue;Seen.Add(Source);
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("source"),Source);J->SetStringField(TEXT("chinese"),Resolve(Source,0,ELanguage::Chinese));J->SetStringField(TEXT("english"),Resolve(Source,0,ELanguage::English));Current.Add(MakeShared<FJsonValueObject>(J));
    }
    Check(TEXT("bilingual_format_signatures_match"),Formats==0);
    Check(TEXT("legacy_dynamic_alias_signatures_match"),Aliases==0);
    auto Root=MakeShared<FJsonObject>();Root->SetArrayField(TEXT("resolved_messages"),Current);Root->SetArrayField(TEXT("invalid_dynamic_aliases"),Errors);
    FString Json;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));
    const FString CopyDir=FPaths::ProjectSavedDir()/(FParse::Param(FCommandLine::Get(),TEXT("WFV73Review"))?TEXT("Gameplay/V73/Review"):FParse::Param(FCommandLine::Get(),TEXT("WFV72Review"))?TEXT("Gameplay/V72/Review"):TEXT("Gameplay/V66/Review"));
    IFileManager::Get().MakeDirectory(*CopyDir,true);FFileHelper::SaveStringToFile(Json,*(CopyDir/TEXT("resolved-copy.json")));
}
const TCHAR* Literal(const TCHAR* Source)
{
    auto& S=State();if(const FMessage* M=FindMessageV66(Source))return S.Language==ELanguage::Chinese?M->Chinese:M->English;
    return Source;
}
bool WriteCoverageReport()
{
    auto& S=State();auto Root=MakeShared<FJsonObject>();Root->SetNumberField(TEXT("authored_messages"),UE_ARRAY_COUNT(Messages));Root->SetStringField(TEXT("language"),S.Language==ELanguage::Chinese?TEXT("zh"):TEXT("en"));
    TArray<TSharedPtr<FJsonValue>> Missing;for(const FString& M:S.Missing)Missing.Add(MakeShared<FJsonValueString>(M));Root->SetArrayField(TEXT("unmapped_visible_messages"),Missing);
    TArray<TSharedPtr<FJsonValue>> Fallbacks;for(const FString& M:S.ChineseFallbacks)Fallbacks.Add(MakeShared<FJsonValueString>(M));Root->SetArrayField(TEXT("chinese_unmapped_latin_review"),Fallbacks);
    int32 FormatErrors=0;for(const auto& M:Messages)if(ParseTemplate(M.Chinese).Types!=ParseTemplate(M.English).Types)++FormatErrors;
    Root->SetNumberField(TEXT("format_signature_errors"),FormatErrors);Root->SetBoolField(TEXT("success"),FormatErrors==0&&Missing.IsEmpty());
    FString Json;FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
    const FString Directory=FPaths::ProjectSavedDir()/(FParse::Param(FCommandLine::Get(),TEXT("WFV45Review"))?TEXT("Gameplay/V45"):FParse::Param(FCommandLine::Get(),TEXT("WFV43Review"))?TEXT("Gameplay/V43"):FParse::Param(FCommandLine::Get(),TEXT("WFV42Review"))?TEXT("Gameplay/V42"):FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))?TEXT("Gameplay/V41"):FParse::Param(FCommandLine::Get(),TEXT("WFV40Review"))?TEXT("Gameplay/V40"):FParse::Param(FCommandLine::Get(),TEXT("WFV39Review"))?TEXT("Gameplay/V39"):FParse::Param(FCommandLine::Get(),TEXT("WFV38Review"))?TEXT("Gameplay/V38"):FParse::Param(FCommandLine::Get(),TEXT("WFV37Review"))?TEXT("Gameplay/V37"):FParse::Param(FCommandLine::Get(),TEXT("WFV36Review"))?TEXT("Gameplay/V36"):TEXT("Gameplay/V35"));
    const FString OutputDirectory=FParse::Param(FCommandLine::Get(),TEXT("WFV73Review"))?FPaths::ProjectSavedDir()/TEXT("Gameplay/V73/Review"):FParse::Param(FCommandLine::Get(),TEXT("WFV72Review"))?FPaths::ProjectSavedDir()/TEXT("Gameplay/V72/Review"):FParse::Param(FCommandLine::Get(),TEXT("WFV66Review"))?FPaths::ProjectSavedDir()/TEXT("Gameplay/V66/Review"):Directory;
    IFileManager::Get().MakeDirectory(*OutputDirectory,true);
    const bool Saved=FFileHelper::SaveStringToFile(Json,*(OutputDirectory/TEXT("localization-runtime.json")));
    return Saved&&FormatErrors==0&&Missing.IsEmpty();
}
}

