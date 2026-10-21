#include "WarfareFrontlineArmourValourV8221.h"
// Included after the achievement namespace: this shares its existing account
// mutex and eligibility rules; the new tally has its own primary/mirror ledger.
namespace FrontlineArmourKillTallyV8220
{
using namespace FrontlineAchievementsV76;
namespace
{
FGuid ReceiptFor(const FGuid& Encounter,int32 VictimTeam,int32 VictimSerial)
{
    FGuid Receipt;
    const FString Digest=FMD5::HashAnsiString(*FString::Printf(TEXT("%s:infantry-armour-v8220:%d:%d"),
        *Encounter.ToString(EGuidFormats::Digits),VictimTeam,VictimSerial));
    FGuid::ParseExact(Digest,EGuidFormats::Digits,Receipt);return Receipt;
}
bool ReviewScope(AWarfareFrontlineGameMode* B)
{
    FString Dir;
    const bool TallyReview=FParse::Param(FCommandLine::Get(),TEXT("WFV8220Review"));
    const bool IntegratedWallReview=FParse::Param(FCommandLine::Get(),TEXT("WFV8222Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8221Review"));
    return B&&FrontlineEffectsReviewSafetyV8216::Allowed(B)&&(TallyReview||IntegratedWallReview)
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&!WarfareFRRUCampaignV82::Active(B)
        &&!FrontlineUltimateArchiveV8215::Active(B)&&B->GetCurrentCampaign()&&B->GetWorld()->IsGameWorld();
}
}
int32 Count(EFRRUNationV81 Nation)
{
    LoadArmourTallyV8220();const int32 N=int32(Nation);return N>=0&&N<4&&!ArmourTallyLedgerV8220.Invalid?ArmourTallyLedgerV8220.Totals[N]:0;
}
int32 Count(EFrontlineFaction Faction){return Count(EFRRUNationV81(uint8(Faction)));}
FString Description(EFRRUNationV81 Nation,bool Chinese)
{
    return FrontlineArmourValourV8221::Description(FrontlineArmourValourV8221::Display(Nation,Count(Nation)),Chinese);
}
FNotification TakeNotification()
{
    Load();if(!Account.Unsupported&&!PendingArmourTallyV8220.IsEmpty()&&FPlatformTime::Seconds()>=ArmourTallyRetryV8220)PersistArmourTallyV8220();
    if(ArmourKillNotificationsV8220.IsEmpty())return {};
    const auto Event=ArmourKillNotificationsV8220[0];ArmourKillNotificationsV8220.RemoveAt(0);return Event;
}
void RecordDestroyedArmour(AWarfareFrontlineGameMode* B,int32 VictimTeam,int32 VictimSerial,const FFrontlineKillSourceV65& Source)
{
    if(!B||B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE||VictimTeam!=1||VictimSerial<=0
        ||Source.Team!=0||Source.SquadId==INDEX_NONE||Source.Role<0||Source.Role>=int32(EFrontlineRole::Count)
        ||Source.VehicleSerial!=INDEX_NONE||Source.VehicleType!=INDEX_NONE||Source.Support!=INDEX_NONE
        ||Source.FireMissionId!=INDEX_NONE||!Source.NationalVehicleIdV82.IsEmpty())return;
    const bool Expanded=WarfareFRRUCampaignV82::Active(B);const int32 Nation=int32(WarfareFRRUCampaignV82::Nation(B,0));const auto* Campaign=B->GetCurrentCampaign();
    const FGuid CampaignId=Expanded?WarfareFRRUCampaignV82::CampaignId(B):Campaign?Campaign->Id:FGuid();
    const bool HonourEligible=Expanded?WarfareFRRUCampaignV82::HonourEligible(B):Campaign&&Campaign->bAchievementEligibleV76;
    Load();LoadArmourTallyV8220();const auto& Encounter=State(B);
    if(Nation<0||Nation>3||!CampaignId.IsValid()||!B->GetEncounterId().IsValid()
        ||!Encounter.Started||!Encounter.Eligible||Encounter.Id!=B->GetEncounterId()||Encounter.Campaign!=CampaignId
        ||!HonourEligible||Account.Corrupt||Account.Unsupported||ArmourTallyLedgerV8220.Invalid||Account.Tainted.Contains(CampaignId)
        ||FrontlineDebugV37::HasCheatState(B)||(B->IsProgressionVolatile()&&!IsolatedAwardsAllowed(B)&&!FrontlineUltimateArchiveV8215::ReplayEligible(B)))return;
    // Attribution was captured at discharge. The shooter may already be dead;
    // requiring a live soldier here would erase his grenade's final result.
    if(const auto* Section=B->FindSquad(Source.SquadId);Section&&Section->Team!=0)return;
    const FGuid Receipt=ReceiptFor(B->GetEncounterId(),VictimTeam,VictimSerial);
    if(!Receipt.IsValid()||ArmourTallyLedgerV8220.Receipts.Contains(Receipt))return;
    // Bound untrusted persistent input. Exhaustion must not block unrelated
    // historical decoration/achievement settlements in the shared account.
    if(!PendingArmourTallyV8220.Contains(Receipt)&&ArmourTallyLedgerV8220.Receipts.Num()+PendingArmourTallyV8220.Num()>=MaximumArmourKillReceiptsV8220)return;
    auto& Pending=PendingArmourTallyV8220.FindOrAdd(Receipt);Pending.Campaign=CampaignId;
    Pending.Notice.Nation=EFRRUNationV81(Nation);Pending.Notice.Encounter=B->GetEncounterId();Pending.Notice.Receipt=Receipt;
    PersistArmourTallyV8220();
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=ReviewScope(B);
    Check(TEXT("v8220_armour_tally_strict_disposable_actual_world"),Safe);if(!Safe)return;
    FTransientReviewV81 QueueScope;Load();const auto BeforeAccount=Account;const auto BeforeEncounter=State(B);
    const auto BeforeAwardBattle=IsolatedAwardBattle;const auto BeforeUnits=B->Units;const auto BeforeSquads=B->Squads;
    const auto BeforePhase=B->Phase;const int32 BeforeOutro=B->BattleOutroWinner;
    const FFrontlineCombatRecordV65 BeforeRecords[]={B->CombatRecordV65[0],B->CombatRecordV65[1]};
    const float BeforeMorale[]={B->Morale[0],B->Morale[1]};const int32 BeforeCasualties[]={B->Casualties[0],B->Casualties[1]};
    const float BeforeDamage[]={B->DamageDealt[0],B->DamageDealt[1]},BeforeFriendly=B->FriendlyFireDamage;
    const FString BeforeMessage=B->LastMessage;
    const FString Path=AccountPath(),Backup=Path+TEXT(".bak"),TallyPath=ArmourTallyPathV8220(),TallyBackup=TallyPath+TEXT(".bak");TArray<uint8> PrimaryBytes,BackupBytes,TallyBytes,TallyBackupBytes;
    const bool HadPrimary=IFileManager::Get().FileExists(*Path),HadBackup=IFileManager::Get().FileExists(*Backup);
    const bool HadTally=IFileManager::Get().FileExists(*TallyPath),HadTallyBackup=IFileManager::Get().FileExists(*TallyBackup);
    if((HadPrimary&&!FFileHelper::LoadFileToArray(PrimaryBytes,*Path))||(HadBackup&&!FFileHelper::LoadFileToArray(BackupBytes,*Backup))
        ||(HadTally&&!FFileHelper::LoadFileToArray(TallyBytes,*TallyPath))||(HadTallyBackup&&!FFileHelper::LoadFileToArray(TallyBackupBytes,*TallyBackup)))
    {Check(TEXT("v8220_armour_tally_disposable_snapshot"),false);return;}
    FFrontlineAchievementReviewV76 CampaignScope(B);
    TArray<AWarfareFrontlineSoldier*> Made;TArray<AFrontlineTank*> Tanks;
    ON_SCOPE_EXIT
    {
        for(auto* Unit:Made)if(IsValid(Unit))Unit->Destroy();for(auto* Tank:Tanks)if(IsValid(Tank))Tank->Destroy();
        Account=BeforeAccount;IsolatedAwardBattle=BeforeAwardBattle;Encounters.Add(B,BeforeEncounter);
        B->Units=BeforeUnits;B->Squads=BeforeSquads;B->Phase=BeforePhase;B->BattleOutroWinner=BeforeOutro;B->LastMessage=BeforeMessage;
        for(int32 N=0;N<2;++N){B->CombatRecordV65[N]=BeforeRecords[N];B->Morale[N]=BeforeMorale[N];B->Casualties[N]=BeforeCasualties[N];B->DamageDealt[N]=BeforeDamage[N];}B->FriendlyFireDamage=BeforeFriendly;
        if(HadPrimary)FFileHelper::SaveArrayToFile(PrimaryBytes,*Path);else IFileManager::Get().Delete(*Path);
        if(HadBackup)FFileHelper::SaveArrayToFile(BackupBytes,*Backup);else IFileManager::Get().Delete(*Backup);
        if(HadTally)FFileHelper::SaveArrayToFile(TallyBytes,*TallyPath);else IFileManager::Get().Delete(*TallyPath);
        if(HadTallyBackup)FFileHelper::SaveArrayToFile(TallyBackupBytes,*TallyBackup);else IFileManager::Get().Delete(*TallyBackup);
        TArray<uint8> P,K,T,L;Check(TEXT("v8220_armour_tally_restores_isolated_account_bytes"),
            (HadPrimary?FFileHelper::LoadFileToArray(P,*Path)&&P==PrimaryBytes:!IFileManager::Get().FileExists(*Path))
            &&(HadBackup?FFileHelper::LoadFileToArray(K,*Backup)&&K==BackupBytes:!IFileManager::Get().FileExists(*Backup))
            &&(HadTally?FFileHelper::LoadFileToArray(T,*TallyPath)&&T==TallyBytes:!IFileManager::Get().FileExists(*TallyPath))
            &&(HadTallyBackup?FFileHelper::LoadFileToArray(L,*TallyBackup)&&L==TallyBackupBytes:!IFileManager::Get().FileExists(*TallyBackup)));
    };
    IsolatedAwardBattle=B;B->Units.Empty();B->Squads.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    Account=FAccount();Account.Loaded=true;ArmourTallyLedgerV8220=FArmourTallyLedgerV8220();ArmourTallyLedgerV8220.Loaded=true;bool InitialSaved=false;
    IFileManager::Get().Delete(*TallyPath);IFileManager::Get().Delete(*TallyBackup);
    {FAccountLock Lock;InitialSaved=Lock.Valid()&&SaveLocked();}
    Check(TEXT("v8220_armour_tally_empty_account_committed"),InitialSaved);if(!InitialSaved)return;
    CampaignScope.Campaign(EFrontlineFaction::British);BeginEncounter(B);
    int32 NextSerial=8220500;
    auto Soldier=[&](EFrontlineRole Role)
    {
        const int32 Id=8220600+Made.Num();auto* Unit=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
        if(!Unit)return Unit;Unit->Initialize(B,Id,0,Role,0);Unit->SetActorTickEnabled(false);Unit->bFieldEntered=true;
        Unit->SetActorLocation(FVector(0,Made.Num()*120.f,200));Made.Add(Unit);B->Units.Add(Unit);
        FFrontlineSquad Section;Section.Id=Id;Section.Team=0;Section.Role=Role;Section.Members.Add(Unit);B->Squads.Add(Section);return Unit;
    };
    auto Vehicle=[&](int32 Team)
    {
        auto* Tank=B->GetWorld()->SpawnActor<AFrontlineTank>();if(!Tank)return Tank;
        if(!Tank->Configure(B,EFrontlineVehicle::A7V,Team,++NextSerial)){Tank->Destroy();return static_cast<AFrontlineTank*>(nullptr);}
        FFrontlineArmourKillTallyV8220Review::Enter(Tank);Tank->SetActorTickEnabled(false);Tanks.Add(Tank);return Tank;
    };
    auto* Grenadier=Soldier(EFrontlineRole::Mills);auto* RifleGrenadier=Soldier(EFrontlineRole::RifleGrenadier);auto* AntiTank=Soldier(EFrontlineRole::AntiTankRifle);
    Check(TEXT("v8220_armour_tally_actual_registered_infantry"),Grenadier&&RifleGrenadier&&AntiTank&&Grenadier->GetRootComponent()->IsRegistered()&&Grenadier->Soldier->IsRegistered());
    if(!Grenadier||!RifleGrenadier||!AntiTank)return;
    const auto GrenadeSource=FFrontlineKillSourceV65::Soldier(Grenadier);
    auto* First=Vehicle(1);Check(TEXT("v8220_armour_tally_real_vehicle_fixture"),First&&First->GetRootComponent()->IsRegistered());if(!First)return;
    First->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,10000,0,GrenadeSource);
    Check(TEXT("v8220_armour_tally_track_disable_is_not_kill"),Count(EFrontlineFaction::British)==0&&TakeNotification().Nation==EFRRUNationV81::Invalid);
    const int32 BeforeArmour=B->CombatRecordV65[0].Total.Armour;
    First->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,GrenadeSource);const auto FirstNotice=TakeNotification();
    First->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,GrenadeSource);FrontlineCombatRecordV65::ArmourDestroyedV8220(B,First,GrenadeSource);
    Check(TEXT("v8220_armour_tally_first_fatal_hull_emits_once_and_stats_once"),FirstNotice.Nation==EFRRUNationV81::British&&FirstNotice.Total==1&&FirstNotice.Encounter==B->GetEncounterId()&&FirstNotice.Receipt.IsValid()
        &&Count(EFrontlineFaction::British)==1&&B->CombatRecordV65[0].Total.Armour==BeforeArmour+1&&TakeNotification().Nation==EFRRUNationV81::Invalid);
    for(auto* Shooter:{RifleGrenadier,AntiTank})
    {
        auto* Victim=Vehicle(1);if(!Victim){Check(TEXT("v8220_armour_tally_additional_vehicle_fixture"),false);return;}
        Victim->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,FFrontlineKillSourceV65::Soldier(Shooter));const auto Event=TakeNotification();
        Check(FString::Printf(TEXT("v8220_armour_tally_role_%d_retains_individual_fatal_attribution"),int32(Shooter->Role)),Event.Nation==EFRRUNationV81::British&&Event.Receipt.IsValid()&&Event.Total==Count(EFrontlineFaction::British));
    }
    Check(TEXT("v8220_armour_tally_three_infantry_weapons_accumulate"),Count(EFrontlineFaction::British)==3);
    Check(TEXT("v8220_armour_tally_does_not_fabricate_historical_decorations"),Account.MedalAwardsV8216.IsEmpty()&&Account.CheatMedalsV8218.IsEmpty());
    Grenadier->ReceiveHit(1,0,1,FVector::ForwardVector,EFrontlineDamageKind::Bullet);
    auto* Delayed=Vehicle(1);if(!Delayed){Check(TEXT("v8220_armour_tally_delayed_vehicle_fixture"),false);return;}
    Delayed->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,GrenadeSource);
    Check(TEXT("v8220_armour_tally_dead_thrower_retains_delayed_grenade"),!Grenadier->IsAlive()&&Count(EFrontlineFaction::British)==4&&TakeNotification().Total==4);
    auto* Friendly=Vehicle(0);auto* GunVictim=Vehicle(1);auto* TankVictim=Vehicle(1);
    if(!Friendly||!GunVictim||!TankVictim){Check(TEXT("v8220_armour_tally_exclusion_vehicle_fixture"),false);return;}
    const auto VehicleSource=FFrontlineKillSourceV65::Vehicle(Friendly);
    Friendly->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,GrenadeSource);
    GunVictim->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::AntiTank)));
    TankVictim->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,VehicleSource);
    Check(TEXT("v8220_armour_tally_excludes_friendly_support_and_vehicle_kills"),Count(EFrontlineFaction::British)==4&&TakeNotification().Nation==EFRRUNationV81::Invalid);
    auto* Failed=Vehicle(1);if(!Failed){Check(TEXT("v8220_armour_tally_failed_save_vehicle_fixture"),false);return;}
    ReviewFailSaveV81=true;Failed->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,GrenadeSource);
    Check(TEXT("v8220_armour_tally_failed_primary_has_no_count_or_notice"),Count(EFrontlineFaction::British)==4&&TakeNotification().Nation==EFRRUNationV81::Invalid&&!PendingArmourTallyV8220.IsEmpty());
    ReviewFailSaveV81=false;ArmourTallyRetryV8220=0;const auto Retried=TakeNotification();
    RecordDestroyedArmour(B,1,Failed->Serial,GrenadeSource);
    Check(TEXT("v8220_armour_tally_retry_then_duplicate_preserves_one_record"),Retried.Total==5&&Count(EFrontlineFaction::British)==5&&TakeNotification().Nation==EFRRUNationV81::Invalid);
    auto* Mirror=Vehicle(1);if(!Mirror){Check(TEXT("v8220_armour_tally_mirror_vehicle_fixture"),false);return;}
    ReviewFailMirrorV81=true;Mirror->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,GrenadeSource);FArmourTallyLedgerV8220 Disk;
    Check(TEXT("v8220_armour_tally_primary_only_waits_for_mirror"),ReadArmourTallyFileV8220(TallyPath,Disk)&&Disk.Totals[0]==6&&Count(EFrontlineFaction::British)==5&&TakeNotification().Nation==EFRRUNationV81::Invalid&&!PendingArmourTallyV8220.IsEmpty());
    ReviewFailMirrorV81=false;ArmourTallyRetryV8220=0;const auto Mirrored=TakeNotification();
    Check(TEXT("v8220_armour_tally_mirror_retry_emits_once"),Mirrored.Total==6&&Count(EFrontlineFaction::British)==6&&TakeNotification().Nation==EFRRUNationV81::Invalid);
    CampaignScope.Run->Id=FGuid::NewGuid();CampaignScope.Campaign(EFrontlineFaction::German);BeginEncounter(B);
    auto* German=Vehicle(1);if(!German){Check(TEXT("v8220_armour_tally_german_vehicle_fixture"),false);return;}
    German->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,GrenadeSource);const auto GermanEvent=TakeNotification();
    Check(TEXT("v8220_armour_tally_cross_campaign_national_counts_remain_separate"),GermanEvent.Nation==EFRRUNationV81::German&&GermanEvent.Total==1&&Count(EFrontlineFaction::British)==6&&Count(EFrontlineFaction::German)==1&&Count(EFRRUNationV81::French)==0&&Count(EFRRUNationV81::Russian)==0);
    Account=FAccount();ArmourTallyLedgerV8220=FArmourTallyLedgerV8220();Load();Check(TEXT("v8220_armour_tally_account_reload_retains_counts"),!Account.Corrupt&&!Account.Unsupported&&Count(EFrontlineFaction::British)==6&&Count(EFrontlineFaction::German)==1);
    RecordDestroyedArmour(B,1,German->Serial,GrenadeSource);Check(TEXT("v8220_armour_tally_reloaded_receipt_cannot_repeat"),Count(EFrontlineFaction::German)==1&&TakeNotification().Nation==EFRRUNationV81::Invalid);
    FString SavedText;FFileHelper::LoadFileToString(SavedText,*TallyPath);TSharedPtr<FJsonObject> Json;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(SavedText),Json);
    if(Json.IsValid())
    {
        Json->SetNumberField(TEXT("uk"),7);FString BadText;FJsonSerializer::Serialize(Json.ToSharedRef(),TJsonWriterFactory<>::Create(&BadText));FArmourTallyLedgerV8220 Bad;
        Check(TEXT("v8220_armour_tally_rejects_forged_counter_receipt_mismatch"),!DecodeArmourTallyV8220(BadText,Bad));
    }
    else Check(TEXT("v8220_armour_tally_codec_readable_fixture"),false);
    // A supported old package rewrites only its known schema-5 main account.
    {FAccountLock Lock;Check(TEXT("v8220_armour_tally_old_account_rewrite_fixture"),Lock.Valid()&&SaveLocked());}
    Account=FAccount();ArmourTallyLedgerV8220=FArmourTallyLedgerV8220();Load();
    Check(TEXT("v8220_armour_tally_old_account_rewrite_preserves_sidecar"),Count(EFrontlineFaction::British)==6&&Count(EFrontlineFaction::German)==1);
    State(B).Eligible=false;auto* Assisted=Vehicle(1);if(!Assisted){Check(TEXT("v8220_armour_tally_assisted_vehicle_fixture"),false);return;}
    Assisted->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,GrenadeSource);
    Check(TEXT("v8220_armour_tally_assisted_encounter_never_awards"),Count(EFrontlineFaction::German)==1&&TakeNotification().Nation==EFRRUNationV81::Invalid);
    FString CheatResponse;const bool CheatGranted=GrantAllForCheatV8218(B,true,true,CheatResponse);
    Check(TEXT("v8220_armour_tally_all_honours_cheat_cannot_invent_kill_records"),CheatGranted&&Count(EFrontlineFaction::British)==6&&Count(EFrontlineFaction::German)==1&&TakeNotification().Nation==EFRRUNationV81::Invalid);
}
bool SeedNotificationReview(AWarfareFrontlineGameMode* B)
{
    if(!ReviewScope(B))return false;
    Load();LoadArmourTallyV8220();const auto OldEncounter=State(B);const auto OldAwardBattle=IsolatedAwardBattle;
    const auto OldPhase=B->Phase;const int32 OldOutro=B->BattleOutroWinner;
    const auto OldUnits=B->Units;const auto OldSquads=B->Squads;
    const FFrontlineCombatRecordV65 OldRecords[]={B->CombatRecordV65[0],B->CombatRecordV65[1]};const float OldMorale[]={B->Morale[0],B->Morale[1]};
    AWarfareFrontlineSoldier* Shooter=nullptr;AFrontlineTank* Victim=nullptr;
    ON_SCOPE_EXIT
    {
        if(IsValid(Shooter))Shooter->Destroy();if(IsValid(Victim))Victim->Destroy();
        Encounters.Add(B,OldEncounter);IsolatedAwardBattle=OldAwardBattle;B->Phase=OldPhase;B->BattleOutroWinner=OldOutro;
        B->Units=OldUnits;B->Squads=OldSquads;for(int32 N=0;N<2;++N){B->CombatRecordV65[N]=OldRecords[N];B->Morale[N]=OldMorale[N];}
    };
    IsolatedAwardBattle=B;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;BeginEncounter(B);
    Shooter=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();Victim=B->GetWorld()->SpawnActor<AFrontlineTank>();
    if(!Shooter||!Victim)return false;
    Shooter->Initialize(B,8220991,0,EFrontlineRole::AntiTankRifle,0);Shooter->SetActorTickEnabled(false);Shooter->bFieldEntered=true;
    B->Units.Add(Shooter);FFrontlineSquad Q;Q.Id=Shooter->SquadId;Q.Team=0;Q.Role=Shooter->Role;Q.Members.Add(Shooter);B->Squads.Add(Q);
    const int32 Serial=8221000+ArmourTallyLedgerV8220.Receipts.Num();
    if(!Victim->Configure(B,EFrontlineVehicle::A7V,1,Serial))return false;
    FFrontlineArmourKillTallyV8220Review::Enter(Victim);Victim->SetActorTickEnabled(false);
    const int32 Before=Count(B->GetPlayerFaction());Victim->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,FFrontlineKillSourceV65::Soldier(Shooter));
    return Count(B->GetPlayerFaction())==Before+1&&!ArmourKillNotificationsV8220.IsEmpty();
}
}
