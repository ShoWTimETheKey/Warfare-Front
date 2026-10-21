#include "WarfareFrontlineCombatRecordV65.h"
#include <atomic>
#include "WarfareFrontlineReportV839.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineArmourKillTallyV8220.h"
#include "WarfareFrontlineArmourKillTallyAccessV8220.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFRRUArmourV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineFieldV32.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"

struct FFrontlineCombatRecordReviewV65
{
    static AFrontlineTank* PrepareArmour(AWarfareFrontlineGameMode* B)
    {
        FString UserDir;
        // Also the V68 art review's armour-under-new-light fixture.
        if(!B||!B->IsProgressionVolatile()||(!FParse::Param(FCommandLine::Get(),TEXT("WFV65Review"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV68ArtReview")))
            ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return nullptr;
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Fleet)return nullptr;
        // The whole encounter is disposable; retire prior review tanks before registering this sample.
        Fleet->ClearEncounter();Fleet->LastEncounter=B->GetEncounterId();Fleet->NextEnemyAt=MAX_flt;
        const FVector Position(0,0,AWarfareFrontlineGround::HeightAt(0,0));
        auto* Tank=B->GetWorld()->SpawnActor<AFrontlineTank>(Position,FRotator(0,B->GetTeamDirection(0)<0?180.f:0.f,0));
        if(!Tank)return nullptr;
        if(!Tank->Configure(B,EFrontlineVehicle::MarkIVMale,0,++Fleet->NextSerial)){Tank->Destroy();return nullptr;}
        Tank->PreferredLaneV37=Tank->DesiredLaneV37=0;Tank->bEntered=true;
        Tank->FitGroundV35(Position,0);Tank->SetActorTickEnabled(false);
        Tank->KillsV65.Infantry=23;Tank->KillsV65.Armour=2; // Synthetic layout values, not a combat receipt.
        // Vehicles simulate in the subsystem even with ActorTick disabled. Detach only this static
        // review sample's director after ground fitting; operational modules and hover remain intact.
        Tank->Battle=nullptr;Fleet->Vehicles.Add(Tank);
        return Tank;
    }
};

namespace
{
constexpr int32 Maximum=100000000;
struct FArmourDeathsV8220{FGuid Encounter;TSet<int32> Serials;};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FArmourDeathsV8220> ArmourDeathsV8220;
int32 Sum(int32 A,int32 B){return int32(FMath::Min(int64(Maximum),int64(A)+int64(B)));}
bool ReadNumber(const FJsonObject& J,const TCHAR* Key,int32& Out)
{double V=0;if(!J.TryGetNumberField(Key,V)||!FMath::IsFinite(V)||V<0||V>Maximum||V!=FMath::FloorToDouble(V))return false;Out=int32(V);return true;}
TSharedRef<FJsonObject> WriteTally(const FFrontlineKillTallyV65& T)
{auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("infantry"),T.Infantry);J->SetNumberField(TEXT("armour"),T.Armour);return J;}
bool ReadTally(const FJsonObject& J,FFrontlineKillTallyV65& T)
{return ReadNumber(J,TEXT("infantry"),T.Infantry)&&ReadNumber(J,TEXT("armour"),T.Armour);}
template<size_t N> void WriteArray(FJsonObject& J,const TCHAR* Name,const FFrontlineKillTallyV65 (&Rows)[N])
{TArray<TSharedPtr<FJsonValue>> A;for(const auto& R:Rows)A.Add(MakeShared<FJsonValueObject>(WriteTally(R)));J.SetArrayField(Name,A);}
template<size_t N> bool ReadArray(const FJsonObject& J,const TCHAR* Name,FFrontlineKillTallyV65 (&Rows)[N],size_t LegacyCount=0,size_t PreviousCount=0)
{const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(!J.TryGetArrayField(Name,A)||!A||(A->Num()!=N&&!(LegacyCount>0&&LegacyCount<N&&A->Num()==LegacyCount)&&!(PreviousCount>0&&PreviousCount<N&&A->Num()==PreviousCount)))return false;for(int32 I=0;I<A->Num();++I){const TSharedPtr<FJsonObject>* O=nullptr;if(!(*A)[I].IsValid()||!(*A)[I]->TryGetObject(O)||!O||!O->IsValid()||!ReadTally(**O,Rows[I]))return false;}for(size_t I=A->Num();I<N;++I)Rows[I]={};return true;}
void Loss(AWarfareFrontlineGameMode* B,int32 Team,const FFrontlineKillSourceV65& S,bool Armour,const AWarfareFrontlineSoldier* Victim=nullptr)
{
    const bool SupportOutro=B&&B->CanResolveCommittedSupportV8217()&&B->BattleOutroWinner>=0
        &&S.Support>=0&&S.Support<int32(EFrontlineSupport::Count);
    if(!B||Team<0||Team>1||((B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)&&!SupportOutro))return;
    auto& Losses=B->CombatRecordV65[Team];
    if(Armour)Losses.ArmourLost=Sum(Losses.ArmourLost,1);else Losses.InfantryLost=Sum(Losses.InfantryLost,1);
    Losses.InvalidateReportV86();
    if(!Armour)FrontlineReportV839::InfantryDeath(B,Team,Victim,S);
    if(B->Phase==EFrontlinePhase::Fighting)FrontlineAchievementsV76::Loss(B,Team,S,Armour);
    // Friendly fire is a loss, never an enemy kill or a unit's achievement.
    if(S.Team<0||S.Team>1||S.Team==Team)return;
    FFrontlineKillTallyV65 One;if(Armour)One.Armour=1;else One.Infantry=1;
    auto& R=B->CombatRecordV65[S.Team];R.Total.Add(One);
    R.InvalidateReportV86();
    if(S.SquadId!=INDEX_NONE&&S.Role>=0&&S.Role<UE_ARRAY_COUNT(R.InfantrySources))
    {
        R.InfantrySources[S.Role].Add(One);
        if(auto* Q=B->FindSquad(S.SquadId);Q&&Q->Team==S.Team)Q->KillsV65.Add(One);
    }
    else if(S.VehicleSerial!=INDEX_NONE)
    {
        if(!S.NationalVehicleIdV82.IsEmpty()&&WarfareFRRUArmourV82::Definition(S.NationalVehicleIdV82))R.NationalArmourSourcesV82.FindOrAdd(S.NationalVehicleIdV82).Add(One);
        else if(S.VehicleType>=0&&S.VehicleType<UE_ARRAY_COUNT(R.ArmourSources))R.ArmourSources[S.VehicleType].Add(One);
        else R.OtherSources.Add(One);
        if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* V:Fleet->GetVehicles())
            if(V&&V->Team==S.Team&&V->Serial==S.VehicleSerial){V->KillsV65.Add(One);break;}
    }
    else if(S.Support>=0&&S.Support<UE_ARRAY_COUNT(R.SupportSources))R.SupportSources[S.Support].Add(One);
    else R.OtherSources.Add(One);
}
}
void FFrontlineKillTallyV65::Add(const FFrontlineKillTallyV65& R){Infantry=Sum(Infantry,R.Infantry);Armour=Sum(Armour,R.Armour);}
uint64 FFrontlineCombatRecordV65::NewReportRevisionV86()
{static std::atomic<uint64> Revision{0};return Revision.fetch_add(1,std::memory_order_relaxed)+1;}
void FFrontlineCombatRecordV65::Add(const FFrontlineCombatRecordV65& R)
{
    if((BattlesRecorded>0&&!DetailsV839.bRecorded)||(R.BattlesRecorded>0&&!R.DetailsV839.bRecorded))DetailsV839.bIncludesEarlierUnrecordedBattles=true;
    DetailsV839.Add(R.DetailsV839);
    Total.Add(R.Total);OtherSources.Add(R.OtherSources);
    for(int32 I=0;I<UE_ARRAY_COUNT(InfantrySources);++I)InfantrySources[I].Add(R.InfantrySources[I]);
    for(int32 I=0;I<UE_ARRAY_COUNT(ArmourSources);++I)ArmourSources[I].Add(R.ArmourSources[I]);
    for(int32 I=0;I<UE_ARRAY_COUNT(SupportSources);++I)SupportSources[I].Add(R.SupportSources[I]);
    for(const auto& Entry:R.NationalArmourSourcesV82)NationalArmourSourcesV82.FindOrAdd(Entry.Key).Add(Entry.Value);
    InfantryLost=Sum(InfantryLost,R.InfantryLost);ArmourLost=Sum(ArmourLost,R.ArmourLost);BattlesRecorded=Sum(BattlesRecorded,R.BattlesRecorded);
    bIncludesEarlierUnrecordedBattles|=R.bIncludesEarlierUnrecordedBattles;
    InvalidateReportV86();
}
FFrontlineKillSourceV65 FFrontlineKillSourceV65::Soldier(const AWarfareFrontlineSoldier* U)
{
    FFrontlineKillSourceV65 S;
    if(U)
    {
        S.Team=U->Team;S.SquadId=U->SquadId;S.Role=int32(U->Role);
        if(const auto* Equipment=WarfareFRRUEquipmentV82::Resolve(U))
        {S.NationalWeaponIdV82=Equipment->WeaponId;S.NationalSourceNationV82=int32(WarfareFRRUCampaignV82::Nation(U));}
    }
    return S;
}
FFrontlineKillSourceV65 FFrontlineKillSourceV65::Vehicle(const AFrontlineTank* V)
{FFrontlineKillSourceV65 S;if(V){S.Team=V->Team;S.VehicleSerial=V->Serial;S.VehicleType=int32(V->Type);S.NationalVehicleIdV82=WarfareFRRUArmourV82::Id(V);}return S;}
FFrontlineKillSourceV65 FFrontlineKillSourceV65::FireSupport(int32 Team,int32 Support)
{FFrontlineKillSourceV65 S;S.Team=Team;S.Support=Support;return S;}

namespace FrontlineCombatRecordV65
{
AFrontlineTank* PrepareArmourReview(AWarfareFrontlineGameMode* B){return FFrontlineCombatRecordReviewV65::PrepareArmour(B);}
void InfantryLost(AWarfareFrontlineGameMode* B,int32 Team,const FFrontlineKillSourceV65& S,const AWarfareFrontlineSoldier* Victim){Loss(B,Team,S,false,Victim);}
void ArmourLost(AWarfareFrontlineGameMode* B,int32 Team,const FFrontlineKillSourceV65& S){Loss(B,Team,S,true);}
void ArmourDestroyedV8220(AWarfareFrontlineGameMode* B,const AFrontlineTank* Victim,const FFrontlineKillSourceV65& Source)
{
    const bool SupportOutro=B&&B->CanResolveCommittedSupportV8217()&&B->BattleOutroWinner>=0
        &&Source.Support>=0&&Source.Support<int32(EFrontlineSupport::Count);
    if(!B||!IsValid(Victim)||FFrontlineArmourKillTallyV8220Review::Owner(Victim)!=B
        ||Victim->Serial<=0||Victim->Team<0||Victim->Team>1||!FMath::IsFinite(Victim->GetHullIntegrity())||Victim->GetHullIntegrity()>0
        ||((B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)&&!SupportOutro))return;
    for(auto It=ArmourDeathsV8220.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& Seen=ArmourDeathsV8220.FindOrAdd(B);
    if(Seen.Encounter!=B->GetEncounterId()){Seen=FArmourDeathsV8220();Seen.Encounter=B->GetEncounterId();}
    if(Seen.Serials.Contains(Victim->Serial))return;
    Seen.Serials.Add(Victim->Serial);
    Loss(B,Victim->Team,Source,true);
    FrontlineAchievementsV76::ArmourDestroyedV84415(B,Victim,Source);
    FrontlineReportV839::ArmourDeath(B,Victim,Source);
    FrontlineArmourKillTallyV8220::RecordDestroyedArmour(B,Victim->Team,Victim->Serial,Source);
}
void WriteArchive(FJsonObject& J,const FFrontlineCombatRecordV65& R)
{
    auto O=MakeShared<FJsonObject>();O->SetObjectField(TEXT("total"),WriteTally(R.Total));O->SetObjectField(TEXT("other"),WriteTally(R.OtherSources));
    WriteArray(*O,TEXT("infantry_sources"),R.InfantrySources);WriteArray(*O,TEXT("armour_sources"),R.ArmourSources);WriteArray(*O,TEXT("support_sources"),R.SupportSources);
    if(!R.NationalArmourSourcesV82.IsEmpty())
    {auto National=MakeShared<FJsonObject>();TArray<FString> Keys;R.NationalArmourSourcesV82.GetKeys(Keys);Keys.Sort();for(const auto& Key:Keys)National->SetObjectField(Key,WriteTally(R.NationalArmourSourcesV82[Key]));O->SetObjectField(TEXT("national_armour_sources_v82"),National);}
    O->SetNumberField(TEXT("infantry_lost"),R.InfantryLost);O->SetNumberField(TEXT("armour_lost"),R.ArmourLost);O->SetNumberField(TEXT("battles_recorded"),R.BattlesRecorded);
    O->SetBoolField(TEXT("earlier_battles_unrecorded"),R.bIncludesEarlierUnrecordedBattles);FrontlineReportV839::WriteArchive(*O,R.DetailsV839);
    if(!R.BattlesV86.IsEmpty())
    {
        TArray<TSharedPtr<FJsonValue>> Returns;
        for(const auto& Battle:R.BattlesV86)
        {
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("encounter"),Battle.Encounter.ToString(EGuidFormats::Digits));
            Row->SetNumberField(TEXT("mission"),Battle.Mission);Row->SetNumberField(TEXT("winner"),Battle.Winner);Row->SetNumberField(TEXT("seconds"),Battle.Seconds);
            for(int32 Team=0;Team<2;++Team)if(Battle.Armies[Team].IsValid())
            {
                auto Side=MakeShared<FJsonObject>();auto Snapshot=*Battle.Armies[Team];Snapshot.BattlesV86.Reset();
                WriteArchive(*Side,Snapshot);Row->SetObjectField(Team==0?TEXT("our_army"):TEXT("enemy_army"),Side);
            }
            Returns.Add(MakeShared<FJsonValueObject>(Row));
        }
        O->SetArrayField(TEXT("campaign_returns_v86"),Returns);
    }
    J.SetObjectField(TEXT("combat_record_v65"),O);
}
bool ReadArchive(const FJsonObject& J,FFrontlineCombatRecordV65& R,bool Earlier)
{
    FFrontlineCombatRecordV65 Read;
    if(!J.HasField(TEXT("combat_record_v65"))){Read.bIncludesEarlierUnrecordedBattles=Earlier;R=Read;R.InvalidateReportV86();return true;}
    const TSharedPtr<FJsonObject>* O=nullptr,*T=nullptr,*U=nullptr;
    if(!J.TryGetObjectField(TEXT("combat_record_v65"),O)||!O||!O->IsValid()
        ||!(*O)->TryGetObjectField(TEXT("total"),T)||!T||!T->IsValid()||!ReadTally(**T,Read.Total)
        ||!(*O)->TryGetObjectField(TEXT("other"),U)||!U||!U->IsValid()||!ReadTally(**U,Read.OtherSources)
        ||!ReadArray(**O,TEXT("infantry_sources"),Read.InfantrySources)||!ReadArray(**O,TEXT("armour_sources"),Read.ArmourSources,10,12)||!ReadArray(**O,TEXT("support_sources"),Read.SupportSources)
        ||!ReadNumber(**O,TEXT("infantry_lost"),Read.InfantryLost)||!ReadNumber(**O,TEXT("armour_lost"),Read.ArmourLost)||!ReadNumber(**O,TEXT("battles_recorded"),Read.BattlesRecorded)
        ||!(*O)->TryGetBoolField(TEXT("earlier_battles_unrecorded"),Read.bIncludesEarlierUnrecordedBattles))return false;
    FFrontlineKillTallyV65 SumSources=Read.OtherSources;
    if((*O)->HasField(TEXT("national_armour_sources_v82")))
    {
        const TSharedPtr<FJsonObject>* National=nullptr;
        if(!(*O)->TryGetObjectField(TEXT("national_armour_sources_v82"),National)||!National||!National->IsValid()||(*National)->Values.Num()>5)return false;
        for(const auto& Entry:(*National)->Values)
        {
            const FString Id(*Entry.Key);
            const TSharedPtr<FJsonObject>* Tally=nullptr;FFrontlineKillTallyV65 Counts;
            if(!WarfareFRRUArmourV82::Definition(Id)||!Entry.Value.IsValid()||!Entry.Value->TryGetObject(Tally)||!Tally||!Tally->IsValid()||!ReadTally(**Tally,Counts))return false;
            Read.NationalArmourSourcesV82.Add(Id,Counts);SumSources.Add(Counts);
        }
    }
    for(const auto& A:Read.InfantrySources)SumSources.Add(A);for(const auto& A:Read.ArmourSources)SumSources.Add(A);for(const auto& A:Read.SupportSources)SumSources.Add(A);
    if(SumSources.Infantry!=Read.Total.Infantry||SumSources.Armour!=Read.Total.Armour)return false;
    if(!FrontlineReportV839::ReadArchive(**O,Read.DetailsV839,Read.BattlesRecorded>0||Earlier))return false;
    int64 ArmourLostModels=0,ArmourDestroyedModels=0,InfantryLostRoles=0;
    for(int32 I=0;I<13;++I){ArmourLostModels+=Read.DetailsV839.ArmourLostByModel[I];ArmourDestroyedModels+=Read.DetailsV839.ArmourDestroyedByModel[I];}
    for(const auto& E:Read.DetailsV839.NationalArmourLost)ArmourLostModels+=E.Value;for(const auto& E:Read.DetailsV839.NationalArmourDestroyed)ArmourDestroyedModels+=E.Value;
    for(int32 I=0;I<11;++I)InfantryLostRoles+=Read.DetailsV839.LostByRole[I];
    if(ArmourLostModels>Read.ArmourLost||ArmourDestroyedModels>Read.Total.Armour||InfantryLostRoles>Read.InfantryLost)return false;
    if((*O)->HasField(TEXT("campaign_returns_v86")))
    {
        const TArray<TSharedPtr<FJsonValue>>* Returns=nullptr;TSet<FGuid> Seen;
        if(!(*O)->TryGetArrayField(TEXT("campaign_returns_v86"),Returns)||!Returns||Returns->Num()>10000)return false;
        for(const auto& Value:*Returns)
        {
            const TSharedPtr<FJsonObject>* Row=nullptr;FFrontlineCampaignBattleV86 Battle;FString Guid;double Seconds=0;
            if(!Value.IsValid()||!Value->TryGetObject(Row)||!Row||!Row->IsValid()
                ||!(*Row)->TryGetStringField(TEXT("encounter"),Guid)||!FGuid::ParseExact(Guid,EGuidFormats::Digits,Battle.Encounter)||!Battle.Encounter.IsValid()||Seen.Contains(Battle.Encounter)
                ||!ReadNumber(**Row,TEXT("mission"),Battle.Mission)||Battle.Mission>99
                ||!ReadNumber(**Row,TEXT("winner"),Battle.Winner)||Battle.Winner>1
                ||!(*Row)->TryGetNumberField(TEXT("seconds"),Seconds)||!FMath::IsFinite(Seconds)||Seconds<0||Seconds>10000000)return false;
            Battle.Seconds=float(Seconds);
            for(int32 Team=0;Team<2;++Team)
            {
                const TSharedPtr<FJsonObject>* Side=nullptr,*Child=nullptr;FFrontlineCombatRecordV65 Snapshot;
                if(!(*Row)->TryGetObjectField(Team==0?TEXT("our_army"):TEXT("enemy_army"),Side)||!Side||!Side->IsValid()
                    ||!(*Side)->TryGetObjectField(TEXT("combat_record_v65"),Child)||!Child||!Child->IsValid()
                    ||(*Child)->HasField(TEXT("campaign_returns_v86"))||!ReadArchive(**Side,Snapshot,false)||Snapshot.BattlesRecorded!=1)return false;
                Battle.Armies[Team]=MakeShared<FFrontlineCombatRecordV65>(MoveTemp(Snapshot));
            }
            Seen.Add(Battle.Encounter);Read.BattlesV86.Add(MoveTemp(Battle));
        }
        if(Read.BattlesV86.Num()>Read.BattlesRecorded)return false;
    }
    R=Read;R.InvalidateReportV86();return true;
}
static_assert(int32(EFrontlineRole::Count)==11&&int32(EFrontlineSupport::Count)==10&&int32(EFrontlineVehicle::Count)==13,"Extend V65 archive arrays when adding content.");
void RunSourceCodecChecksV8218(TFunctionRef<void(const FString&,bool)> Check)
{
    FrontlineReportV839::RunCodecChecks(Check);
    FFrontlineCombatRecordV65 New;New.ArmourSources[10]={2,1};New.ArmourSources[11]={3,0};New.Total={5,1};New.BattlesRecorded=1;
    FJsonObject Json;WriteArchive(Json,New);FFrontlineCombatRecordV65 Decoded;
    Check(TEXT("v8218_twelve_armour_sources_star_austin_roundtrip_and_sum"),ReadArchive(Json,Decoded,false)&&Decoded.Total.Infantry==5&&Decoded.Total.Armour==1&&Decoded.ArmourSources[10].Infantry==2&&Decoded.ArmourSources[10].Armour==1&&Decoded.ArmourSources[11].Infantry==3);
    FFrontlineCombatRecordV65 Combined;Combined.Add(Decoded);Combined.Add(Decoded);
    Check(TEXT("v8218_new_armour_sources_accumulate_with_total"),Combined.Total.Infantry==10&&Combined.ArmourSources[10].Infantry==4&&Combined.ArmourSources[11].Infantry==6);
    FFrontlineCombatRecordV65 Old;Old.Total=Old.ArmourSources[0]={4,1};WriteArchive(Json,Old);
    const auto Object=Json.GetObjectField(TEXT("combat_record_v65"));auto Rows=Object->GetArrayField(TEXT("armour_sources"));Rows.SetNum(10);Object->SetArrayField(TEXT("armour_sources"),Rows);
    FFrontlineCombatRecordV65 Legacy;
    Check(TEXT("v8218_legacy_ten_armour_sources_zero_extend_without_relabel"),ReadArchive(Json,Legacy,false)&&Legacy.Total.Infantry==4&&Legacy.ArmourSources[0].Armour==1&&Legacy.ArmourSources[10].Total()==0&&Legacy.ArmourSources[11].Total()==0);
    Rows.Add(MakeShared<FJsonValueObject>(WriteTally({})));Object->SetArrayField(TEXT("armour_sources"),Rows);
    Check(TEXT("v8218_unknown_eleven_armour_sources_rejected_without_overwrite"),!ReadArchive(Json,Legacy,false)&&Legacy.Total.Infantry==4&&Legacy.ArmourSources[0].Armour==1);
    Rows.Add(MakeShared<FJsonValueObject>(WriteTally({})));Object->SetArrayField(TEXT("armour_sources"),Rows);
    Check(TEXT("v836_legacy_twelve_armour_sources_zero_extend_train"),ReadArchive(Json,Legacy,false)&&Legacy.Total.Infantry==4&&Legacy.ArmourSources[12].Total()==0);
    FFrontlineCombatRecordV65 Rail;Rail.Total=Rail.ArmourSources[12]={7,2};Rail.BattlesRecorded=1;WriteArchive(Json,Rail);
    Check(TEXT("v836_train_source_roundtrip_and_sum"),ReadArchive(Json,Decoded,false)&&Decoded.Total.Infantry==7&&Decoded.ArmourSources[12].Armour==2);
    WriteArchive(Json,New);Json.GetObjectField(TEXT("combat_record_v65"))->SetObjectField(TEXT("total"),WriteTally({6,1}));
    Check(TEXT("v8218_new_armour_codec_still_rejects_mismatched_source_sum"),!ReadArchive(Json,Legacy,false)&&Legacy.Total.Infantry==4);
}
void RunFocusedChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString UserDir;
    if(!B||!B->IsProgressionVolatile()||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty()){Check(TEXT("v65_stats_isolated_fixture"),false);return;}
    const auto Units=B->Units;const auto Squads=B->Squads;const auto Phase=B->Phase;const auto OldOutro=B->BattleOutroWinner;
    const FFrontlineCombatRecordV65 Records[]={B->CombatRecordV65[0],B->CombatRecordV65[1]};
    const int32 Casualties[]={B->Casualties[0],B->Casualties[1]};const float Damage[]={B->DamageDealt[0],B->DamageDealt[1]},Morale[]={B->Morale[0],B->Morale[1]};
    const float Friendly=B->FriendlyFireDamage,Peak=B->PeakSuppression;const auto Message=B->LastMessage;
    TArray<AWarfareFrontlineSoldier*> Made;AFrontlineTank* Tank=nullptr;
    ON_SCOPE_EXIT{for(auto* U:Made)if(IsValid(U))U->Destroy();if(IsValid(Tank))Tank->Destroy();B->Units=Units;B->Squads=Squads;B->Phase=Phase;B->BattleOutroWinner=OldOutro;for(int32 I=0;I<2;++I){B->CombatRecordV65[I]=Records[I];B->Casualties[I]=Casualties[I];B->DamageDealt[I]=Damage[I];B->Morale[I]=Morale[I];}B->FriendlyFireDamage=Friendly;B->PeakSuppression=Peak;B->LastMessage=Message;};
    B->Units.Reset();B->Squads.Reset();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->CombatRecordV65[0]={};B->CombatRecordV65[1]={};
    auto Make=[&](int32 Team,EFrontlineRole Role)
    {
        const int32 Id=65000+Made.Num();FVector P((Team?1.f:-1.f)*450,Made.Num()*100,0);P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y)+88;
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(P,FRotator(0,Team?180:0,0));if(!U)return U;
        U->Initialize(B,Id,Team,Role,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;B->Units.Add(U);Made.Add(U);
        FFrontlineSquad S;S.Id=Id;S.Team=Team;S.Role=Role;S.Members.Add(U);B->Squads.Add(S);return U;
    };
    auto* Shooter=Make(0,EFrontlineRole::Rifle);auto* Victim=Make(1,EFrontlineRole::Rifle);auto* Flame=Make(1,EFrontlineRole::Flamethrower);auto* FriendlyUnit=Make(0,EFrontlineRole::Rifle);
    if(!Shooter||!Victim||!Flame||!FriendlyUnit){Check(TEXT("v65_stats_fixture_spawn"),false);return;}
    if(auto* Animation=Cast<UWarfareFrontlineAnimation>(Shooter->Soldier->GetAnimInstance()))
    {
        auto Step=[&](float DT){Shooter->Soldier->HandleExistingParallelEvaluationTask(true,true);Shooter->Soldier->TickAnimation(DT,false);Shooter->Soldier->RefreshBoneTransforms();};
        Shooter->bMoving=true;Animation->TransitionTo(Shooter->GetWalkMotion(),true,1.f,.31f,.18f);
        for(int32 I=0;I<18;++I)Step(1.f/60);
        const FVector Hand=Shooter->Soldier->GetBoneLocation(TEXT("hand_l"));
        Shooter->bMoving=false;Animation->TransitionTo(Shooter->GetAimMotion(),true,1.f,0,.20f);Step(0);
        Check(TEXT("v65_transition_starts_at_displayed_hand_pose"),FVector::Dist(Hand,Shooter->Soldier->GetBoneLocation(TEXT("hand_l")))<.15f);
        for(int32 I=0;I<7;++I)Step(1.f/60);
        Animation->TransitionTo(Shooter->GetIdleMotion(),true,1.f,.15f,.18f);for(int32 I=0;I<18;++I)Step(1.f/60);
        const auto D=Animation->GetCharacterDiagnostics();bool Finite=true;
        for(const auto& T:Shooter->Soldier->GetComponentSpaceTransforms())Finite&=!T.ContainsNaN();
        Check(TEXT("v65_interrupted_transition_retains_bounded_momentum"),Finite&&D.InterruptedTransitions>0&&D.InertialBlendFramesV65>0&&D.MaximumInertialAngleV65<=7.01f);
        Check(TEXT("v65_transition_keeps_anatomical_arm_lengths"),D.MaximumArmLengthErrorCm<.1f&&D.InvalidBoneFrames==0);
    }
    else Check(TEXT("v65_animation_proxy_present"),false);
    const auto Source=FFrontlineKillSourceV65::Soldier(Shooter);
    Victim->ReceiveHit(1,0,0,FVector::ForwardVector,EFrontlineDamageKind::Bullet,1,Source);Victim->ReceiveHit(1,0,0,FVector::ForwardVector,EFrontlineDamageKind::Bullet,1,Source);
    Check(TEXT("v65_real_fatal_hit_credits_once"),B->FindSquad(Shooter->SquadId)->KillsV65.Infantry==1&&B->CombatRecordV65[0].Total.Infantry==1);
    Flame->ReceiveHit(1,0,0,FVector::ForwardVector,EFrontlineDamageKind::Bullet,1,Source);
    Check(TEXT("v65_protected_hit_is_not_a_kill"),Flame->IsAlive()&&B->CombatRecordV65[0].Total.Infantry==1);
    Shooter->ReceiveHit(1,0,1,FVector::ForwardVector,EFrontlineDamageKind::Bullet); // A projectile already in flight survives its owner.
    Flame->ReceiveHit(1,0,0,FVector::ForwardVector,EFrontlineDamageKind::Grenade,1,Source);
    Check(TEXT("v65_dead_shooter_retains_delayed_credit"),B->FindSquad(Shooter->SquadId)->KillsV65.Infantry==2);
    FriendlyUnit->ReceiveHit(1,0,0,FVector::ForwardVector,EFrontlineDamageKind::Artillery,1,Source);
    Check(TEXT("v65_friendly_fire_is_loss_not_kill"),B->CombatRecordV65[0].Total.Infantry==2&&B->CombatRecordV65[0].InfantryLost==2);
    Tank=B->GetWorld()->SpawnActor<AFrontlineTank>();const bool Configured=Tank&&Tank->Configure(B,EFrontlineVehicle::A7V,1,65001);
    if(Configured)
    {
        Tank->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,10000,0,Source);Check(TEXT("v65_track_loss_does_not_credit_armour_kill"),B->CombatRecordV65[0].Total.Armour==0);
        Tank->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,Source);Tank->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,Source);
        Check(TEXT("v65_hull_death_credits_exactly_one_armour"),B->CombatRecordV65[0].Total.Armour==1&&B->FindSquad(Shooter->SquadId)->KillsV65.Armour==1);
    }
    else Check(TEXT("v65_armour_fixture_spawn"),false);
    FJsonObject J;WriteArchive(J,B->CombatRecordV65[0]);FFrontlineCombatRecordV65 Roundtrip;
    Check(TEXT("v65_detailed_archive_roundtrip"),ReadArchive(J,Roundtrip,false)&&Roundtrip.Total.Infantry==2&&Roundtrip.Total.Armour==1&&Roundtrip.InfantrySources[0].Infantry==2&&Roundtrip.InfantryLost==2);
    FJsonObject Legacy;FFrontlineCombatRecordV65 Migrated;Check(TEXT("v65_legacy_archive_no_fabricated_kills"),ReadArchive(Legacy,Migrated,true)&&Migrated.bIncludesEarlierUnrecordedBattles&&Migrated.Total.Total()==0);
    J.GetObjectField(TEXT("combat_record_v65"))->GetObjectField(TEXT("total"))->SetNumberField(TEXT("infantry"),999);
    Check(TEXT("v65_inconsistent_stats_rejected_without_overwrite"),!ReadArchive(J,Roundtrip,false)&&Roundtrip.Total.Infantry==2);
    // A codec fixture proves stable-key closure; these are not earned kills.
    FFrontlineCombatRecordV65 National;National.Total={2,1};National.NationalArmourSourcesV82.Add(TEXT("ru.austin_series1"),National.Total);National.BattlesRecorded=1;
    FJsonObject NationalJson;WriteArchive(NationalJson,National);FFrontlineCombatRecordV65 NationalRead;
    const bool NationalOK=ReadArchive(NationalJson,NationalRead,false);
    const auto* Austin=NationalRead.NationalArmourSourcesV82.Find(TEXT("ru.austin_series1"));
    Check(TEXT("v82_national_armour_archive_stable_key_roundtrip"),NationalOK&&Austin&&Austin->Infantry==2&&Austin->Armour==1&&NationalRead.OtherSources.Total()==0&&NationalRead.BattlesRecorded==1);
    NationalJson.GetObjectField(TEXT("combat_record_v65"))->GetObjectField(TEXT("national_armour_sources_v82"))->SetObjectField(TEXT("unknown.vehicle"),WriteTally({}));
    Check(TEXT("v82_unknown_national_armour_key_rejected_without_overwrite"),!ReadArchive(NationalJson,NationalRead,false)&&NationalRead.Total.Infantry==2&&NationalRead.NationalArmourSourcesV82.Num()==1);
}
TArray<AWarfareFrontlineSoldier*> PrepareCharacterReview(AWarfareFrontlineGameMode* B)
{
    TArray<AWarfareFrontlineSoldier*> Out;FString UserDir;
    if(!B||!B->IsProgressionVolatile()||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return Out;
    for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Reset();B->Squads.Reset();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    for(int32 I=0;I<6;++I)
    {
        const int32 Team=I%2,Id=65500+I;const EFrontlineRole Role=I==1?EFrontlineRole::Lewis:I==2?EFrontlineRole::Officer:I==5?EFrontlineRole::Flamethrower:EFrontlineRole::Rifle;
        FVector P((I%3-1)*430,I<3?-230:170,0);P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y)+88;
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(P,FRotator(0,Team?180:0,0));if(!U)continue;
        U->Initialize(B,Id,Team,Role,0);U->bFieldEntered=true;
        FFrontlineSquad S;S.Id=Id;S.Team=Team;S.Role=Role;S.bHoldPosition=true;S.Members.Add(U);B->Squads.Add(S);B->Units.Add(U);Out.Add(U);
        if(I<3)
        {
            U->SetActorTickEnabled(false); // Posed control examples; these are not live AI combatants.
            if(auto* A=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))A->TransitionTo(I==0?U->GetAimMotion():U->GetIdleMotion(),true,1.f,U->GetMotionPhase(),.18f);
            B->FindSquad(Id)->KillsV65.Infantry=I+2; // Labelled tooltip-layout fixture, never a campaign receipt.
        }
        else U->ReceiveHit(1,0,1-Team,FVector::ForwardVector,EFrontlineDamageKind::Fire,.2f);
    }
    return Out;
}
}
#include "WarfareFrontlineReportReviewV839.inl"
