#include "WarfareFrontlineSupportV32.h"
#include "WarfareFrontlineReportV839.h"
#include "WarfareFrontlineAirSupportV8220.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

namespace FrontlineSupportV32
{
namespace
{
struct FAirTask {TWeakObjectPtr<AActor> Aircraft;FVector Target;int32 Team=0,FireMission=INDEX_NONE,FirstRound=0,RoundCount=12;float Arrival=0,DrivenOffAt=-1;bool Recon=false,Reported=false;};
struct FState
{
    FGuid Encounter;float Ready[2][3]={{}};int32 Sequence[2]={};float EnemyThink=24;TArray<FAirTask> Air;
    FReconStatusV37 Recon[2];float ReconUntil[2]={};
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FState> States;
int32 Index(EFrontlineSupport S){return int32(S)-int32(EFrontlineSupport::CreepingBarrage);}
FVector Bound(FVector P){P.X=FrontlineFireSupportV26::ReflectInside(P.X,FrontlineBattlefield::SupportHalfLength);P.Y=FrontlineFireSupportV26::ReflectInside(P.Y,FrontlineBattlefield::HalfFrontage);P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y);return P;}
bool InReconSector(const FVector& Centre,const FVector& P)
{return FMath::Square((P.X-Centre.X)/3600.)+FMath::Square((P.Y-Centre.Y)/1300.)<=1.;}
void RecordRecon(AWarfareFrontlineGameMode* B,FState& S,const FAirTask& A,const FVector& Aircraft)
{
    auto& Intel=S.Recon[A.Team];Intel=FReconStatusV37();Intel.Centre=A.Target;
    S.ReconUntil[A.Team]=B->Elapsed+(B->HasTechnology(20,A.Team)?60.f:45.f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(V37AirObservation),true);
    if(A.Aircraft.IsValid())Params.AddIgnoredActor(A.Aircraft.Get());
    for(AWarfareFrontlineSoldier* U:B->Units)if(U)Params.AddIgnoredActor(U);
    auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld());
    if(Vehicles)for(AFrontlineTank* T:Vehicles->GetVehicles())if(T)Params.AddIgnoredActor(T);
    auto Visible=[&](const FVector& Point)
    {
        if(!InReconSector(A.Target,Point)||B->DirectFireMultiplier(Point,Point)<.95f)return false;
        return !B->GetWorld()->LineTraceTestByChannel(Aircraft,Point,ECC_Visibility,Params);
    };
    TArray<AWarfareFrontlineSoldier*> Seen;
    for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team!=A.Team&&U->bFieldEntered&&U->IsActiveCombatant()&&Visible(U->Feet()+FVector(0,0,100)))
    {++Intel.Infantry;if(U->IsMachineGunner())++Intel.Machineguns;Seen.Add(U);}
    if(Vehicles)for(AFrontlineTank* T:Vehicles->GetVehicles())if(T&&T->Team!=A.Team&&T->HasEnteredField()&&T->IsOperational()&&Visible(T->AimPoint()))++Intel.Armour;
    // This one observed pass feeds ordinary decaying AI contact memory. It never
    // follows hidden units, reads orders, or continuously refreshes their positions.
    if(A.Team==1)FrontlineDifficultyV30::Observe(B,Seen);
    FrontlineFieldV32::SetAirObservation(B,A.Team,A.Target,S.ReconUntil[A.Team]);
    if(A.Team==0)B->LastMessage=FString::Printf(TEXT("Aircraft reporting: %d infantry, %d machine guns, %d tanks. Call artillery into the photographed sector."),Intel.Infantry,Intel.Machineguns,Intel.Armour);
}
}
void Reset(AWarfareFrontlineGameMode* B)
{
    auto& S=States.FindOrAdd(B);for(auto& A:S.Air)if(A.Aircraft.IsValid())
    {if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->UpdateAircraftAudio(int32(A.Aircraft->GetUniqueID()),A.Aircraft->GetActorLocation(),0,false);A.Aircraft->Destroy();}
    S=FState();S.Encounter=B->GetEncounterId();
    for(int32 T=0;T<2;++T){S.Ready[T][0]=FrontlineGameplayV43::PreparationSeconds(48+T*12,T, B->GetCampaignDifficulty());S.Ready[T][1]=FrontlineGameplayV43::PreparationSeconds(12+T*8,T, B->GetCampaignDifficulty());S.Ready[T][2]=FrontlineGameplayV43::PreparationSeconds(65+T*14,T, B->GetCampaignDifficulty());if(WarfareFRRUCampaignV82::Active(B))for(int32 I=0;I<3;++I)S.Ready[T][I]=WarfareFRRUEquipmentV82::SupportCooldown(B,EFrontlineSupport(int32(EFrontlineSupport::CreepingBarrage)+I),T);}
    for(auto It=States.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
}
float Cooldown(const AWarfareFrontlineGameMode* B,EFrontlineSupport Support,int32 Team)
{
    const int32 I=Index(Support);if(!B||I<0||I>2||Team<0||Team>1)return MAX_flt;
    const auto* S=States.Find(const_cast<AWarfareFrontlineGameMode*>(B));return S?FMath::Max(0.f,S->Ready[Team][I]-B->Elapsed):0;
}
bool GetReconStatusV37(const AWarfareFrontlineGameMode* B,int32 Team,FReconStatusV37& Status)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV37Review")))return false;
    if(!B||Team<0||Team>1||B->Phase!=EFrontlinePhase::Fighting)return false;const auto* S=States.Find(const_cast<AWarfareFrontlineGameMode*>(B));
    if(!S||S->Encounter!=B->GetEncounterId()||B->Elapsed>=S->ReconUntil[Team])return false;
    Status=S->Recon[Team];Status.SecondsRemaining=S->ReconUntil[Team]-B->Elapsed;return true;
}
bool HasRegistrationV37(const AWarfareFrontlineGameMode* B,int32 Team,const FVector& Point)
{
    return false; // V64 retired aerial registration; old save ordinal now calls shrapnel.
}
void ResetReadinessForDebug(AWarfareFrontlineGameMode* B,int32 Team)
{
    if(!B||Team<0||Team>1)return;auto* S=States.Find(B);if(!S||S->Encounter!=B->GetEncounterId())return;
    for(float& Ready:S->Ready[Team])Ready=B->Elapsed;
}
bool Request(AWarfareFrontlineGameMode* B,EFrontlineSupport Support,const FVector& Target,int32 Team)
{
    const int32 I=Index(Support);if(!B||I<0||I>2||Team<0||Team>1||Target.ContainsNaN()||FMath::Abs(Target.X)>FrontlineBattlefield::SupportHalfLength||FMath::Abs(Target.Y)>FrontlineBattlefield::HalfFrontage)return false;
    const FString Block=B->SupportBlockReason(Support,Team);if(!Block.IsEmpty()){if(Team==0)B->LastMessage=Block;return false;}
    if(Team==1&&!B->IsFireSupportAreaSafe(Target,Support,Team))return false;
    auto& S=States.FindOrAdd(B);const int32 Sequence=++S.Sequence[Team];
    FRandomStream R(FrontlineDifficultyV30::SupportSeed(B,Team,3201918+Sequence*137+Team*719));const int32 Dir=B->GetTeamDirection(Team);
    const auto Response=B->FireSupportDelayRange(Support,Target,Team);
    const bool Registered=HasRegistrationV37(B,Team,Target);const float Scatter=Registered?FrontlineFireSupportV26::AirRegistrationScatterV37:1.f;
    if(Support==EFrontlineSupport::CreepingBarrage)
    {
        const float First=R.FRandRange(Response.X,Response.Y);const bool Trained=B->HasTechnology(19,Team);
        for(int32 Salvo=0;Salvo<8;++Salvo)for(int32 Round=0;Round<3;++Round)
        {
            FFrontlineStrike Strike;Strike.SourceV65=FFrontlineKillSourceV65::FireSupport(Team,int32(Support));Strike.Team=Team;Strike.Radius=780;Strike.ArmorPower=420.f;
            Strike.Target=Bound(Target+FVector(Dir*Salvo*(Trained?330:420)+R.FRandRange(-220,220)*Scatter,((Round-1)*480.f+R.FRandRange(-120,120))*Scatter,0));
            Strike.ImpactAt=B->Elapsed+First+Salvo*5.2f+Round*.63f;Strike.LaunchedAt=Strike.ImpactAt-2.6f;Strike.Origin=Strike.Target+FVector(-Dir*1300,0,1900);
            Strike.FireMissionId=320000+Team*10000+Sequence;Strike.SalvoIndex=Salvo;Strike.RoundIndex=Round;Strike.WarningLeadSeconds=1.8;B->Strikes.Add(Strike);
        }
    }
    else if(Support==EFrontlineSupport::AirRecon)
    {
        // The legacy ordinal is retained solely for campaign/loadout compatibility.
        // Timed shrapnel is an explicit anti-infantry weapon, not an invisible buff.
        const float First=R.FRandRange(Response.X,Response.Y);
        const int32 Salvos=B->HasTechnology(20,Team)?3:2;
        for(int32 Salvo=0;Salvo<Salvos;++Salvo)for(int32 Round=0;Round<3;++Round)
        {
            FFrontlineStrike Strike;Strike.SourceV65=FFrontlineKillSourceV65::FireSupport(Team,int32(Support));Strike.Team=Team;Strike.bShrapnelV64=true;Strike.Radius=1200.f;
            Strike.Target=Bound(Target+FVector((Round-1)*720.f+R.FRandRange(-260,260),R.FRandRange(-600,600),0));
            Strike.Target.Z+=430.f+R.FRandRange(-50,65);
            Strike.ImpactAt=B->Elapsed+First+Salvo*3.7f+Round*.38f;
            Strike.LaunchedAt=Strike.ImpactAt-.72f;
            Strike.Origin=Strike.Target+FVector(-Dir*1850.f,0,2450.f);
            Strike.FireMissionId=340000+Team*10000+Sequence;Strike.SalvoIndex=Salvo;Strike.RoundIndex=Round;
            Strike.WarningLeadSeconds=.65f;B->Strikes.Add(Strike);
        }
    }
    else
    {
        auto* Mesh=FrontlineSupportArtV36::AircraftFor(B,Team);if(!Mesh)return false;
        // Keep twelve bombs at .55-second global intervals. Three aircraft each
        // carry four, allowing real cruise speed without stretching one plane's
        // ground coverage across the entire field or accelerating its release clock.
        TArray<FAirTask> Aircraft;
        for(int32 Plane=0;Plane<3;++Plane)
        {
            auto* Actor=B->GetWorld()->SpawnActor<AActor>();
            if(!Actor){for(auto& Made:Aircraft)if(Made.Aircraft.IsValid())Made.Aircraft->Destroy();return false;}
            auto* Component=NewObject<UStaticMeshComponent>(Actor);Actor->SetRootComponent(Component);Actor->AddInstanceComponent(Component);Component->SetStaticMesh(Mesh);Component->SetMobility(EComponentMobility::Movable);Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);Component->RegisterComponent();Actor->SetActorHiddenInGame(true);
            FAirTask A;A.Aircraft=Actor;A.Target=Target;A.Team=Team;A.Recon=false;
            A.FireMission=350000+Team*10000+Sequence;A.FirstRound=Plane*4;A.RoundCount=4;Aircraft.Add(A);
        }
        const float FirstEffect=R.FRandRange(Response.X,Response.Y);
        float Arrival=0.f;
        for(int32 Round=0;Round<12;++Round)
        {
            const float DropAge=1.1f+(Round%4)*.55f;FFrontlineStrike Strike;Strike.SourceV65=FFrontlineKillSourceV65::FireSupport(Team,int32(Support));Strike.Team=Team;Strike.Radius=820;Strike.ArmorPower=500.f;Strike.bAirDropped=true;
            const float Along=(DropAge-1.925f)*FrontlineSupportArtV8220::BomberSpeed;
            Strike.Target=Bound(Target+FVector((Along+R.FRandRange(-160,160)*Scatter)*Dir,R.FRandRange(-770,770)*Scatter,0));
            Strike.Origin=FrontlineSupportArtV8220::BomberFlight(Target,Dir,DropAge).Position;
            // Release from the visible aircraft without an upward launch impulse.
            const float Fall=FMath::Sqrt(FMath::Max(1.f,float(Strike.Origin.Z-Strike.Target.Z))*2.f/980.f);
            if(Round==0)Arrival=B->Elapsed+FirstEffect-1.1f-Fall;
            Strike.LaunchedAt=Arrival+1.1f+Round*.55f;
            Strike.ImpactAt=Strike.LaunchedAt+Fall;
            Strike.WarningLeadSeconds=1;Strike.RoundIndex=Round;Strike.FireMissionId=350000+Team*10000+Sequence;B->Strikes.Add(Strike);
        }
        for(auto& A:Aircraft){A.Arrival=Arrival+A.FirstRound*.55f;S.Air.Add(A);}
    }
    const float Base=B->SupportCooldownSeconds(Support);S.Ready[Team][I]=B->Elapsed+(WarfareFRRUCampaignV82::Active(B)?WarfareFRRUEquipmentV82::SupportCooldown(B,Support,Team):FrontlineGameplayV43::PreparationSeconds(Base*(B->HasTechnology(21,Team)&&B->Morale[Team]<35?.78f:1.f),Team,B->GetCampaignDifficulty()));
    FrontlineReportV839::SupportDelivered(B,Team,int32(Support));
    if(Team==0)B->LastMessage=FString::Printf(TEXT("%s · 联络已发出"),B->GetSupportName(Support,Team));return true;
}
void Tick(AWarfareFrontlineGameMode* B,float DT)
{
    auto& S=States.FindOrAdd(B);if(S.Encounter!=B->GetEncounterId()){Reset(B);return;}
    const bool Fighting=B->Phase==EFrontlinePhase::Fighting;
    const float VisualTime=B->Elapsed+(Fighting?0.f:B->BattleOutroElapsed);
    for(int32 I=S.Air.Num()-1;I>=0;--I)
    {
        auto& A=S.Air[I];const float Age=VisualTime-A.Arrival;auto* Plane=A.Aircraft.Get();if(!Plane){S.Air.RemoveAtSwap(I);continue;}
        if(Age<-3)continue;if(Age>12)
        {if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->UpdateAircraftAudio(int32(Plane->GetUniqueID()),Plane->GetActorLocation(),0,false);Plane->Destroy();S.Air.RemoveAtSwap(I);continue;}
        Plane->SetActorHiddenInGame(false);const int32 Dir=B->GetTeamDirection(A.Team);
        auto Flight=A.Recon?FrontlineSupportArtV40::TransitFlight(A.Target,Dir,Age):FrontlineSupportArtV8220::BomberFlight(A.Target,Dir,Age);
        if(Fighting&&A.DrivenOffAt<0&&FrontlineFieldSitesV59::EngageAircraft(B,A.Team,Flight.Position,Plane->GetUniqueID(),DT))
        {
            A.DrivenOffAt=B->Elapsed;
            // Released ordnance remains physical and lethal. Only bombs still
            // aboard this exact aircraft are cancelled, never another sortie.
            B->Strikes.RemoveAll([&](const FFrontlineStrike& Strike){return Strike.FireMissionId==A.FireMission&&Strike.RoundIndex>=A.FirstRound&&Strike.RoundIndex<A.FirstRound+A.RoundCount&&Strike.LaunchedAt>B->Elapsed;});
        }
        if(A.DrivenOffAt>=0){const float Escape=FMath::Min(2.f,B->Elapsed-A.DrivenOffAt);Flight.Position.Z+=Escape*450;Flight.Rotation.Roll+=Escape*14;}
        const FVector Position=Flight.Position;Plane->SetActorLocation(Position);Plane->SetActorRotation(Flight.Rotation);
        if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
            Audio->UpdateAircraftAudio(int32(Plane->GetUniqueID()),Position,1.f,B->CanResolveCommittedSupportV8217(),Flight.Velocity);
        if(!Fighting)continue; // Existing aircraft depart during the ceasefire; no new ordnance or observation.
        if(A.Recon&&A.DrivenOffAt<0)
        {
            if(Age>=2.6f&&!A.Reported){A.Reported=true;RecordRecon(B,S,A,Position);}
        }
    }
    // New support follows observed concentration and field conditions, on its
    // own assessment clock. Shrapnel attacks observed concentrations without reading player orders.
    if(Fighting&&!WarfareFRRUCampaignV82::Active(B)
        &&B->GetCampaignDifficulty()==EFrontlineDifficulty::Rookie&&B->Elapsed>=S.EnemyThink)
    {
        const auto& Doctrine=FrontlineDifficultyV30::Doctrine(B->GetCampaignDifficulty());
        auto& Tactics=FrontlineDifficultyV30::State(B);S.EnemyThink=B->Elapsed+Doctrine.ExtendedAssessment;
        if(B->Elapsed-Tactics.LastSupportAt<Doctrine.FireCoordinationGap)return;
        TArray<AWarfareFrontlineSoldier*> Seen;
        for(AWarfareFrontlineSoldier* U:B->Units)if(FrontlineDifficultyV30::CanObserve(B,U))Seen.Add(U);
        FrontlineDifficultyV30::Observe(B,Seen);
        FVector Target=FVector::ZeroVector;float Best=0;int32 Concentration=0;
        for(const auto& Candidate:Tactics.Contacts)
        {
            const float Score=FrontlineDifficultyV30::TargetScore(Candidate,Tactics.Contacts,B->Elapsed,Doctrine);
            if(Score<=Best)continue;Best=Score;Target=Candidate.Position;Concentration=0;
            for(const auto& Contact:Tactics.Contacts)if(FrontlineDifficultyV30::Confidence(Contact,B->Elapsed,Doctrine)>.35f
                &&FMath::Abs(Contact.Position.X-Target.X)<1400&&FMath::Abs(Contact.Position.Y-Target.Y)<650)++Concentration;
        }
        if(Concentration)
        {
            const auto Type=Concentration>=7?EFrontlineSupport::AirRecon:Concentration>=4?EFrontlineSupport::CreepingBarrage:EFrontlineSupport::AirBombing;
            if(B->SupportBlockReason(Type,1).IsEmpty()&&Request(B,Type,Target,1)){Tactics.LastSupportAt=B->Elapsed;++Tactics.PlannedFireMissions;}
        }
    }
}
void RunV37Checks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check)
{
    FString UserDir;const bool Isolated=B&&FParse::Param(FCommandLine::Get(),TEXT("WFV37Review"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
    Check(TEXT("support_review_uses_isolated_profile"),Isolated);if(!Isolated)return;
    bool Count=true,Commitment=true,Registration=true;
    for(bool Mortar:{false,true})for(int32 Seed=0;Seed<12;++Seed)
    {
        const auto Base=FrontlineFireSupportV26::Plan(Mortar,Seed),Again=FrontlineFireSupportV26::Plan(Mortar,Seed);
        const auto Air=FrontlineFireSupportV26::Plan(Mortar,Seed,false,false,false,true);
        Count&=Base.Num()==12&&Base.Last().Salvo==3&&Base.Last().Round==2;
        for(int32 I=0;I<Base.Num();++I)
        {
            Commitment&=Base[I].Offset.Equals(Again[I].Offset,.001)&&Base[I].ImpactDelay==Again[I].ImpactDelay
                &&Base[I].ImpactDelay>Base[I].TerminalFlight&&(!I||Base[I].ImpactDelay>Base[I-1].ImpactDelay);
            Registration&=(Base[I].Offset*.75).Equals(Air[I].Offset,.001)&&FMath::IsNearlyEqual(Base[I].ImpactDelay-Air[I].ImpactDelay,2.f,.001f);
        }
    }
    Check(TEXT("gun_and_mortar_orders_have_four_three_round_groups"),Count);
    Check(TEXT("committed_salvos_remain_repeatable_ordered_and_ballistic"),Commitment);
    Check(TEXT("aerial_registration_tightens_real_plans_and_first_response"),Registration);
    const FState Before=States.FindOrAdd(B);const auto OldUnits=B->Units;const auto OldSmoke=B->SmokeZones;
    const float OldTime=B->Elapsed;const auto OldPhase=B->Phase;const FString OldMessage=B->LastMessage;TArray<AActor*> Owned;
    ON_SCOPE_EXIT{for(auto* A:Owned)if(IsValid(A))A->Destroy();States.FindOrAdd(B)=Before;B->Units=OldUnits;B->SmokeZones=OldSmoke;B->Elapsed=OldTime;B->Phase=OldPhase;B->LastMessage=OldMessage;};
    auto& S=States.FindOrAdd(B);S=FState();S.Encounter=B->GetEncounterId();B->Units.Empty();B->SmokeZones.Empty();B->Elapsed=100;B->Phase=EFrontlinePhase::Fighting;
    const FVector Centre(900,0,AWarfareFrontlineGround::HeightAt(900,0));
    auto* Seen=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(Centre+FVector(0,0,88),FRotator::ZeroRotator);Owned.Add(Seen);
    const FVector Obscured=Centre+FVector(550,0,0);
    auto* Hidden=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(Obscured+FVector(0,0,88),FRotator::ZeroRotator);Owned.Add(Hidden);
    if(Seen){Seen->Battle=B;Seen->Team=1;Seen->Role=EFrontlineRole::Lewis;B->Units.Add(Seen);}
    if(Hidden){Hidden->Battle=B;Hidden->Team=1;B->Units.Add(Hidden);}
    FFrontlineSmokeZone Smoke;Smoke.Centre=Obscured;Smoke.Radius=220;Smoke.StartedAt=99;Smoke.ExpiresAt=200;B->SmokeZones.Add(Smoke);
    FAirTask Pass;Pass.Team=0;Pass.Target=Centre;RecordRecon(B,S,Pass,Centre+FVector(0,0,1550));
    FReconStatusV37 Intel;
    Check(TEXT("actual_air_pass_counts_visible_gun_crew_and_excludes_smoke"),Seen&&Hidden&&GetReconStatusV37(B,0,Intel)&&Intel.Infantry==1&&Intel.Machineguns==1);
    if(Seen)Seen->SetActorLocation(Centre+FVector(5000,0,88));
    Check(TEXT("air_report_keeps_last_observation_without_tracking_hidden_movement"),GetReconStatusV37(B,0,Intel)&&Intel.Infantry==1);
    S.Recon[1]=S.Recon[0];S.ReconUntil[1]=S.ReconUntil[0];
    Check(TEXT("both_armies_register_only_clear_points_inside_the_photographed_sector"),HasRegistrationV37(B,0,Centre)&&HasRegistrationV37(B,1,Centre)
        &&!HasRegistrationV37(B,0,Obscured)&&!HasRegistrationV37(B,0,Centre+FVector(0,1350,0))&&!HasRegistrationV37(B,0,Centre+FVector(3650,0,0)));
    const float Until=S.ReconUntil[0];B->Elapsed=Until+.01f;
    Check(TEXT("expired_air_report_cannot_authorize_registration"),!GetReconStatusV37(B,0,Intel)&&!HasRegistrationV37(B,0,Centre));B->Elapsed=100;
    S.Ready[0][0]=800;S.Ready[1][0]=900;const int32 AirCount=S.Air.Num();ResetReadinessForDebug(B,0);
    Check(TEXT("debug_readiness_preserves_enemy_clock_committed_air_tasks_and_recon"),S.Ready[0][0]==B->Elapsed&&S.Ready[1][0]==900&&S.Air.Num()==AirCount&&S.ReconUntil[0]==Until);
    const auto AT=B->FireSupportDelayRange(EFrontlineSupport::AntiTank,Centre),Gas=B->FireSupportDelayRange(EFrontlineSupport::Gas,Centre);
    Check(TEXT("antitank_response_is_fast_and_other_support_retains_a_real_delay"),AT.X==5&&AT.Y==7&&Gas.X>15&&Gas.Y<20);
    RunFrontlineSupportChecksV37(B,Check);
}
#include "WarfareFrontlineAirBombingChecksV8220.inl"

bool SetupReconReviewV37(AWarfareFrontlineGameMode* B)
{
    FString UserDir;if(!B||!FParse::Param(FCommandLine::Get(),TEXT("WFV37Review"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return false;
    ResetReadinessForDebug(B);const FVector Target(0,0,AWarfareFrontlineGround::HeightAt(0,0));
    if(!Request(B,EFrontlineSupport::AirRecon,Target,0))return false;
    auto& S=States.FindOrAdd(B);if(S.Air.IsEmpty())return false;
    S.Air.Last().Arrival=B->Elapsed-2.6f;S.EnemyThink=FMath::Max(S.EnemyThink,B->Elapsed+10.f);
    Tick(B,0);return HasRegistrationV37(B,0,Target);
}
}
