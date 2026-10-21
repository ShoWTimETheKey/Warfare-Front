#include "WarfareFrontlineReportV839.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFRRUArmourV82.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "WarfareFrontlineCampaignV51.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#if WITH_EDITOR
#include "FRRU/FRRUParityReviewSafetyV87.h"
#endif

namespace
{
constexpr int32 MaxCountV839=100000000;
constexpr double MaxDamageV839=1.e12;
int32 AddV839(int32 A,int32 B){return int32(FMath::Min(int64(MaxCountV839),int64(A)+int64(B)));}
double AddDamageV839(double A,double B){return FMath::Min(MaxDamageV839,A+B);}
bool TeamV839(int32 Team){return Team>=0&&Team<2;}
bool SupportV839(int32 Support){return Support>=0&&Support<10;}
bool RecordingV839(const AWarfareFrontlineGameMode* B,const FFrontlineKillSourceV65& Source)
{
    return B&&(B->Phase==EFrontlinePhase::Fighting&&B->BattleOutroWinner==INDEX_NONE
        ||B->CanResolveCommittedSupportV8217()&&B->BattleOutroWinner>=0&&SupportV839(Source.Support));
}
bool NumberV839(const FJsonObject& J,const TCHAR* Key,int32& Value)
{double N=0;if(!J.TryGetNumberField(Key,N)||!FMath::IsFinite(N)||N<0||N>MaxCountV839||N!=FMath::FloorToDouble(N))return false;Value=int32(N);return true;}
bool DamageV839(const FJsonObject& J,const TCHAR* Key,double& Value)
{double N=0;if(!J.TryGetNumberField(Key,N)||!FMath::IsFinite(N)||N<0||N>MaxDamageV839)return false;Value=N;return true;}
TSharedRef<FJsonObject> TallyV839(const FFrontlineKillTallyV65& T)
{auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("infantry"),T.Infantry);J->SetNumberField(TEXT("armour"),T.Armour);return J;}
bool ReadTallyV839(const FJsonObject& J,FFrontlineKillTallyV65& T)
{return NumberV839(J,TEXT("infantry"),T.Infantry)&&NumberV839(J,TEXT("armour"),T.Armour);}
template<size_t N> void CountsV839(FJsonObject& J,const TCHAR* Key,const int32 (&Values)[N])
{TArray<TSharedPtr<FJsonValue>> Rows;for(int32 Value:Values)Rows.Add(MakeShared<FJsonValueNumber>(Value));J.SetArrayField(Key,Rows);}
template<size_t N> bool ReadCountsV839(const FJsonObject& J,const TCHAR* Key,int32 (&Values)[N])
{const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;if(!J.TryGetArrayField(Key,Rows)||!Rows||Rows->Num()!=N)return false;for(int32 I=0;I<Rows->Num();++I){double V=0;if(!(*Rows)[I].IsValid()||!(*Rows)[I]->TryGetNumber(V)||!FMath::IsFinite(V)||V<0||V>MaxCountV839||V!=FMath::FloorToDouble(V))return false;Values[I]=int32(V);}return true;}
void ModelsV839(FJsonObject& J,const TCHAR* Key,const TMap<FString,int32>& Values)
{auto Rows=MakeShared<FJsonObject>();TArray<FString> Keys;Values.GetKeys(Keys);Keys.Sort();for(const auto& Id:Keys)Rows->SetNumberField(Id,Values[Id]);J.SetObjectField(Key,Rows);}
bool ReadModelsV839(const FJsonObject& J,const TCHAR* Key,TMap<FString,int32>& Values)
{const TSharedPtr<FJsonObject>* Rows=nullptr;if(!J.TryGetObjectField(Key,Rows)||!Rows||!Rows->IsValid()||(*Rows)->Values.Num()>5)return false;for(const auto& Entry:(*Rows)->Values){const FString Id(*Entry.Key);int32 V=0;if(!WarfareFRRUArmourV82::Definition(Id)||!NumberV839(**Rows,*Id,V))return false;Values.Add(Id,V);}return true;}
void AddSupportV839(FFrontlineSupportReportV839& A,const FFrontlineSupportReportV839& B)
{
    A.Delivered=AddV839(A.Delivered,B.Delivered);A.Rounds=AddV839(A.Rounds,B.Rounds);
    A.EnemyHullDamage=AddDamageV839(A.EnemyHullDamage,B.EnemyHullDamage);A.EnemyTrackDamage=AddDamageV839(A.EnemyTrackDamage,B.EnemyTrackDamage);
    A.FriendlyHullDamage=AddDamageV839(A.FriendlyHullDamage,B.FriendlyHullDamage);A.FriendlyTrackDamage=AddDamageV839(A.FriendlyTrackDamage,B.FriendlyTrackDamage);
    A.bDeliveryRecorded|=B.bDeliveryRecorded;A.bRoundsRecorded|=B.bRoundsRecorded;A.bDamageRecorded|=B.bDamageRecorded;
}
}

void FFrontlineDetailedRecordV839::Add(const FFrontlineDetailedRecordV839& R)
{
    bRecorded|=R.bRecorded;bIncludesEarlierUnrecordedBattles|=R.bIncludesEarlierUnrecordedBattles;FriendlyFire.Add(R.FriendlyFire);
    for(int32 I=0;I<11;++I){DeployedByRole[I]=AddV839(DeployedByRole[I],R.DeployedByRole[I]);LostByRole[I]=AddV839(LostByRole[I],R.LostByRole[I]);}
    for(int32 I=0;I<13;++I){ArmourLostByModel[I]=AddV839(ArmourLostByModel[I],R.ArmourLostByModel[I]);ArmourDestroyedByModel[I]=AddV839(ArmourDestroyedByModel[I],R.ArmourDestroyedByModel[I]);}
    for(int32 I=0;I<10;++I){FriendlySupport[I].Add(R.FriendlySupport[I]);AddSupportV839(Support[I],R.Support[I]);}
    for(const auto& Entry:R.NationalArmourLost){auto& V=NationalArmourLost.FindOrAdd(Entry.Key);V=AddV839(V,Entry.Value);}
    for(const auto& Entry:R.NationalArmourDestroyed){auto& V=NationalArmourDestroyed.FindOrAdd(Entry.Key);V=AddV839(V,Entry.Value);}
}
bool FFrontlineDetailedRecordV839::HasCounts() const
{
    if(FriendlyFire.Total()>0)return true;
    for(int32 I=0;I<11;++I)if(DeployedByRole[I]||LostByRole[I])return true;
    for(int32 I=0;I<13;++I)if(ArmourLostByModel[I]||ArmourDestroyedByModel[I])return true;
    for(const auto& E:NationalArmourLost)if(E.Value)return true;for(const auto& E:NationalArmourDestroyed)if(E.Value)return true;
    for(int32 I=0;I<10;++I){const auto& S=Support[I];if(FriendlySupport[I].Total()||S.Delivered||S.Rounds||S.EnemyHullDamage||S.EnemyTrackDamage||S.FriendlyHullDamage||S.FriendlyTrackDamage)return true;}
    return false;
}

