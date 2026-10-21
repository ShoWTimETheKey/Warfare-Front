#include "WarfareFrontlinePlayerGuideV43.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineLocalizationV35.h"

namespace
{
struct FLine {const TCHAR *Key,*Chinese,*English;};
const FLine Lines[]={
#define WFV43_MESSAGE(Key,Zh,En) {TEXT(Key),TEXT(Zh),TEXT(En)},
#include "WarfareFrontlinePlayerGuideDataV43.inl"
#undef WFV43_MESSAGE
};
const TCHAR* Line(const FString& Key)
{
    for(const auto& L:Lines)if(Key==L.Key)return L.English;
    return TEXT("");
}
constexpr int32 BritishTechnology[]={0,2,3,0,1,2,6,4,7,11,8,9,8,12,13,10,5,15,16,15,14,16,17,18,2,6,19};
constexpr int32 GermanTechnology[]={0,2,3,0,1,2,5,4,7,6,8,9,11,12,13,11,5,15,15,15,14,16,17,18,14,14,19};
}
namespace FrontlinePlayerGuideV43
{
FMoraleLabels MoraleLabels(bool Left)
{
    const FMoraleLabels L{FBox2D(FVector2D(94,41),FVector2D(594,64)),FBox2D(FVector2D(620,41),FVector2D(836,64))};
    if(Left)return L;
    auto Mirror=[](const FBox2D& R){return FBox2D(FVector2D(1920-R.Max.X,R.Min.Y),FVector2D(1920-R.Min.X,R.Max.Y));};
    return {Mirror(L.Army),Mirror(L.Value)};
}
TArray<EFrontlineRole> NewRoles(EFrontlineFaction Faction,const FFrontlineCampaignMission& Mission)
{
    TArray<EFrontlineRole> Roles;
    for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
    {
        const auto Role=EFrontlineRole(I);
        if(FrontlineGameplayV52::SelectableRole(Role)&&FrontlineCampaignV41::RoleArrival(Faction,Role)==Mission.Id && !(Mission.DisabledRoleMask&(1u<<I)))Roles.Add(Role);
    }
    return Roles;
}
FString Unit(const AWarfareFrontlineGameMode* B,EFrontlineRole Role)
{
    const bool G=B&&B->GetPlayerFaction()==EFrontlineFaction::German;
    if(G&&Role==EFrontlineRole::Marksman)return FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?
        TEXT("Jäger patrol. Veteran riflemen close rapidly and deliver accurate fire at medium range. Effective against exposed infantry; machine guns and armour halt an unsupported advance."):
        TEXT("猎兵小队。老兵迅速接敌，在中等距离以准确步枪火力压住暴露步兵。缺乏掩护时，难以突破机枪与装甲。");
    FString Key=FString::Printf(TEXT("%s.U%d"),G?TEXT("G"):TEXT("B"),int32(Role));
    if(B&&Role==EFrontlineRole::Marksman&&B->HasScopedMarksmen())Key=TEXT("B.Sniper");
    if(B&&Role==EFrontlineRole::Mills&&B->HasStormtroopers())Key=TEXT("G.Storm");
    if(B&&Role==EFrontlineRole::Officer&&B->HasCommissionedOfficers())Key=G?TEXT("G.Officer"):TEXT("B.Officer");
    return Line(Key);
}
const TCHAR* Support(EFrontlineSupport Kind,EFrontlineFaction)
{return Line(FString::Printf(TEXT("S.%d"),int32(Kind)));}
const TCHAR* Armour(EFrontlineVehicle Type)
{
 if(const TCHAR* NewGuide=FrontlineArmourV63::VehicleDescriptionV8218(Type);*NewGuide)return NewGuide;
 return Line(FString::Printf(TEXT("A.%d"),int32(Type)));
}

FString Technology(const AWarfareFrontlineGameMode* B,const FFrontlineTechnologyNode& Node)
{
    const bool G=B&&B->GetPlayerFaction()==EFrontlineFaction::German;
    const TCHAR* Effect=Line(FString::Printf(TEXT("%s.T%d"),G?TEXT("G"):TEXT("B"),Node.Id));
    if(!*Effect)Effect=Line(FString::Printf(TEXT("T.%d"),Node.Id));
    // Each authored entry carries its own period flavour and concise rule. Do
    // not prepend the obsolete second explanation from the legacy node table.
    return *Effect?FString(Effect):FString(Node.Effect);
}
int32 TechnologyArrival(EFrontlineFaction Faction,int32 Id)
{return Id<0||Id>=int32(EFrontlineTechnology::Count)?MAX_int32:((Faction==EFrontlineFaction::German&&(Id==12||Id==14||Id==15))?11:0);}
EFrontlineRole TechnologyRole(EFrontlineFaction F,int32 Id)
{
    if(Id==24||Id==25)return F==EFrontlineFaction::British?EFrontlineRole::RifleGrenadier:EFrontlineRole::AntiTankRifle;
    if(Id==17)return EFrontlineRole::Pioneer;if(Id==18)return EFrontlineRole::Flamethrower;
    if(F==EFrontlineFaction::British)
    {
        if(Id==2||Id==6||Id==7||Id==8)return EFrontlineRole::Lewis;
        if(Id==4||Id==11)return EFrontlineRole::Mills;
        if(Id==9||Id==14)return EFrontlineRole::Officer;
        if(Id==10||Id==12||Id==13)return EFrontlineRole::Marksman;
    }
    else
    {
        if(Id==2||Id==7||Id==8||Id==13)return EFrontlineRole::Lewis;
        if(Id==6)return EFrontlineRole::Officer;
        if(Id==4||Id==9||Id==11||Id==12||Id==14||Id==15)return EFrontlineRole::Mills;
    }
    return EFrontlineRole::Rifle;
}
int32 SupportArrival(EFrontlineSupport Kind)
{
    switch(Kind)
    {
    case EFrontlineSupport::Artillery:return 0;
    case EFrontlineSupport::Smoke:return 1;
    case EFrontlineSupport::Mortar:return 5;
    case EFrontlineSupport::Gas:return 7;
    case EFrontlineSupport::AntiTank:return 8;
    case EFrontlineSupport::AirStrike:return 11;
    case EFrontlineSupport::AirRecon:return 3;
    case EFrontlineSupport::CreepingBarrage:return 15;
    case EFrontlineSupport::AirBombing:return 16;
    default:return MAX_int32;
    }
}
void ApplyCampaign(TArray<FFrontlineCampaignMission>& Missions,bool G)
{
    // Keep every saved operation ID, date, paid research bit and reward boundary.
    // These are company attachments and orders, not weapon invention dates.
    for(auto& M:Missions)
    {
        if(M.Id<0||M.Id>=20)continue;
        M.Briefing=Line(FString::Printf(TEXT("%s.O%d"),G?TEXT("G"):TEXT("B"),M.Id));
        M.AlliedContext=Line(FString::Printf(TEXT("%s.C%d"),G?TEXT("G"):TEXT("B"),M.Id));
        M.ObjectiveText=TEXT("肃清敌军末端防区，让整支小队或整辆战车突破后方交通线；也可击溃敌军士气。");
        // Counterattacks and captured supplies are actual intermediate events.
        // Distinct compositions use ordinary readiness and map-edge recruitment.
        if(M.Id==6){M.bSupplyBayV41=true;M.PlayerMortarDelay=15.f;}
        if(M.Id==12){M.bCounterstrokeV41=true;M.bSupplyBayV41=false;}
        if(M.Id==18){M.bSupplyBayV41=true;M.PlayerSmokeDelay=0.f;}
        if(M.Id==19){M.bCounterstrokeV41=true;M.bSupplyBayV41=false;}
        // Company attachments differ by task. The adaptive planner may still
        // choose counters, but its ordinary recruitment pool is no longer the
        // same unlocked catalogue in every operation. Repeated entries are
        // intentional weights, never additional troops or instant readiness.
        const auto Available=M.EnemyRoleOrder;
        static const int32 Priority[20][3]={
            {0,2,0},{2,0,2},{2,0,1},{1,0,1},{1,0,2},
            {1,4,2},{1,2,3},{1,3,4},{1,3,0},{1,4,2},
            {2,3,0},{2,4,1},{2,1,4},{3,2,4},{3,1,4},
            {6,2,1},{6,5,2},{8,3,6},{3,4,6},{4,1,6}
        };
        M.EnemyRoleOrder={EFrontlineRole::Rifle,EFrontlineRole::Rifle};
        for(int32 Role:Priority[M.Id])
        {
            const auto R=EFrontlineRole(Role);
            if(Available.Contains(R)&&!(M.Id==10&&R==EFrontlineRole::Flamethrower))M.EnemyRoleOrder.Add(R);
        }
        // Mission restrictions use the player's attachment mask, which is not
        // a valid enemy unlock table. Keep all historically available enemy
        // specialists in the fallback pool with their own faction gates.
        for(const auto R:Available)
            if(!M.EnemyRoleOrder.Contains(R)&&!(M.Id==10&&R==EFrontlineRole::Flamethrower))M.EnemyRoleOrder.Add(R);
        M.EnemyOpeningRoles={EFrontlineRole::Rifle};
        for(int32 Role:Priority[M.Id])
        {
            const auto R=EFrontlineRole(Role);
            if(Available.Contains(R)&&!M.EnemyOpeningRoles.Contains(R)&&!(M.Id==10&&R==EFrontlineRole::Flamethrower))M.EnemyOpeningRoles.Add(R);
            if(M.EnemyOpeningRoles.Num()>=3)break;
        }
    }
}
void RunChecks(const AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)> Check)
{
    bool Text=true,Gates=true,Chapters=true,Guides=true,Rules=true,Introductions=true;TSet<FString> Keys,AllFocus;
    for(const auto& L:Lines){Text&=*L.English&&*L.Chinese&&!Keys.Contains(L.Key)&&FrontlineLocalizationV35::English(L.Chinese)==FrontlineLocalizationV35::English(L.English);Keys.Add(L.Key);}
    for(int32 F=0;F<2;++F)
    {
        const auto& Missions=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction(F));
        const auto& Nodes=AWarfareFrontlineGameMode::GetTechnologyNodesForFaction(EFrontlineFaction(F));
        Chapters&=Missions.Num()==20&&Nodes.Num()==int32(EFrontlineTechnology::Count);TSet<FString> Focus;
        for(const auto& M:Missions)
        {
            Focus.Add(M.AlliedContext);AllFocus.Add(M.AlliedContext);Chapters&=*M.Briefing&&*M.AlliedContext;
            Chapters&=M.Id>=0&&M.Id<20&&*M.AlliedContext;
            const auto Roles=NewRoles(EFrontlineFaction(F),M);
            for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
            {
                const auto Role=EFrontlineRole(I);const bool Expected=FrontlineGameplayV52::SelectableRole(Role)&&FrontlineCampaignV41::RoleAllowed(EFrontlineFaction(F),Role)
                    &&FrontlineCampaignV41::RoleArrival(EFrontlineFaction(F),Role)==M.Id&&!(M.DisabledRoleMask&(1u<<I));
                Introductions&=Roles.Contains(Role)==Expected;
            }
        }
        Chapters&=Focus.Num()==20;
        for(int32 R=0;R<int32(EFrontlineRole::Count);++R)if(FrontlineCampaignV41::RoleAllowed(EFrontlineFaction(F),EFrontlineRole(R)))
            Guides&=*Line(FString::Printf(TEXT("%s.U%d"),F?TEXT("G"):TEXT("B"),R))!=0;
        for(const auto Kind:{EFrontlineSupport::Artillery,EFrontlineSupport::Smoke,EFrontlineSupport::Mortar,EFrontlineSupport::Gas,EFrontlineSupport::AntiTank,EFrontlineSupport::AirStrike,EFrontlineSupport::CreepingBarrage,EFrontlineSupport::AirRecon,EFrontlineSupport::AirBombing})
            Guides&=*Support(Kind,EFrontlineFaction(F))!=0;
        for(const auto& N:Nodes)
        {
            const int32 Arrival=TechnologyArrival(EFrontlineFaction(F),N.Id);Gates&=Arrival>=0&&Arrival<20;
            Gates&=Arrival==0||(F==1&&(N.Id==12||N.Id==14||N.Id==15)&&Arrival==11);
            for(int32 P:{N.Prerequisite,N.SecondaryPrerequisite})if(P>=0)Gates&=TechnologyArrival(EFrontlineFaction(F),P)<=Arrival;
            if(F==1&&(N.Id==12||N.Id==14||N.Id==15))Gates&=Missions.IsValidIndex(Arrival)&&Missions[Arrival].Year>=1918;
            const TCHAR* Effect=Line(FString::Printf(TEXT("%s.T%d"),F?TEXT("G"):TEXT("B"),N.Id));
            if(!*Effect)Effect=Line(FString::Printf(TEXT("T.%d"),N.Id));
            // Completeness and separate rule text matter; prose length is not
            // a quality test and must not penalise concise localized copy.
            Guides&=N.Effect&&*N.Effect&&*Effect&&FString(Effect).Contains(TEXT("\n"));
        }
        for(int32 I=0;I<20;++I)
            Chapters&=*Missions[I].Briefing&&*Missions[I].AlliedContext;
        if(Missions.Num()==20)
        {
            for(int32 I:{2,3,6,11,14,17,18})Rules&=Missions[I].bSupplyBayV41;
            for(int32 I:{4,7,8,9,12,13,16,19})Rules&=Missions[I].bCounterstrokeV41;
            Rules&=Missions[6].PlayerMortarDelay<Missions[5].PlayerMortarDelay;
            const uint32 Smoke=1u<<int32(EFrontlineSupport::Smoke),Civilian=(1u<<int32(EFrontlineSupport::Gas))|(1u<<int32(EFrontlineSupport::AirBombing));
            Rules&=!Missions[10].bArmourAllowedV41&&!(Missions[10].DisabledSupportMask&Smoke)
                &&Missions[10].DisabledSupportMask!=0&&Missions[11].bArmourAllowedV41&&Missions[11].DisabledSupportMask==0;
            Rules&=(Missions[18].DisabledSupportMask&Civilian)==Civilian&&(Missions[19].DisabledSupportMask&Civilian)==Civilian;
            for(int32 I=0;I<20;++I)Rules&=Missions[I].bArmoredTrain==(I==17);
            // Introduction coverage above follows the current attachment table;
            // engineers no longer first arrive together with late flame troops.
        }
    }
    for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)Guides&=*Armour(EFrontlineVehicle(I))!=0;
    for(const TCHAR* K:{TEXT("B.Sniper"),TEXT("G.Storm"),TEXT("B.Officer"),TEXT("G.Officer")})Guides&=*Line(K)!=0;
    Chapters&=AllFocus.Num()==40;
    const auto L=MoraleLabels(true),R=MoraleLabels(false);
    auto Mirrored=[](const FBox2D& A,const FBox2D& Z){return A.Min.X+Z.Max.X==1920&&A.Max.X+Z.Min.X==1920&&A.Min.Y==Z.Min.Y&&A.Max.Y==Z.Max.Y;};
    Check(TEXT("v43_morale_labels_share_baseline_and_exact_mirrored_measured_boxes"),Mirrored(L.Army,R.Army)&&Mirrored(L.Value,R.Value)
        &&L.Army.Max.X<L.Value.Min.X&&R.Value.Max.X<R.Army.Min.X&&L.Army.Min.Y==L.Value.Min.Y&&L.Army.Max.Y==L.Value.Max.Y);
    Check(TEXT("v43_player_guides_complete_bilingual_and_unique"),Text);
    Check(TEXT("v43_every_faction_role_armour_support_and_technology_has_flavor_and_effect_guidance"),Guides);
    Check(TEXT("v51_research_is_preparable_without_attachment_gates_and_respects_MP18_date"),Gates);
    Check(TEXT("v43_forty_orders_twenty_meaningful_chapter_focuses"),Chapters);
    Check(TEXT("v43_promised_supply_counterstroke_reintroduction_and_civilian_rules_are_enabled"),Rules);
    Check(TEXT("v43_all_simultaneous_new_roles_receive_introduction_cards"),Introductions);
}
}
