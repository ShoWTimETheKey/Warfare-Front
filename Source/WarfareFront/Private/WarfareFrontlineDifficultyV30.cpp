#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineSupportArrivalV30.h"
#include "WarfareFrontlineGameplayV43.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Dom/JsonObject.h"

namespace FrontlineDifficultyV30
{
const FDoctrine& Doctrine(EFrontlineDifficulty Difficulty)
{
    // Supply concessions are selected explicitly in GameplayV43. These values
    // govern observed decisions; weapons and fatal hits remain shared.
    static const FDoctrine Rookie={1.06f,1.06f,1.06f,1.06f,6.36f,12.72f,2.97f,6.15f,4.f,122.f,3.41f,9.54f,16.96f,.38f};
    static const FDoctrine Veteran={.9858f,.848f,.8268f,.901f,4.77f,8.48f,2.54f,5.09f,8.f,106.f,3.20f,7.42f,13.78f,.32f};
    static const FDoctrine Survivor={.9116f,.689f,.636f,.742f,3.71f,6.36f,2.01f,4.03f,12.f,90.1f,2.94f,5.30f,10.60f,.27f};
    return Difficulty==EFrontlineDifficulty::Survivor?Survivor:Difficulty==EFrontlineDifficulty::Veteran?Veteran:Rookie;
}
const TCHAR* Label(EFrontlineDifficulty D)
{return D==EFrontlineDifficulty::Survivor?TEXT("去而复返"):D==EFrontlineDifficulty::Veteran?TEXT("深渊回忆"):TEXT("士兵之悔");}
const TCHAR* Description(EFrontlineDifficulty D)
{
    if(D==EFrontlineDifficulty::Survivor)return TEXT("敌军保留预备队，寻找暴露的火力点。炮火与突击接踵而至。");
    if(D==EFrontlineDifficulty::Veteran)return TEXT("敌军更快组织梯队，用机枪守住缺口，并以炮火掩护推进。");
    return TEXT("敌军进攻之间留有整队的间隙。前线仍有时间等待增援。");
}
bool ReadArchiveDifficulty(const FJsonObject& Json,EFrontlineDifficulty& Difficulty)
{
    Difficulty=EFrontlineDifficulty::Rookie;if(!Json.HasField(TEXT("difficulty")))return true;
    double Value=0;if(!Json.HasTypedField<EJson::Number>(TEXT("difficulty")) || !Json.TryGetNumberField(TEXT("difficulty"),Value) || !FMath::IsFinite(Value)
        || Value<0 || Value>2 || FMath::FloorToDouble(Value)!=Value)return false;
    Difficulty=EFrontlineDifficulty(uint8(Value));return true;
}
bool CanObserve(const AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* Unit)
{
    if(!B || !Unit || Unit->Team!=0 || !Unit->IsActiveCombatant())return false;
    for(const AWarfareFrontlineSoldier* Observer:B->Units)if(Observer && Observer->Team==1 && Observer->IsActiveCombatant()
        && FVector::Dist2D(Observer->Feet(),Unit->Feet())<=FMath::Max(4800.f,FrontlineWeapons::ProfileFor(Observer).FireRangeCm)*B->EncounterVisibilityScale()
        && B->DirectFireMultiplier(Observer->Feet(),Unit->Feet())>=.95f && B->HasSight(Observer,Unit))return true;
    return false;
}
int32 SupportSeed(AWarfareFrontlineGameMode* B,int32 Team,int32 PlayerSeed)
{
    if(Team!=1)return PlayerSeed;
    // The opposing battery owns its random sequence. Player calls do not
    // alter the opponent's dispersal, arrival time or ammunition scheduling.
    auto& S=State(B);return 301917+(++S.SupportSeedSerial)*1879;
}
FState& State(AWarfareFrontlineGameMode* Battle)
{
    static TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FState> States;
    for(auto It=States.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& S=States.FindOrAdd(Battle);
    if(Battle && S.Encounter!=Battle->GetEncounterId()){S=FState();S.Encounter=Battle->GetEncounterId();}
    return S;
}
void Reset(AWarfareFrontlineGameMode* Battle)
{auto& S=State(Battle);S=FState();if(Battle)S.Encounter=Battle->GetEncounterId();}
void Observe(AWarfareFrontlineGameMode* B,const TArray<AWarfareFrontlineSoldier*>& Seen)
{
    auto& S=State(B);const auto& D=Doctrine(B->GetCampaignDifficulty());
    if(B->Elapsed<=S.LastContactUpdate)return;S.LastContactUpdate=B->Elapsed;
    for(auto* U:Seen)
    {
        if(!U || !U->IsActiveCombatant())continue;
        FContact* C=S.Contacts.FindByPredicate([U](const FContact& Item){return Item.Identity==U;});
        if(!C){FContact New;New.Identity=U;S.Contacts.Add(New);C=&S.Contacts.Last();}
        const float DT=B->Elapsed-C->LastSeen;
        // Velocity is estimated only from two observations. Never query a lost
        // contact's transform, health, orders, target or weapon readiness.
        C->ObservedVelocity=C->LastSeen>0 && DT>.05f && DT<3.f?(U->Feet()-C->Position)/DT:FVector::ZeroVector;
        C->ObservedVelocity=C->ObservedVelocity.GetClampedToMaxSize(400.f);
        C->Position=U->Feet();C->LastSeen=B->Elapsed;C->Moving=U->bMoving;
        C->Machinegun=U->IsAutomaticWeapon();C->Leader=U->IsSectionLeader();C->Fired=U->ShotsDischarged>0;
        C->Rifle=U->Role==EFrontlineRole::Rifle;
        C->Entrenched=B->CoverFor(U)>0;C->Pressure=U->Suppression;++S.Observations;
    }
    S.ExpiredContactCount+=S.Contacts.RemoveAll([B,&D](const FContact& C){return B->Elapsed-C.LastSeen>D.MemorySeconds;});
}
float Confidence(const FContact& C,float Now,const FDoctrine& D)
{return FMath::Clamp(1.f-(Now-C.LastSeen)/FMath::Max(.1f,D.MemorySeconds),0.f,1.f);}
float RememberedStrength(const TArray<FContact>& Contacts,float Now,const FDoctrine& D,float TargetX,float Radius)
{
    float Strength=0;
    for(const auto& C:Contacts)if(FMath::Abs(C.Position.X-TargetX)<Radius)Strength+=Confidence(C,Now,D);
    return Strength;
}
bool CanAssault(float Friendly,float Observed,float Remembered,float Pressure,bool Gun,bool Counterattack)
{
    // No observation is not proof of an empty position. An unobserved approach
    // needs a formed wave; remembered defenders still count until they expire.
    if(Observed<=0)return Friendly>=6.f&&Friendly>Remembered*1.5f;
    const bool Suppressed=Pressure>.60f;
    return Friendly>FMath::Max(Observed,Remembered)*(Suppressed?.75f:Gun?1.75f:1.25f)*(Counterattack?.90f:1.f);
}
float TargetScore(const FContact& C,const TArray<FContact>& Contacts,float Now,const FDoctrine& D)
{
    const float Certainty=Confidence(C,Now,D);if(Certainty<=0)return 0;
    float Score=(C.Machinegun&&C.Fired?2.8f:0)+(C.Leader?.75f:0)+(C.Entrenched?.55f:0);
    for(const auto& Nearby:Contacts)if(FVector::Dist2D(C.Position,Nearby.Position)<520)
        Score+=Confidence(Nearby,Now,D)*(Nearby.Moving?(Nearby.Rifle?.75f:.50f):1.f);
    // A moving contact quickly becomes a poor artillery target during the real
    // transmission delay. A remembered fixed gun remains useful for longer.
    return Score*Certainty*(C.Moving?FMath::Max(0.f,1.f-(Now-C.LastSeen)*.25f):1.f);
}
FVector RegisteredPoint(const FContact& C,float Now,const FDoctrine& D,FRandomStream& R)
{
    FVector P=C.Position;
    const float Age=FMath::Max(0.f,Now-C.LastSeen);
    // Only modest lead from a previously observed course. No prediction through
    // the full 12–22 second battery delay and no homing after commitment.
    if(C.Moving)P+=C.ObservedVelocity*FMath::Min(.65f,Age+.25f);
    P+=FVector(R.FRandRange(-D.RegistrationError,D.RegistrationError),R.FRandRange(-D.RegistrationError*.75f,D.RegistrationError*.75f),0);
    P.X=FMath::Clamp(P.X,-double(FrontlineBattlefield::SupportHalfLength-200),double(FrontlineBattlefield::SupportHalfLength-200));
    const double Flank=FrontlineBattlefield::HalfFrontage-90.f;
    P.Y=FMath::Clamp(P.Y,-Flank,Flank);return P;
}
}

TSharedRef<FJsonObject> RunFrontlineV30TacticsChecks(AWarfareFrontlineGameMode* B)
{
    using namespace FrontlineDifficultyV30;
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
    auto Check=[&](const TCHAR* Name,bool Pass){auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Pass);Rows.Add(MakeShared<FJsonValueObject>(Row));};
    const auto& R=Doctrine(EFrontlineDifficulty::Rookie);const auto& V=Doctrine(EFrontlineDifficulty::Veteran);const auto& S=Doctrine(EFrontlineDifficulty::Survivor);
    Check(TEXT("old_archives_default_rookie"),FFrontlineCampaignProgress().Difficulty==EFrontlineDifficulty::Rookie);
    FJsonObject Archive;EFrontlineDifficulty Loaded=EFrontlineDifficulty::Survivor;
    Check(TEXT("missing_archive_field_migrates_without_resetting_campaign"),ReadArchiveDifficulty(Archive,Loaded)&&Loaded==EFrontlineDifficulty::Rookie);
    bool Roundtrip=true;for(int32 I=0;I<3;++I){Archive.SetNumberField(TEXT("difficulty"),I);Roundtrip&=ReadArchiveDifficulty(Archive,Loaded)&&int32(Loaded)==I;}
    Check(TEXT("all_three_archive_values_roundtrip"),Roundtrip);
    Archive.SetNumberField(TEXT("difficulty"),1.5);const bool FractionRejected=!ReadArchiveDifficulty(Archive,Loaded);
    Archive.SetNumberField(TEXT("difficulty"),3);const bool FutureRejected=!ReadArchiveDifficulty(Archive,Loaded);
    Archive.SetStringField(TEXT("difficulty"),TEXT("2"));Check(TEXT("malformed_difficulty_is_not_silently_clamped"),FractionRejected&&FutureRejected&&!ReadArchiveDifficulty(Archive,Loaded));
    Check(TEXT("difficulty_improves_cadence_without_weapon_or_health_multipliers"),R.RecruitScale>1&&S.RecruitScale<V.RecruitScale&&V.RecruitScale<R.RecruitScale&&S.DecisionScale<V.DecisionScale&&V.DecisionScale<R.DecisionScale);
    Check(TEXT("all_orders_retain_transmission_time"),R.ReactionMin>0&&V.ReactionMin>0&&S.ReactionMin>0);
    Check(TEXT("v59_survivor_has_no_blanket_supply_handicap_and_player_is_unchanged"),
        FrontlineGameplayV43::PreparationSeconds(30,1,EFrontlineDifficulty::Survivor)==30.f
        &&FrontlineGameplayV43::PreparationSeconds(30,1,EFrontlineDifficulty::Veteran)>30.f
        &&FrontlineGameplayV43::PreparationSeconds(30,1,EFrontlineDifficulty::Rookie)==40.f
        &&FrontlineGameplayV43::PreparationSeconds(30,0,EFrontlineDifficulty::Rookie)==30.f
        &&FrontlineGameplayV43::PreparationSeconds(30,0,EFrontlineDifficulty::Survivor)==30.f);
    Check(TEXT("v59_unseen_line_is_not_a_free_assault_and_observed_guns_need_strength"),
        !CanAssault(5,0,0,0,false,false)&&!CanAssault(8,0,8,0,false,false)
        &&CanAssault(8,0,0,0,false,false)&&!CanAssault(8,6,6,0,true,false)
        &&CanAssault(8,6,6,.8f,true,false));
    FContact Gun;Gun.Position=FVector(1500,0,0);Gun.LastSeen=10;Gun.Machinegun=Gun.Fired=Gun.Entrenched=true;
    FContact Walker=Gun;Walker.Moving=true;Walker.Machinegun=Walker.Fired=Walker.Entrenched=false;
    TArray<FContact> Contacts={Gun,Walker};
    Check(TEXT("visible_fixed_gun_outscores_a_running_singleton"),TargetScore(Gun,Contacts,10,S)>TargetScore(Walker,Contacts,10,S));
    Check(TEXT("lost_moving_target_loses_value"),TargetScore(Walker,Contacts,13,S)<TargetScore(Walker,Contacts,10,S));
    Check(TEXT("expired_observation_cannot_authorize_fire"),TargetScore(Gun,Contacts,23,S)==0);
    FRandomStream RandomA(191730),RandomC(191730);const FVector PointA=RegisteredPoint(Gun,11,S,RandomA);Gun.Identity.Reset();const FVector PointC=RegisteredPoint(Gun,11,S,RandomC);
    Check(TEXT("registered_target_uses_observation_values_without_actor_readback"),PointA.Equals(PointC,.001f));
    Check(TEXT("frontage_shortened_consistently"),FrontlineBattlefield::TrenchSpacing==3680&&FrontlineBattlefield::SpawnHalfLength==10488&&FrontlineBattlefield::GoalHalfLength==10856&&FrontlineBattlefield::SupportHalfLength==11040);
    if(B)
    {
        auto SavedState=State(B);ON_SCOPE_EXIT{State(B)=SavedState;};
        State(B).SupportSeedSerial=7;const int32 SeedA=SupportSeed(B,1,123);
        State(B).SupportSeedSerial=7;SupportSeed(B,0,987);const int32 SeedB=SupportSeed(B,1,99999);
        Check(TEXT("player_support_calls_do_not_change_enemy_mission_rng"),SeedA==SeedB);
        FString UserDir;
        const bool Isolated=(FParse::Param(FCommandLine::Get(),TEXT("WFV30Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV30TacticsVerify")))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
        if(Isolated)
        {
            const float SavedClock=B->Elapsed;const auto SavedPhase=B->Phase;
            TArray<AActor*> Owned;ON_SCOPE_EXIT{B->Elapsed=SavedClock;B->Phase=SavedPhase;for(auto* Actor:Owned)if(IsValid(Actor))Actor->Destroy();};
            B->Phase=EFrontlinePhase::Fighting;B->Elapsed=100;
            FFrontlineStrike Strike;Strike.LaunchedAt=105;Strike.ImpactAt=108;Strike.Origin=FVector(0,0,1800);Strike.Target=FVector(1300,0,0);
            EnsureFrontlineSupportProjectileV30(B,Strike);const bool HiddenBefore=!Strike.Projectile.IsValid();
            B->Elapsed=105.1f;EnsureFrontlineSupportProjectileV30(B,Strike);
            auto* Shell=Strike.Projectile.Get();if(Shell)Owned.Add(Shell);
            const auto* Component=Shell?Shell->FindComponentByClass<UStaticMeshComponent>():nullptr;
            Check(TEXT("artillery_visual_spawns_at_launch_with_mesh_material_and_no_collision"),HiddenBefore&&Component&&Component->GetStaticMesh()
                &&Component->GetMaterial(0)&&Component->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!Strike.bResolved);
            auto* Arrival=B->GetWorld()->SpawnActor<AFrontlineSupportArrivalV30>();if(Arrival)Owned.Add(Arrival);
            bool Before=false,InFlight=false,Ended=false;
            if(Arrival&&Arrival->Configure(B,FVector(900,0,0),0,110,false))
            {
                auto* C=Arrival->FindComponentByClass<UStaticMeshComponent>();
                Arrival->Tick(0);Before=C&&!C->IsVisible();B->Elapsed=109;Arrival->Tick(0);
                InFlight=C&&C->IsVisible()&&Arrival->GetActorLocation().Z>0;
                B->Elapsed=110;Arrival->Tick(0);Ended=Arrival->IsActorBeingDestroyed();
            }
            Check(TEXT("smoke_gas_carrier_is_visible_in_flight_and_removed_at_release_without_damage"),Before&&InFlight&&Ended);
        }
        Report->SetBoolField(TEXT("live_arrival_fixture_ran"),Isolated);
    }
    int32 Passed=0;for(const auto& Row:Rows)Passed+=Row->AsObject()->GetBoolField(TEXT("passed"))?1:0;
    Report->SetArrayField(TEXT("checks"),Rows);Report->SetNumberField(TEXT("passed"),Passed);Report->SetNumberField(TEXT("total"),Rows.Num());Report->SetBoolField(TEXT("success"),Passed==Rows.Num());
    Report->SetStringField(TEXT("scope"),TEXT("Production archive parser, doctrine, observed contact scoring/expiry and independent mission RNG; opt-in isolated real shell actors when WFV30Review/UserDir are supplied. Full campaign difficulty balance still requires play sessions."));
    return Report;
}