namespace FrontlineReportV839
{
void Begin(AWarfareFrontlineGameMode* B)
{if(!B)return;for(auto& R:B->CombatRecordV65){R.DetailsV839={};R.DetailsV839.bRecorded=true;for(auto& S:R.DetailsV839.Support)S.bDeliveryRecorded=S.bRoundsRecorded=S.bDamageRecorded=true;R.InvalidateReportV86();}}
void RegisterSquad(AWarfareFrontlineGameMode* B,int32 Team,int32 Id,int32 Role,int32 Strength)
{
    if(!B||!TeamV839(Team)||Id<0||Role<0||Role>=11||Strength<0)return;
    auto& D=B->CombatRecordV65[Team].DetailsV839;if(D.Squads.Contains(Id))return;
    FFrontlineSquadReportV839 Row;Row.Id=Id;Row.OriginalRole=Role;Row.Deployed=Strength;D.Squads.Add(Id,Row);
    D.DeployedByRole[Role]=AddV839(D.DeployedByRole[Role],Strength);
    B->CombatRecordV65[Team].InvalidateReportV86();
}
void InfantryDeath(AWarfareFrontlineGameMode* B,int32 Team,const AWarfareFrontlineSoldier* Victim,const FFrontlineKillSourceV65& Source)
{
    if(!TeamV839(Team)||!RecordingV839(B,Source))return;
    auto& Loss=B->CombatRecordV65[Team].DetailsV839;
    B->CombatRecordV65[Team].InvalidateReportV86();
    if(Victim&&Victim->Team==Team)
    {
        auto* Row=Loss.Squads.Find(Victim->SquadId);const int32 Role=Row?Row->OriginalRole:int32(Victim->Role);
        if(Role>=0&&Role<11)Loss.LostByRole[Role]=AddV839(Loss.LostByRole[Role],1);
        if(Row)Row->Lost=AddV839(Row->Lost,1);
    }
    if(!TeamV839(Source.Team))return;
    auto& D=B->CombatRecordV65[Source.Team].DetailsV839;FFrontlineKillTallyV65 One;One.Infantry=1;
    B->CombatRecordV65[Source.Team].InvalidateReportV86();
    if(Source.Team==Team){D.FriendlyFire.Add(One);if(SupportV839(Source.Support))D.FriendlySupport[Source.Support].Add(One);}
    if(Source.SquadId>=0&&Source.Role>=0&&Source.Role<11)
    {
        auto& Row=D.Squads.FindOrAdd(Source.SquadId);Row.Id=Source.SquadId;if(Row.OriginalRole==INDEX_NONE)Row.OriginalRole=Source.Role;
        if(Source.Team==Team)Row.FriendlyFire.Add(One);else Row.Kills.Add(One);
    }
}
void ArmourDeath(AWarfareFrontlineGameMode* B,const AFrontlineTank* Victim,const FFrontlineKillSourceV65& Source)
{
    if(!Victim||!TeamV839(Victim->Team)||!RecordingV839(B,Source))return;
    const FString National=WarfareFRRUArmourV82::Id(Victim);const int32 Model=int32(Victim->Type);
    auto ModelCount=[&](FFrontlineDetailedRecordV839& D,bool Lost)
    {
        if(!National.IsEmpty()&&WarfareFRRUArmourV82::Definition(National)){auto& V=Lost?D.NationalArmourLost.FindOrAdd(National):D.NationalArmourDestroyed.FindOrAdd(National);V=AddV839(V,1);}
        else if(Model>=0&&Model<13){auto& V=Lost?D.ArmourLostByModel[Model]:D.ArmourDestroyedByModel[Model];V=AddV839(V,1);}
    };
    ModelCount(B->CombatRecordV65[Victim->Team].DetailsV839,true);
    B->CombatRecordV65[Victim->Team].InvalidateReportV86();
    if(!TeamV839(Source.Team))return;
    auto& D=B->CombatRecordV65[Source.Team].DetailsV839;FFrontlineKillTallyV65 One;One.Armour=1;
    B->CombatRecordV65[Source.Team].InvalidateReportV86();
    if(Source.Team==Victim->Team){D.FriendlyFire.Add(One);if(SupportV839(Source.Support))D.FriendlySupport[Source.Support].Add(One);}
    else ModelCount(D,false);
    if(Source.SquadId>=0&&Source.Role>=0&&Source.Role<11)
    {auto& Row=D.Squads.FindOrAdd(Source.SquadId);Row.Id=Source.SquadId;if(Row.OriginalRole==INDEX_NONE)Row.OriginalRole=Source.Role;if(Source.Team==Victim->Team)Row.FriendlyFire.Add(One);else Row.Kills.Add(One);}
}
void SupportDelivered(AWarfareFrontlineGameMode* B,int32 Team,int32 Kind,int32 Count)
{if(!B||!TeamV839(Team)||!SupportV839(Kind)||Count<0||B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)return;auto& S=B->CombatRecordV65[Team].DetailsV839.Support[Kind];S.bDeliveryRecorded=true;S.Delivered=AddV839(S.Delivered,Count);B->CombatRecordV65[Team].InvalidateReportV86();}
void SupportRounds(AWarfareFrontlineGameMode* B,int32 Team,int32 Kind,int32 Count)
{const auto Source=FFrontlineKillSourceV65::FireSupport(Team,Kind);if(!TeamV839(Team)||!SupportV839(Kind)||Count<0||!RecordingV839(B,Source))return;auto& S=B->CombatRecordV65[Team].DetailsV839.Support[Kind];S.bRoundsRecorded=true;S.Rounds=AddV839(S.Rounds,Count);B->CombatRecordV65[Team].InvalidateReportV86();}
void ArmourDamage(AWarfareFrontlineGameMode* B,int32 Team,const FFrontlineKillSourceV65& Source,float Hull,float Track)
{
    if(!TeamV839(Team)||!TeamV839(Source.Team)||!SupportV839(Source.Support)||!RecordingV839(B,Source)||!FMath::IsFinite(Hull)||!FMath::IsFinite(Track)||Hull<0||Track<0)return;
    auto& S=B->CombatRecordV65[Source.Team].DetailsV839.Support[Source.Support];S.bDamageRecorded=true;
    if(Team==Source.Team){S.FriendlyHullDamage=AddDamageV839(S.FriendlyHullDamage,Hull);S.FriendlyTrackDamage=AddDamageV839(S.FriendlyTrackDamage,Track);}
    else {S.EnemyHullDamage=AddDamageV839(S.EnemyHullDamage,Hull);S.EnemyTrackDamage=AddDamageV839(S.EnemyTrackDamage,Track);}
    B->CombatRecordV65[Source.Team].InvalidateReportV86();
}
FFrontlineReportDataV839 GetReportData(const AWarfareFrontlineGameMode* B)
{FFrontlineReportDataV839 R;if(!B)return R;R.Encounter=B->GetEncounterId();R.Seconds=B->Elapsed;for(int32 T=0;T<2;++T){R.Armies[T]=B->CombatRecordV65[T];R.Survivors[T]=B->LivingSoldiers(T);R.Morale[T]=FMath::RoundToInt(B->Morale[T]);}return R;}
namespace
{
FString SnapshotTextV86(const FFrontlineCombatRecordV65& Record)
{
    FJsonObject J;FrontlineCombatRecordV65::WriteArchive(J,Record);FString Text;
    FJsonSerializer::Serialize(J.GetObjectField(TEXT("combat_record_v65")).ToSharedRef(),TJsonWriterFactory<>::Create(&Text));return Text;
}
FFrontlineCombatRecordV65 LiveTailV86(const FFrontlineCombatRecordV65& Now,const FFrontlineCombatRecordV65& Before)
{
    FFrontlineCombatRecordV65 Delta;
    auto Difference=[](const FFrontlineKillTallyV65& A,const FFrontlineKillTallyV65& B){return FFrontlineKillTallyV65{FMath::Max(0,A.Infantry-B.Infantry),FMath::Max(0,A.Armour-B.Armour)};};
    Delta.Total=Difference(Now.Total,Before.Total);Delta.OtherSources=Difference(Now.OtherSources,Before.OtherSources);
    for(int32 I=0;I<11;++I)Delta.InfantrySources[I]=Difference(Now.InfantrySources[I],Before.InfantrySources[I]);
    for(int32 I=0;I<13;++I)Delta.ArmourSources[I]=Difference(Now.ArmourSources[I],Before.ArmourSources[I]);
    for(int32 I=0;I<10;++I)Delta.SupportSources[I]=Difference(Now.SupportSources[I],Before.SupportSources[I]);
    for(const auto& Entry:Now.NationalArmourSourcesV82)Delta.NationalArmourSourcesV82.Add(Entry.Key,Difference(Entry.Value,Before.NationalArmourSourcesV82.FindRef(Entry.Key)));
    Delta.InfantryLost=FMath::Max(0,Now.InfantryLost-Before.InfantryLost);Delta.ArmourLost=FMath::Max(0,Now.ArmourLost-Before.ArmourLost);
    Delta.DetailsV839=Tail(Now.DetailsV839,Before.DetailsV839);return Delta;
}
bool UpsertReturnV86(FFrontlineCombatRecordV65& Campaign,const FFrontlineCampaignBattleV86& Return)
{
    if(!Return.Encounter.IsValid()||Return.Mission<0||Return.Mission>99||Return.Winner<0||Return.Winner>1||!FMath::IsFinite(Return.Seconds)||Return.Seconds<0
        ||!Return.Armies[0].IsValid()||!Return.Armies[1].IsValid())return false;
    auto* Existing=Campaign.BattlesV86.FindByPredicate([&](const auto& Row){return Row.Encounter==Return.Encounter;});
    if(Existing)
    {
        bool Same=Existing->Mission==Return.Mission&&Existing->Winner==Return.Winner&&Existing->Seconds==Return.Seconds;
        for(int32 Team=0;Team<2&&Same;++Team)Same=Existing->Armies[Team].IsValid()&&SnapshotTextV86(*Existing->Armies[Team])==SnapshotTextV86(*Return.Armies[Team]);
        if(Same)return false;
        *Existing=Return;
    }
    else Campaign.BattlesV86.Add(Return);
    Campaign.InvalidateReportV86();
    return true;
}
}
FFrontlineCombatRecordV65 CombatTailV87(const FFrontlineCombatRecordV65& Now,const FFrontlineCombatRecordV65& Before)
{return LiveTailV86(Now,Before);}
bool CaptureCampaignBattleV86(FFrontlineCombatRecordV65& Campaign,const AWarfareFrontlineGameMode* B,int32 Mission,int32 Winner)
{
    if(!B||!B->GetEncounterId().IsValid()
        ||(FrontlineUltimateArchiveV8215::Active(B)&&!WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B)))return false;
    const bool ResultPage=B->Phase==EFrontlinePhase::Victory||B->Phase==EFrontlinePhase::Defeat;
    // Native outcomes are validated and committed before the shared result
    // phase changes. Require the actual finished mission result in that path.
    const bool NativeResult=[&](){
        if(B->Phase!=EFrontlinePhase::Fighting||!WarfareFRRUCampaignV82::Active(B))return false;
        const auto Native=WarfareFRRUWorldV82::View(B);return Native.Finished&&Native.Success==(Winner==0);
    }();
    if(!ResultPage&&!NativeResult)return false;
    FFrontlineCampaignBattleV86 Return;Return.Encounter=B->GetEncounterId();Return.Mission=Mission;Return.Winner=Winner;
    Return.Seconds=FMath::Max(0.f,B->Elapsed); // Combat duration excludes the celebration tail.
    for(int32 Team=0;Team<2;++Team)
    {auto Snapshot=B->CombatRecordV65[Team];Snapshot.BattlesV86.Reset();Snapshot.BattlesRecorded=1;Return.Armies[Team]=MakeShared<FFrontlineCombatRecordV65>(MoveTemp(Snapshot));}
    return UpsertReturnV86(Campaign,Return);
}
namespace
{
struct FCampaignReportKeyV86
{
    FGuid Campaign,Encounter;
    uint64 Stored=0,Live[2]={};
    int32 Victories=0,Defeats=0,Battles=0,Returns=0,Phase=0;
    bool Earlier=false,Overlay=false;
    bool operator==(const FCampaignReportKeyV86& R) const
    {return Campaign==R.Campaign&&Encounter==R.Encounter&&Stored==R.Stored&&Live[0]==R.Live[0]&&Live[1]==R.Live[1]
        &&Victories==R.Victories&&Defeats==R.Defeats&&Battles==R.Battles&&Returns==R.Returns&&Phase==R.Phase&&Earlier==R.Earlier&&Overlay==R.Overlay;}
};
struct FCampaignReportCacheV86
{
    FCampaignReportKeyV86 Key;
    FFrontlineReportDataV839 Data;
    uint64 Builds=0,Sorts=0;
    bool Valid=false;
};
FCampaignReportKeyV86 ReportKeyV86(const FFrontlineCombatRecordV65& Record,const FGuid& Campaign,const FGuid& Encounter,
    int32 Victories,int32 Defeats,int32 Phase,bool Overlay,const FFrontlineCombatRecordV65* Live)
{
    FCampaignReportKeyV86 Key;Key.Campaign=Campaign;Key.Encounter=Encounter;Key.Stored=Record.ReportRevisionV86;
    Key.Victories=Victories;Key.Defeats=Defeats;Key.Battles=Record.BattlesRecorded;Key.Returns=Record.BattlesV86.Num();
    Key.Earlier=Record.bIncludesEarlierUnrecordedBattles;Key.Phase=Phase;Key.Overlay=Overlay;
    if(Overlay&&Live)for(int32 Team=0;Team<2;++Team)Key.Live[Team]=Live[Team].ReportRevisionV86;
    return Key;
}
void BuildCampaignReportV86(FCampaignReportCacheV86& Cache,const FCampaignReportKeyV86& Key,
    const FFrontlineCombatRecordV65& Saved,const FFrontlineCombatRecordV65* Live)
{
    auto& Data=Cache.Data;Data={};Data.CampaignV86=true;++Cache.Builds;
    auto Record=Saved;
    // Read-only overlay of already committed support still landing during the return sheet.
    // The archive owns the saved baseline; no gameplay clocks, counters or files change here.
    if(Key.Overlay&&Live)
    {
        const auto* Current=Record.BattlesV86.FindByPredicate([&](const auto& Row){return Row.Encounter==Key.Encounter;});
        if(Current&&Current->Armies[0].IsValid())
        {
            Record.Add(LiveTailV86(Live[0],*Current->Armies[0]));
            // Local display copy: no serialized equality comparison in the HUD frame.
            auto* Display=Record.BattlesV86.FindByPredicate([&](const auto& Row){return Row.Encounter==Key.Encounter;});
            for(int32 Team=0;Team<2;++Team)
            {auto Side=Live[Team];Side.BattlesV86.Reset();Side.BattlesRecorded=1;Display->Armies[Team]=MakeShared<FFrontlineCombatRecordV65>(MoveTemp(Side));}
        }
    }
    Data.Encounter=Key.Campaign;Data.Armies[0]=Record;Data.Armies[0].BattlesV86.Reset();Data.Armies[0].DetailsV839.Squads.Reset();
    Data.BattlesV86=Record.bIncludesEarlierUnrecordedBattles?FMath::Max(Record.BattlesRecorded,Key.Victories+Key.Defeats):Record.BattlesRecorded;
    Data.VictoriesV86=Key.Victories;Data.DefeatsV86=Key.Defeats;
    Data.bEarlierAttemptsUnrecordedV86=Record.bIncludesEarlierUnrecordedBattles||Record.BattlesV86.Num()<Data.BattlesV86;
    if(Data.bEarlierAttemptsUnrecordedV86)Data.Armies[0].DetailsV839.bIncludesEarlierUnrecordedBattles=true;
    const int32 Earlier=FMath::Max(0,Data.BattlesV86-Record.BattlesV86.Num());
    int32 Identity[2]={};
    for(int32 Attempt=0;Attempt<Record.BattlesV86.Num();++Attempt)
    {
        const auto& Return=Record.BattlesV86[Attempt];Data.Seconds+=Return.Seconds;
        for(int32 Team=0;Team<2;++Team)if(Return.Armies[Team].IsValid())
        {
            const auto& Side=*Return.Armies[Team];if(Team==1)Data.Armies[1].Add(Side);
            TArray<int32> Keys;Side.DetailsV839.Squads.GetKeys(Keys);Keys.Sort();++Cache.Sorts;
            for(int32 SectionKey:Keys){auto Section=Side.DetailsV839.Squads[SectionKey];Section.BattleNumberV86=Earlier+Attempt+1;Data.Armies[Team].DetailsV839.Squads.Add(++Identity[Team],Section);}
        }
    }
    if(Data.bEarlierAttemptsUnrecordedV86)Data.Armies[1].DetailsV839.bIncludesEarlierUnrecordedBattles=true;
    for(int32 Team=0;Team<2;++Team)
    {
        auto& Sections=Data.SortedSquadsV86[Team];
        for(const auto& Entry:Data.Armies[Team].DetailsV839.Squads)
            if(Entry.Value.OriginalRole>=0&&Entry.Value.OriginalRole<11)Sections.Add(Entry.Value);
        Sections.Sort([](const auto& A,const auto& B){return A.Kills.Total()!=B.Kills.Total()?A.Kills.Total()>B.Kills.Total():A.Id<B.Id;});++Cache.Sorts;
    }
    Cache.Key=Key;Cache.Valid=true;
}
const FFrontlineReportDataV839& ReadCampaignReportV86(FCampaignReportCacheV86& Cache,const FCampaignReportKeyV86& Key,
    const FFrontlineCombatRecordV65& Record,const FFrontlineCombatRecordV65* Live=nullptr)
{
    if(!Cache.Valid||!(Cache.Key==Key))BuildCampaignReportV86(Cache,Key,Record,Live);
    return Cache.Data;
}
}
const FFrontlineReportDataV839& GetCampaignReportV86(const AWarfareFrontlineGameMode* B)
{
    static FFrontlineReportDataV839 Empty;
    const auto* Campaign=B?B->GetCurrentCampaign():nullptr;
    const bool National=WarfareFRRUCampaignV82::Active(B);FGuid Id;int32 Victories=0,Defeats=0;
    const FFrontlineCombatRecordV65* Record=nullptr;
    if(National)
    {if(!WarfareFRRUCampaignV82::CampaignReportMetadataV87(B,Id,Victories,Defeats))return Empty;Record=WarfareFRRUCampaignV82::CampaignCombatRecordV87(B);}
    else if(Campaign){Id=Campaign->Id;Victories=Campaign->Research.Victories;Defeats=Campaign->Research.Defeats;Record=&Campaign->CombatRecordV65;}
    if(!Record)return Empty;
    // The small key uses event revisions, not a ledger scan or serialized JSON hash.
    // Archive decode/reset creates a fresh token, even when the campaign ID is reused.
    static TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FCampaignReportCacheV86> Caches;
    for(auto It=Caches.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& Cache=Caches.FindOrAdd(const_cast<AWarfareFrontlineGameMode*>(B));
    const bool Overlay=(B->Phase==EFrontlinePhase::Victory||B->Phase==EFrontlinePhase::Defeat)&&!FrontlineUltimateArchiveV8215::Active(B);
    const auto Key=ReportKeyV86(*Record,Id,B->GetEncounterId(),Victories,Defeats,int32(B->Phase),Overlay,B->CombatRecordV65);
    return ReadCampaignReportV86(Cache,Key,*Record,B->CombatRecordV65);
}
namespace
{
FFrontlineCampaignBattleV86 SyntheticReturnV86(int32 Mission,int32 Winner,int32 Kills,int32 Losses)
{
    FFrontlineCampaignBattleV86 Return;Return.Encounter=FGuid::NewGuid();Return.Mission=Mission;Return.Winner=Winner;Return.Seconds=280+Mission*11;
    for(int32 Team=0;Team<2;++Team)
    {
        auto Side=MakeShared<FFrontlineCombatRecordV65>();Side->BattlesRecorded=1;Side->Total.Infantry=Team==0?Kills:Losses;
        Side->Total.Armour=Team==0?2:1;Side->InfantrySources[0]=Side->Total;Side->InfantryLost=Team==0?Losses:Kills;Side->ArmourLost=Team==0?1:2;
        auto& D=Side->DetailsV839;D.bRecorded=true;D.DeployedByRole[0]=Side->InfantryLost+12;D.LostByRole[0]=Side->InfantryLost;
        D.ArmourLostByModel[0]=Side->ArmourLost;D.ArmourDestroyedByModel[2]=Side->Total.Armour;
        for(auto& Support:D.Support)Support.bDeliveryRecorded=Support.bRoundsRecorded=Support.bDamageRecorded=true;
        D.Support[0].Delivered=2;D.Support[0].Rounds=24;D.Support[0].EnemyHullDamage=36.5;
        FFrontlineSquadReportV839 Section;Section.Id=1;Section.OriginalRole=0;Section.Deployed=D.DeployedByRole[0];Section.Lost=Side->InfantryLost;Section.Kills=Side->Total;D.Squads.Add(1,Section);
        Return.Armies[Team]=MoveTemp(Side);
    }
    return Return;
}
FFrontlineCombatRecordV65 SyntheticCampaignV86()
{
    FFrontlineCombatRecordV65 R;
    for(int32 I=0;I<3;++I){const auto Return=SyntheticReturnV86(I,I==1?1:0,24+I*5,7+I);R.Add(*Return.Armies[0]);UpsertReturnV86(R,Return);}return R;
}
void RunCampaignCacheChecksV86(TFunctionRef<void(const FString&,bool)> Check)
{
    // In-memory load fixture exercising the production merger/sort/cache, not a
    // substitute implementation or a natural/player-profile campaign claim.
    FFrontlineCombatRecordV65 Large;
    for(int32 Attempt=0;Attempt<20;++Attempt)
    {
        auto Return=SyntheticReturnV86(Attempt,Attempt%2,24,7);
        for(int32 Team=0;Team<2;++Team)
        {
            auto& Side=*Return.Armies[Team];auto& Detail=Side.DetailsV839;Detail.Squads.Reset();Side.Total={};
            for(int32 Id=1;Id<=256;++Id)
            {
                FFrontlineSquadReportV839 Section;Section.Id=Id;Section.OriginalRole=0;Section.Deployed=12;Section.Lost=4;
                Section.Kills={Id%17,Id%64==0?1:0};Side.Total.Add(Section.Kills);Detail.Squads.Add(Id,Section);
            }
            Side.InfantrySources[0]=Side.Total;Side.InfantryLost=1024;Detail.DeployedByRole[0]=3072;Detail.LostByRole[0]=1024;
            Detail.ArmourDestroyedByModel[2]=Side.Total.Armour;Side.InvalidateReportV86();
        }
        Large.Add(*Return.Armies[0]);UpsertReturnV86(Large,Return);
    }
    const FGuid CampaignId=FGuid::NewGuid(),Encounter=Large.BattlesV86.Last().Encounter;
    FCampaignReportCacheV86 Cache;
    auto Key=ReportKeyV86(Large,CampaignId,Encounter,10,10,int32(EFrontlinePhase::Briefing),false,nullptr);
    const uint64 SavedRevision=Large.ReportRevisionV86;
    const auto* First=&ReadCampaignReportV86(Cache,Key,Large);
    Check(TEXT("v86_campaign_cache_actual_merger_handles_10240_section_rows"),First->SortedSquadsV86[0].Num()==5120&&First->SortedSquadsV86[1].Num()==5120&&First->BattlesV86==20);
    const uint64 Sorts=Cache.Sorts;
    bool Stable=true;for(int32 Frame=0;Frame<64;++Frame)Stable&=(&ReadCampaignReportV86(Cache,Key,Large)==First);
    Check(TEXT("v86_campaign_cache_64_unchanged_reads_do_not_rebuild_resort_or_mutate_archive"),Stable&&Cache.Builds==1&&Cache.Sorts==Sorts&&Sorts==42&&Large.ReportRevisionV86==SavedRevision);

    auto Changed=Large.BattlesV86[0];Changed.Armies[0]=MakeShared<FFrontlineCombatRecordV65>(*Changed.Armies[0]);
    auto& Section=Changed.Armies[0]->DetailsV839.Squads[5];const int32 Extra=90000-Section.Kills.Infantry;
    Section.Kills.Infantry+=Extra;Changed.Armies[0]->Total.Infantry+=Extra;Changed.Armies[0]->InfantrySources[0].Infantry+=Extra;
    const uint64 BeforeWrite=Cache.Builds;const bool ChangedReturn=UpsertReturnV86(Large,Changed);
    Key=ReportKeyV86(Large,CampaignId,Encounter,10,10,int32(EFrontlinePhase::Briefing),false,nullptr);
    const auto& Updated=ReadCampaignReportV86(Cache,Key,Large);
    Check(TEXT("v86_campaign_cache_snapshot_write_refreshes_sorted_sections"),ChangedReturn&&Cache.Builds==BeforeWrite+1&&!Updated.SortedSquadsV86[0].IsEmpty()&&Updated.SortedSquadsV86[0][0].Kills.Infantry==90000&&Updated.SortedSquadsV86[0][0].BattleNumberV86==1);
    const uint64 BeforeAdd=Cache.Builds;const int32 BeforeRounds=Updated.Armies[0].DetailsV839.Support[0].Rounds;
    FFrontlineCombatRecordV65 Delta;Delta.DetailsV839.Support[0].Rounds=3;Large.Add(Delta);
    Key=ReportKeyV86(Large,CampaignId,Encounter,10,10,int32(EFrontlinePhase::Briefing),false,nullptr);
    Check(TEXT("v86_campaign_cache_aggregate_write_refreshes_totals"),ReadCampaignReportV86(Cache,Key,Large).Armies[0].DetailsV839.Support[0].Rounds==BeforeRounds+3&&Cache.Builds==BeforeAdd+1);

    FFrontlineCombatRecordV65 Live[2]={*Large.BattlesV86.Last().Armies[0],*Large.BattlesV86.Last().Armies[1]};
    Key=ReportKeyV86(Large,CampaignId,Encounter,10,10,int32(EFrontlinePhase::Victory),true,Live);
    const int32 TailBefore=ReadCampaignReportV86(Cache,Key,Large,Live).Armies[0].DetailsV839.Support[0].Rounds;
    const uint64 BeforeTail=Cache.Builds;Live[0].DetailsV839.Support[0].Rounds+=2;Live[0].InvalidateReportV86();
    Live[1].DetailsV839.Squads[9].Lost+=1;Live[1].InvalidateReportV86();
    Key=ReportKeyV86(Large,CampaignId,Encounter,10,10,int32(EFrontlinePhase::Victory),true,Live);
    const auto& TailData=ReadCampaignReportV86(Cache,Key,Large,Live);
    const auto* EnemyRow=TailData.SortedSquadsV86[1].FindByPredicate([](const auto& Row){return Row.BattleNumberV86==20&&Row.Id==9;});
    Check(TEXT("v86_campaign_cache_both_army_live_tail_events_refresh_display_once"),TailData.Armies[0].DetailsV839.Support[0].Rounds==TailBefore+2&&EnemyRow&&EnemyRow->Lost==5&&Cache.Builds==BeforeTail+1);
    const uint64 TailSorts=Cache.Sorts;ReadCampaignReportV86(Cache,Key,Large,Live);
    Check(TEXT("v86_campaign_cache_unchanged_live_tail_does_not_resort"),Cache.Builds==BeforeTail+1&&Cache.Sorts==TailSorts);

    const uint64 BeforeSwitch=Cache.Builds;Key.Campaign=FGuid::NewGuid();ReadCampaignReportV86(Cache,Key,Large,Live);
    Check(TEXT("v86_campaign_cache_archive_switch_with_identical_counts_refreshes_identity"),Cache.Builds==BeforeSwitch+1&&Cache.Data.Encounter==Key.Campaign);
    FJsonObject Legacy;FrontlineCombatRecordV65::WriteArchive(Legacy,Large);Legacy.GetObjectField(TEXT("combat_record_v65"))->RemoveField(TEXT("campaign_returns_v86"));
    const uint64 OldToken=Large.ReportRevisionV86,BeforeLoad=Cache.Builds;
    const bool Loaded=FrontlineCombatRecordV65::ReadArchive(Legacy,Large,true);
    Key=ReportKeyV86(Large,CampaignId,Encounter,10,10,int32(EFrontlinePhase::Briefing),false,nullptr);
    const auto& OldData=ReadCampaignReportV86(Cache,Key,Large);
    Check(TEXT("v86_campaign_cache_same_id_legacy_archive_decode_refreshes_unrecorded_display"),Loaded&&Large.ReportRevisionV86!=OldToken&&Cache.Builds==BeforeLoad+1&&OldData.SortedSquadsV86[0].IsEmpty()&&OldData.bEarlierAttemptsUnrecordedV86&&OldData.Armies[0].Total.Total()==Large.Total.Total());
    const uint64 BeforeReset=Large.ReportRevisionV86;Large={};
    Check(TEXT("v86_campaign_cache_reset_creates_new_nonserialized_content_token"),Large.ReportRevisionV86!=BeforeReset&&!Legacy.GetObjectField(TEXT("combat_record_v65"))->HasField(TEXT("report_revision_v86")));
}
}
#if WITH_EDITOR
bool PrepareNationalCampaignVisualV87(AWarfareFrontlineGameMode* B,EFRRUNationV81 N,TFunctionRef<void(const FString&,bool)> Check)
{
    FString User;const bool Safe=B&&B->IsProgressionVolatile()&&FRRUParityReviewSafetyV87::UserDir(User);
    Check(TEXT("v87_national_report_visual_requires_exact_disposable_profile"),Safe);if(!Safe)return false;
    FFrontlineCombatRecordV65 Record;
    for(int32 I=0;I<10;++I){const auto Return=SyntheticReturnV86(I,0,24+I*5,7+I);Record.Add(*Return.Armies[0]);UpsertReturnV86(Record,Return);}
    const bool Seeded=WarfareFRRUCampaignV82::PrepareParityCampaignReportV87(B,N,Record);
    Check(TEXT("v87_national_report_explicit_synthetic_record_seeded"),Seeded);if(!Seeded)return false;
    const auto& Actual=GetCampaignReportV86(B);
    const auto* Source=WarfareFRRUCampaignV82::CampaignCombatRecordV87(B);
    Check(TEXT("v87_national_report_uses_real_national_record_and_both_sides"),Source&&Actual.CampaignV86
        &&Actual.Encounter==WarfareFRRUCampaignV82::CampaignId(B)&&Actual.Armies[0].Total.Infantry==Record.Total.Infantry
        &&Actual.Armies[1].Total.Infantry>0&&Source->BattlesV86.Num()==10&&Actual.SortedSquadsV86[0].Num()==10&&Actual.SortedSquadsV86[1].Num()==10);
    Check(TEXT("v87_national_report_fixture_exact_nonzero_return_totals"),Actual.Armies[0].Total.Infantry==465
        &&Actual.Armies[0].Total.Armour==20&&Actual.Armies[0].InfantryLost==115&&Actual.Armies[0].ArmourLost==10
        &&Actual.Armies[1].Total.Infantry==115&&Actual.BattlesV86==10);
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_REPORT_FIXTURE nation=%d infantry=%d armour=%d lost=%d armour_lost=%d battles=%d"),
        int32(N),Actual.Armies[0].Total.Infantry,Actual.Armies[0].Total.Armour,Actual.Armies[0].InfantryLost,Actual.Armies[0].ArmourLost,Actual.BattlesV86);
    return Seeded;
}
#endif
void RunCampaignChecksV86(TFunctionRef<void(const FString&,bool)> Check)
{
    FrontlineProgressionReviewV8219::RunCapacityChecksV86(Check);
    auto Campaign=SyntheticCampaignV86();const auto First=Campaign.BattlesV86[0];const auto Totals=Campaign.Total;
    Check(TEXT("v86_campaign_duplicate_snapshot_does_not_duplicate_attempt_or_totals"),!UpsertReturnV86(Campaign,First)&&Campaign.BattlesV86.Num()==3&&Campaign.Total.Infantry==Totals.Infantry);
    auto Tail=First;Tail.Armies[0]=MakeShared<FFrontlineCombatRecordV65>(*First.Armies[0]);Tail.Armies[0]->Total.Infantry+=2;Tail.Armies[0]->InfantrySources[0].Infantry+=2;Tail.Armies[0]->DetailsV839.Squads[1].Kills.Infantry+=2;
    Check(TEXT("v86_campaign_outro_replaces_one_receipt_without_counting_battle_again"),UpsertReturnV86(Campaign,Tail)&&Campaign.BattlesV86.Num()==3&&Campaign.BattlesV86[0].Armies[0]->Total.Infantry==26&&Campaign.Total.Infantry==Totals.Infantry);
    const auto DisplayTail=LiveTailV86(*Tail.Armies[0],*First.Armies[0]);auto DisplayOnly=Campaign;DisplayOnly.Add(DisplayTail);
    Check(TEXT("v86_campaign_read_only_live_tail_counts_once_without_changing_archive"),DisplayOnly.Total.Infantry==Totals.Infantry+2&&Campaign.Total.Infantry==Totals.Infantry&&LiveTailV86(*Tail.Armies[0],*Tail.Armies[0]).Total.Total()==0&&DisplayOnly.BattlesRecorded==3);
    FJsonObject J;FrontlineCombatRecordV65::WriteArchive(J,Campaign);FFrontlineCombatRecordV65 Read;
    Check(TEXT("v86_campaign_double_army_detail_and_sections_roundtrip"),FrontlineCombatRecordV65::ReadArchive(J,Read,false)&&Read.BattlesV86.Num()==3&&Read.BattlesV86[1].Winner==1&&Read.BattlesV86[2].Armies[1]->InfantryLost==34&&Read.BattlesV86[0].Armies[0]->DetailsV839.Squads[1].Kills.Infantry==26&&Read.BattlesV86[2].Armies[0]->DetailsV839.Support[0].Rounds==24);
    Check(TEXT("v86_campaign_reused_section_ids_remain_separate_attempts"),Read.BattlesV86.Num()==3&&Read.BattlesV86[0].Armies[0]->DetailsV839.Squads[1].Kills.Infantry==26&&Read.BattlesV86[1].Armies[0]->DetailsV839.Squads[1].Kills.Infantry==29);
    auto O=J.GetObjectField(TEXT("combat_record_v65"));O->RemoveField(TEXT("campaign_returns_v86"));FFrontlineCombatRecordV65 Old;
    Check(TEXT("v86_legacy_total_survives_without_inventing_old_attempts"),FrontlineCombatRecordV65::ReadArchive(J,Old,true)&&Old.Total.Infantry==Totals.Infantry&&Old.BattlesV86.IsEmpty()&&Old.BattlesRecorded==3);
    FrontlineCombatRecordV65::WriteArchive(J,Campaign);auto Rows=J.GetObjectField(TEXT("combat_record_v65"))->GetArrayField(TEXT("campaign_returns_v86"));const auto Duplicate=Rows[0];Rows.Add(Duplicate);J.GetObjectField(TEXT("combat_record_v65"))->SetArrayField(TEXT("campaign_returns_v86"),Rows);
    Check(TEXT("v86_campaign_duplicate_receipt_rejected_without_mutating_decoded_record"),!FrontlineCombatRecordV65::ReadArchive(J,Read,false)&&Read.BattlesV86.Num()==3);
    FrontlineCombatRecordV65::WriteArchive(J,Campaign);auto Child=J.GetObjectField(TEXT("combat_record_v65"))->GetArrayField(TEXT("campaign_returns_v86"))[0]->AsObject()->GetObjectField(TEXT("our_army"))->GetObjectField(TEXT("combat_record_v65"));Child->SetArrayField(TEXT("campaign_returns_v86"),TArray<TSharedPtr<FJsonValue>>{});
    Check(TEXT("v86_campaign_nested_ledger_rejected"),!FrontlineCombatRecordV65::ReadArchive(J,Read,false)&&Read.BattlesV86.Num()==3);
    FrontlineCombatRecordV65::WriteArchive(J,Campaign);J.GetObjectField(TEXT("combat_record_v65"))->GetArrayField(TEXT("campaign_returns_v86"))[0]->AsObject()->SetNumberField(TEXT("seconds"),-1);
    Check(TEXT("v86_campaign_invalid_duration_rejected"),!FrontlineCombatRecordV65::ReadArchive(J,Read,false));
    auto Invalid=First;Invalid.Encounter.Invalidate();Check(TEXT("v86_campaign_invalid_encounter_cannot_be_captured"),!UpsertReturnV86(Campaign,Invalid));
    RunCampaignCacheChecksV86(Check);
}
void RunLiveCacheWriterChecksV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString User;
    auto* Campaign=B?const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()):nullptr;
    const bool Allowed=B&&B->GetWorld()&&B->HasActorBegunPlay()&&Campaign&&B->GetEncounterId().IsValid()
        &&B->IsProgressionVolatile()&&B->Phase==EFrontlinePhase::Fighting&&B->AssetFailure.IsEmpty()
        &&(B->GetPlayerFaction()==EFrontlineFaction::British||B->GetPlayerFaction()==EFrontlineFaction::German)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&(FParse::Param(FCommandLine::Get(),TEXT("WFV87FocusedOnly"))||FParse::Param(FCommandLine::Get(),TEXT("WFV87Smoke")))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(User)&&!FrontlineUltimateArchiveV8215::Active(B)&&!WarfareFRRUCampaignV82::Active(B);
    Check(TEXT("v87_campaign_cache_live_writers_require_real_disposable_battle_ready"),Allowed);if(!Allowed)return;
    const FFrontlineCampaignProgress OldCampaign=*Campaign;
    const FFrontlineCombatRecordV65 OldRecords[2]={B->CombatRecordV65[0],B->CombatRecordV65[1]};
    const auto OldPhase=B->Phase;const int32 OldWinner=B->BattleOutroWinner;
    const auto OldDisplay=GetCampaignReportV86(B);
    TArray<AWarfareFrontlineSoldier*> Actors;
    ON_SCOPE_EXIT
    {
        for(auto* Actor:Actors)if(IsValid(Actor))Actor->Destroy();
        for(int32 Team=0;Team<2;++Team)B->CombatRecordV65[Team]=OldRecords[Team];
        *Campaign=OldCampaign;B->Phase=OldPhase;B->BattleOutroWinner=OldWinner;
        const auto& Restored=GetCampaignReportV86(B);
        Check(TEXT("v87_campaign_cache_live_writer_probe_restores_campaign_records_and_cached_display"),
            Campaign->Id==OldCampaign.Id&&Campaign->CombatRecordV65.ReportRevisionV86==OldCampaign.CombatRecordV65.ReportRevisionV86
            &&B->CombatRecordV65[0].ReportRevisionV86==OldRecords[0].ReportRevisionV86&&B->CombatRecordV65[1].ReportRevisionV86==OldRecords[1].ReportRevisionV86
            &&B->Phase==OldPhase&&B->BattleOutroWinner==OldWinner&&Restored.Encounter==OldDisplay.Encounter
            &&Restored.Armies[0].Total.Total()==OldDisplay.Armies[0].Total.Total()
            &&Restored.SortedSquadsV86[0].Num()==OldDisplay.SortedSquadsV86[0].Num()&&Restored.SortedSquadsV86[1].Num()==OldDisplay.SortedSquadsV86[1].Num());
    };
    // Only report writer methods are called. These real world actors provide
    // victim identity; this probe does not run fatal-hit, morale or reward code.
    for(int32 Team=0;Team<2;++Team)
    {
        FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Actor=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(FVector(0,Team*300.f,10000.f),FRotator::ZeroRotator,Spawn);
        if(!Actor){Check(TEXT("v87_campaign_cache_live_writer_victim_actor_spawn"),false);return;}
        Actor->SetActorTickEnabled(false);Actor->SetActorHiddenInGame(true);Actor->SetActorEnableCollision(false);
        Actor->Team=Team;Actor->SquadId=8700000+Team;Actor->Role=EFrontlineRole::Rifle;Actors.Add(Actor);
        B->CombatRecordV65[Team]={};
    }
    Begin(B);for(int32 Team=0;Team<2;++Team)RegisterSquad(B,Team,Actors[Team]->SquadId,int32(Actors[Team]->Role),1);
    B->Phase=EFrontlinePhase::Victory;B->BattleOutroWinner=0;
    Campaign->CombatRecordV65={};Campaign->CombatRecordV65.Add(B->CombatRecordV65[0]);Campaign->CombatRecordV65.BattlesRecorded=1;
    const bool Captured=CaptureCampaignBattleV86(Campaign->CombatRecordV65,B,0,0);
    Check(TEXT("v87_campaign_cache_live_writer_current_attempt_baseline_captured"),Captured);if(!Captured)return;
    const auto Primed=GetCampaignReportV86(B);
    Check(TEXT("v87_campaign_cache_live_writer_actual_getter_is_primed_before_events"),Primed.BattlesV86==1&&Primed.SortedSquadsV86[0].Num()==1&&Primed.SortedSquadsV86[1].Num()==1);
    const uint64 ArchiveRevision=Campaign->CombatRecordV65.ReportRevisionV86;
    const int32 Artillery=int32(EFrontlineSupport::Artillery);
    FFrontlineKillSourceV65 Sources[2]={FFrontlineKillSourceV65::FireSupport(0,Artillery),FFrontlineKillSourceV65::FireSupport(1,Artillery)};
    for(int32 Team=0;Team<2;++Team)
    {
        Sources[Team].SquadId=Actors[Team]->SquadId;Sources[Team].Role=int32(Actors[Team]->Role);
        const uint64 Before=B->CombatRecordV65[Team].ReportRevisionV86;
        SupportRounds(B,Team,Artillery,Team+1);
        const auto& After=GetCampaignReportV86(B);
        Check(FString::Printf(TEXT("v87_campaign_cache_real_support_round_writer_side_%d_invalidates_and_refreshes"),Team),
            B->CombatRecordV65[Team].ReportRevisionV86!=Before&&After.Armies[Team].DetailsV839.Support[Artillery].Rounds==Team+1
            &&Campaign->CombatRecordV65.ReportRevisionV86==ArchiveRevision);
        const uint64 BeforeDamage=B->CombatRecordV65[Team].ReportRevisionV86;
        ArmourDamage(B,1-Team,Sources[Team],7.25f+Team,3.5f+Team);
        const auto& Damaged=GetCampaignReportV86(B);const auto& Support=Damaged.Armies[Team].DetailsV839.Support[Artillery];
        Check(FString::Printf(TEXT("v87_campaign_cache_real_armour_damage_writer_side_%d_invalidates_and_refreshes"),Team),
            B->CombatRecordV65[Team].ReportRevisionV86!=BeforeDamage&&Support.EnemyHullDamage==7.25+Team&&Support.EnemyTrackDamage==3.5+Team
            &&Support.Rounds==Team+1&&Campaign->CombatRecordV65.ReportRevisionV86==ArchiveRevision);
    }
    for(int32 Team=0;Team<2;++Team)
    {
        const uint64 BeforeSource=B->CombatRecordV65[Team].ReportRevisionV86,BeforeVictim=B->CombatRecordV65[1-Team].ReportRevisionV86;
        InfantryDeath(B,1-Team,Actors[1-Team],Sources[Team]);
        const auto& After=GetCampaignReportV86(B);
        const auto* SourceRow=After.SortedSquadsV86[Team].FindByPredicate([&](const auto& Row){return Row.Id==Actors[Team]->SquadId;});
        const auto* VictimRow=After.SortedSquadsV86[1-Team].FindByPredicate([&](const auto& Row){return Row.Id==Actors[1-Team]->SquadId;});
        Check(FString::Printf(TEXT("v87_campaign_cache_real_infantry_death_writer_side_%d_refreshes_both_armies"),Team),
            B->CombatRecordV65[Team].ReportRevisionV86!=BeforeSource&&B->CombatRecordV65[1-Team].ReportRevisionV86!=BeforeVictim
            &&SourceRow&&VictimRow&&SourceRow->Kills.Infantry==1&&VictimRow->Lost==1&&After.Armies[1-Team].DetailsV839.LostByRole[0]==1
            &&Campaign->CombatRecordV65.ReportRevisionV86==ArchiveRevision);
    }
    const auto DisplayOnly=GetCampaignReportV86(B);
    const auto* Saved=Campaign->CombatRecordV65.BattlesV86.FindByPredicate([&](const auto& Row){return Row.Encounter==B->GetEncounterId();});
    Check(TEXT("v87_campaign_cache_real_live_writer_overlay_does_not_write_saved_snapshot"),Saved&&Saved->Armies[0].IsValid()&&Saved->Armies[1].IsValid()
        &&Saved->Armies[0]->DetailsV839.Support[Artillery].Rounds==0&&Saved->Armies[1]->DetailsV839.Support[Artillery].Rounds==0
        &&DisplayOnly.Armies[0].DetailsV839.Support[Artillery].Rounds==1&&DisplayOnly.Armies[1].DetailsV839.Support[Artillery].Rounds==2);
}
bool PrepareCampaignVisualV86(AWarfareFrontlineGameMode* B,APlayerController* PC,int32 Tab,int32 Team)
{
    FString Dir;if(!B||!PC||!B->IsProgressionVolatile()||WarfareFRRUCampaignV82::Active(B)||FrontlineUltimateArchiveV8215::Active(B)
        ||(!FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV86Review")))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)||Tab<0||Tab>5||Team<0||Team>1)return false;
    auto* Campaign=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());if(!Campaign)return false;
    // Labelled UI sample on a disposable profile; no FinishBattle, rewards or SaveProgression.
    Campaign->CombatRecordV65=SyntheticCampaignV86();
    for(int32 I=3;I<20;++I){const auto Return=SyntheticReturnV86(I,I==6||I==14?1:0,24+I*5,7+I);Campaign->CombatRecordV65.Add(*Return.Armies[0]);UpsertReturnV86(Campaign->CombatRecordV65,Return);}
    Campaign->CompletedMissions=B->GetCampaignMissions().Num();Campaign->Research.Victories=17;Campaign->Research.Defeats=3;
    B->Phase=EFrontlinePhase::Briefing;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;
    OpenCampaignV86(PC,B);auto& V=View(PC);V.Tab=Tab;V.Team=Team;V.Offset=0;return Active(PC,B);
}
void OpenCampaignV86(APlayerController* PC,const AWarfareFrontlineGameMode* B)
{
    if(!PC||!B||FrontlineUltimateArchiveV8215::Active(B)||!FrontlineCampaignV51::CanOpenEnding(B))return;
    const FGuid Id=WarfareFRRUCampaignV82::Active(B)?WarfareFRRUCampaignV82::CampaignId(B):B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();
    if(!Id.IsValid())return;
    auto& V=View(PC);V={};V.Encounter=B->GetEncounterId();V.Campaign=Id;V.CampaignV86=V.Open=true;
}
FFrontlineDetailedRecordV839 Tail(const FFrontlineDetailedRecordV839& N,const FFrontlineDetailedRecordV839& P)
{
    FFrontlineDetailedRecordV839 D;D.bRecorded=N.bRecorded;D.bIncludesEarlierUnrecordedBattles=N.bIncludesEarlierUnrecordedBattles;
    auto Count=[](int32 A,int32 B){return FMath::Max(0,A-B);};
    auto Tally=[&](const FFrontlineKillTallyV65& A,const FFrontlineKillTallyV65& B){return FFrontlineKillTallyV65{Count(A.Infantry,B.Infantry),Count(A.Armour,B.Armour)};};
    D.FriendlyFire=Tally(N.FriendlyFire,P.FriendlyFire);
    for(int32 I=0;I<11;++I){D.DeployedByRole[I]=Count(N.DeployedByRole[I],P.DeployedByRole[I]);D.LostByRole[I]=Count(N.LostByRole[I],P.LostByRole[I]);}
    for(int32 I=0;I<13;++I){D.ArmourLostByModel[I]=Count(N.ArmourLostByModel[I],P.ArmourLostByModel[I]);D.ArmourDestroyedByModel[I]=Count(N.ArmourDestroyedByModel[I],P.ArmourDestroyedByModel[I]);}
    for(const auto& E:N.NationalArmourLost)D.NationalArmourLost.Add(E.Key,Count(E.Value,P.NationalArmourLost.FindRef(E.Key)));
    for(const auto& E:N.NationalArmourDestroyed)D.NationalArmourDestroyed.Add(E.Key,Count(E.Value,P.NationalArmourDestroyed.FindRef(E.Key)));
    for(int32 I=0;I<10;++I)
    {
        D.FriendlySupport[I]=Tally(N.FriendlySupport[I],P.FriendlySupport[I]);const auto& A=N.Support[I];const auto& B=P.Support[I];auto& S=D.Support[I];
        S.Delivered=Count(A.Delivered,B.Delivered);S.Rounds=Count(A.Rounds,B.Rounds);
        S.EnemyHullDamage=FMath::Max(0.,A.EnemyHullDamage-B.EnemyHullDamage);S.EnemyTrackDamage=FMath::Max(0.,A.EnemyTrackDamage-B.EnemyTrackDamage);
        S.FriendlyHullDamage=FMath::Max(0.,A.FriendlyHullDamage-B.FriendlyHullDamage);S.FriendlyTrackDamage=FMath::Max(0.,A.FriendlyTrackDamage-B.FriendlyTrackDamage);
        S.bDeliveryRecorded=A.bDeliveryRecorded;S.bRoundsRecorded=A.bRoundsRecorded;S.bDamageRecorded=A.bDamageRecorded;
    }
    return D;
}
void WriteArchive(FJsonObject& J,const FFrontlineDetailedRecordV839& R)
{
    if(!R.bRecorded&&!R.bIncludesEarlierUnrecordedBattles)return;
    auto O=MakeShared<FJsonObject>();O->SetNumberField(TEXT("version"),1);O->SetBoolField(TEXT("recorded"),R.bRecorded);O->SetBoolField(TEXT("earlier_battles_unrecorded"),R.bIncludesEarlierUnrecordedBattles);
    O->SetObjectField(TEXT("friendly_fire"),TallyV839(R.FriendlyFire));
    CountsV839(*O,TEXT("deployed_by_role"),R.DeployedByRole);CountsV839(*O,TEXT("lost_by_role"),R.LostByRole);
    CountsV839(*O,TEXT("armour_lost_by_model"),R.ArmourLostByModel);CountsV839(*O,TEXT("armour_destroyed_by_model"),R.ArmourDestroyedByModel);
    ModelsV839(*O,TEXT("national_armour_lost"),R.NationalArmourLost);ModelsV839(*O,TEXT("national_armour_destroyed"),R.NationalArmourDestroyed);
    TArray<TSharedPtr<FJsonValue>> Rows;
    for(int32 I=0;I<10;++I)
    {
        const auto& S=R.Support[I];auto Row=MakeShared<FJsonObject>();Row->SetObjectField(TEXT("friendly_fire"),TallyV839(R.FriendlySupport[I]));
        Row->SetNumberField(TEXT("delivered"),S.Delivered);Row->SetNumberField(TEXT("rounds"),S.Rounds);
        Row->SetNumberField(TEXT("enemy_hull_damage"),S.EnemyHullDamage);Row->SetNumberField(TEXT("enemy_track_damage"),S.EnemyTrackDamage);
        Row->SetNumberField(TEXT("friendly_hull_damage"),S.FriendlyHullDamage);Row->SetNumberField(TEXT("friendly_track_damage"),S.FriendlyTrackDamage);
        Row->SetBoolField(TEXT("delivery_recorded"),S.bDeliveryRecorded);Row->SetBoolField(TEXT("rounds_recorded"),S.bRoundsRecorded);Row->SetBoolField(TEXT("damage_recorded"),S.bDamageRecorded);Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    O->SetArrayField(TEXT("support"),Rows);
    if(!R.Squads.IsEmpty())
    {
        TArray<int32> Ids;R.Squads.GetKeys(Ids);Ids.Sort();TArray<TSharedPtr<FJsonValue>> Sections;
        for(int32 Id:Ids)
        {
            const auto& S=R.Squads[Id];auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("id"),S.Id);Row->SetNumberField(TEXT("role"),S.OriginalRole);
            Row->SetNumberField(TEXT("deployed"),S.Deployed);Row->SetNumberField(TEXT("lost"),S.Lost);
            Row->SetObjectField(TEXT("kills"),TallyV839(S.Kills));Row->SetObjectField(TEXT("friendly_fire"),TallyV839(S.FriendlyFire));Sections.Add(MakeShared<FJsonValueObject>(Row));
        }
        O->SetArrayField(TEXT("sections_v86"),Sections);
    }
    J.SetObjectField(TEXT("detailed_report_v839"),O);
}
bool ReadArchive(const FJsonObject& J,FFrontlineDetailedRecordV839& R,bool Earlier)
{
    FFrontlineDetailedRecordV839 Read;
    if(!J.HasField(TEXT("detailed_report_v839"))){Read.bIncludesEarlierUnrecordedBattles=Earlier;R=Read;return true;}
    const TSharedPtr<FJsonObject>* O=nullptr,*T=nullptr;int32 Version=0;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if(!J.TryGetObjectField(TEXT("detailed_report_v839"),O)||!O||!O->IsValid()||!NumberV839(**O,TEXT("version"),Version)||Version!=1
        ||!(*O)->TryGetBoolField(TEXT("recorded"),Read.bRecorded)||!(*O)->TryGetBoolField(TEXT("earlier_battles_unrecorded"),Read.bIncludesEarlierUnrecordedBattles)
        ||!(*O)->TryGetObjectField(TEXT("friendly_fire"),T)||!T||!T->IsValid()||!ReadTallyV839(**T,Read.FriendlyFire)
        ||!ReadCountsV839(**O,TEXT("deployed_by_role"),Read.DeployedByRole)||!ReadCountsV839(**O,TEXT("lost_by_role"),Read.LostByRole)
        ||!ReadCountsV839(**O,TEXT("armour_lost_by_model"),Read.ArmourLostByModel)||!ReadCountsV839(**O,TEXT("armour_destroyed_by_model"),Read.ArmourDestroyedByModel)
        ||!ReadModelsV839(**O,TEXT("national_armour_lost"),Read.NationalArmourLost)||!ReadModelsV839(**O,TEXT("national_armour_destroyed"),Read.NationalArmourDestroyed)
        ||!(*O)->TryGetArrayField(TEXT("support"),Rows)||!Rows||Rows->Num()!=10)return false;
    for(int32 I=0;I<10;++I)
    {
        const TSharedPtr<FJsonObject>* V=nullptr;const TSharedPtr<FJsonObject>* F=nullptr;auto& S=Read.Support[I];
        if(!(*Rows)[I].IsValid()||!(*Rows)[I]->TryGetObject(V)||!V||!V->IsValid()||!(*V)->TryGetObjectField(TEXT("friendly_fire"),F)||!F||!F->IsValid()||!ReadTallyV839(**F,Read.FriendlySupport[I])
            ||!NumberV839(**V,TEXT("delivered"),S.Delivered)||!NumberV839(**V,TEXT("rounds"),S.Rounds)
            ||!DamageV839(**V,TEXT("enemy_hull_damage"),S.EnemyHullDamage)||!DamageV839(**V,TEXT("enemy_track_damage"),S.EnemyTrackDamage)
            ||!DamageV839(**V,TEXT("friendly_hull_damage"),S.FriendlyHullDamage)||!DamageV839(**V,TEXT("friendly_track_damage"),S.FriendlyTrackDamage)
            ||!(*V)->TryGetBoolField(TEXT("delivery_recorded"),S.bDeliveryRecorded)||!(*V)->TryGetBoolField(TEXT("rounds_recorded"),S.bRoundsRecorded)||!(*V)->TryGetBoolField(TEXT("damage_recorded"),S.bDamageRecorded))return false;
    }
    if((*O)->HasField(TEXT("sections_v86")))
    {
        const TArray<TSharedPtr<FJsonValue>>* Sections=nullptr;
        if(!(*O)->TryGetArrayField(TEXT("sections_v86"),Sections)||!Sections||Sections->Num()>100000)return false;
        for(const auto& Value:*Sections)
        {
            const TSharedPtr<FJsonObject>* Row=nullptr,*Kills=nullptr,*Friendly=nullptr;FFrontlineSquadReportV839 S;
            if(!Value.IsValid()||!Value->TryGetObject(Row)||!Row||!Row->IsValid()
                ||!NumberV839(**Row,TEXT("id"),S.Id)||Read.Squads.Contains(S.Id)
                ||!NumberV839(**Row,TEXT("role"),S.OriginalRole)||S.OriginalRole>=11
                ||!NumberV839(**Row,TEXT("deployed"),S.Deployed)||!NumberV839(**Row,TEXT("lost"),S.Lost)
                ||!(*Row)->TryGetObjectField(TEXT("kills"),Kills)||!Kills||!Kills->IsValid()||!ReadTallyV839(**Kills,S.Kills)
                ||!(*Row)->TryGetObjectField(TEXT("friendly_fire"),Friendly)||!Friendly||!Friendly->IsValid()||!ReadTallyV839(**Friendly,S.FriendlyFire))return false;
            Read.Squads.Add(S.Id,S);
        }
    }
    R=Read;return true;
}
void RunCodecChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    FFrontlineDetailedRecordV839 R;R.bRecorded=true;R.FriendlyFire={2,1};R.FriendlySupport[0]=R.FriendlyFire;R.DeployedByRole[0]=6;R.LostByRole[0]=2;R.ArmourLostByModel[0]=1;R.ArmourDestroyedByModel[2]=1;R.NationalArmourLost.Add(TEXT("ru.austin_series1"),1);
    auto& S=R.Support[0];S.Delivered=2;S.Rounds=24;S.EnemyHullDamage=29.25;S.FriendlyTrackDamage=3.5;S.bDeliveryRecorded=S.bRoundsRecorded=S.bDamageRecorded=true;
    FJsonObject J;WriteArchive(J,R);FFrontlineDetailedRecordV839 Read;
    Check(TEXT("v839_detail_codec_exact_roundtrip"),ReadArchive(J,Read,false)&&Read.bRecorded&&Read.FriendlyFire.Infantry==2&&Read.ArmourDestroyedByModel[2]==1&&Read.Support[0].Rounds==24&&Read.Support[0].EnemyHullDamage==29.25&&Read.NationalArmourLost.FindRef(TEXT("ru.austin_series1"))==1);
    auto N=R;N.Support[0].Rounds+=3;N.Support[0].EnemyHullDamage+=1.25;N.FriendlySupport[0].Infantry+=1;N.FriendlyFire.Infantry+=1;N.ArmourDestroyedByModel[2]+=1;
    const auto D=Tail(N,R);auto Combined=R;Combined.Add(D);
    Check(TEXT("v839_outro_tail_exact_and_repeat_has_no_counts"),D.Support[0].Rounds==3&&Combined.Support[0].Rounds==27&&Combined.Support[0].EnemyHullDamage==30.5&&Combined.FriendlyFire.Infantry==3&&Tail(N,N).Support[0].Rounds==0&&Tail(N,N).ArmourDestroyedByModel[2]==0);
    FJsonObject Old;FFrontlineDetailedRecordV839 Legacy;
    Check(TEXT("v839_legacy_remains_unrecorded"),ReadArchive(Old,Legacy,true)&&!Legacy.bRecorded&&Legacy.bIncludesEarlierUnrecordedBattles&&Legacy.FriendlyFire.Total()==0);
    J.GetObjectField(TEXT("detailed_report_v839"))->SetNumberField(TEXT("version"),2);
    Check(TEXT("v839_unknown_detail_version_rejected_without_overwrite"),!ReadArchive(J,Read,false)&&Read.Support[0].Rounds==24);
}
FView& View(APlayerController* PC)
{static TMap<TWeakObjectPtr<APlayerController>,FView> Views;for(auto It=Views.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();return Views.FindOrAdd(PC);}
bool Active(APlayerController* PC,const AWarfareFrontlineGameMode* B)
{
    if(!PC||!B)return false;auto& V=View(PC);
    if(V.CampaignV86)
    {
        const auto* R=B->GetCurrentCampaign();const FGuid Id=WarfareFRRUCampaignV82::Active(B)?WarfareFRRUCampaignV82::CampaignId(B):R?R->Id:FGuid();
        if(!Id.IsValid()||Id!=V.Campaign||FrontlineUltimateArchiveV8215::Active(B)||!FrontlineCampaignV51::CanOpenEnding(B)){V={};return false;}
        return V.Open;
    }
    if(V.Encounter!=B->GetEncounterId()){V={};V.Encounter=B->GetEncounterId();}
    return V.Open&&(B->Phase==EFrontlinePhase::Victory||B->Phase==EFrontlinePhase::Defeat)&&!B->IsBattleOutroActive();
}
bool Close(APlayerController* PC,const AWarfareFrontlineGameMode* B)
{if(!Active(PC,B))return false;View(PC).Open=false;return true;}
bool Click(APlayerController* PC,AWarfareFrontlineGameMode* B,FName Command)
{
    if(!PC||!B||B->IsBattleOutroActive())return false;
    if(Command==TEXT("CampaignDetailedReportV86")){OpenCampaignV86(PC,B);return Active(PC,B);}
    const bool IsOpen=Active(PC,B);auto& V=View(PC);const FString Key=Command.ToString();
    if(!IsOpen&&(B->Phase!=EFrontlinePhase::Victory&&B->Phase!=EFrontlinePhase::Defeat))return false;
    if(Command==TEXT("DetailedReportV839")){V.CampaignV86=false;V.Open=true;V.Offset=0;return true;}
    if(!V.Open)return false;
    if(Command==TEXT("ReportBackV839")||Command==TEXT("Back")){V.Open=false;return true;}
    if(Key.StartsWith(TEXT("ReportTabV839_"))){const FString N=Key.Mid(14);if(N.Len()==1&&N[0]>=TEXT('0')&&N[0]<=TEXT('5')){V.Tab=N[0]-TEXT('0');V.Offset=0;}return true;}
    if(Command==TEXT("ReportOurArmyV839")||Command==TEXT("ReportEnemyArmyV839")){V.Team=Command==TEXT("ReportEnemyArmyV839")?1:0;V.Offset=0;return true;}
    if(Command==TEXT("ReportUpV839")){V.Offset=FMath::Max(0,V.Offset-1);return true;}
    if(Command==TEXT("ReportDownV839")){V.Offset=FMath::Min(100000,V.Offset+1);return true;}
    return true; // The open return sheet consumes stale gameplay/settlement hit boxes.
}
bool Wheel(APlayerController* PC,const AWarfareFrontlineGameMode* B,int32 Direction)
{if(!Active(PC,B))return false;auto& V=View(PC);V.Offset=FMath::Clamp(V.Offset+Direction,0,100000);return true;}
}
