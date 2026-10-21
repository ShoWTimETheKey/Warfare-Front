#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineFireV33.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "WarfareFrontlineGameplayFocusedV84412.h"

bool FrontlineUseLegacyNo44V836();

namespace
{
struct FAPRound
{
    TWeakObjectPtr<AFrontlineTank> Tank;FVector Point;float Arrival=0,Roll=1,Chance=0;int32 Team=0;FFrontlineKillSourceV65 SourceV65;
    TWeakObjectPtr<AWarfareFrontlineSoldier> Infantry;
    FVector Direction=FVector::ZeroVector;
    FVector SurfaceNormalV837=FVector::UpVector;
};
struct FMission{FGuid Encounter;TArray<FAPRound> Rounds;FrontlineSpecialistsV41::FArmourRoundReadbackV833 Readback;};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FMission> Missions;
bool ReadbackAllowedV833(AWarfareFrontlineGameMode* B)
{
    static const bool Requested=FParse::Param(FCommandLine::Get(),TEXT("WFV833Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
    if(!Requested||!B||!B->IsProgressionVolatile()||WarfareFRRUCampaignV82::Active(B))return false;
    FString UserDir;return FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty()&&!FPaths::IsRelative(UserDir);
}
FMission& Mission(AWarfareFrontlineGameMode* B)
{
    for(auto It=Missions.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& M=Missions.FindOrAdd(B);if(M.Encounter!=B->GetEncounterId()){M.Encounter=B->GetEncounterId();M.Rounds.Reset();M.Readback={};}return M;
}
}
bool FrontlineSpecialistsV41::IsRifleGrenadier(const AWarfareFrontlineSoldier* U){return U&&U->Role==EFrontlineRole::RifleGrenadier&&U->MemberIndex==0;}
bool FrontlineSpecialistsV41::IsAntiTankGunner(const AWarfareFrontlineSoldier* U){return U&&U->Role==EFrontlineRole::AntiTankRifle&&U->MemberIndex==0;}
bool FrontlineSpecialistsV41::IsSpecialist(const AWarfareFrontlineSoldier* U){return IsRifleGrenadier(U)||IsAntiTankGunner(U);}
bool FrontlineSpecialistsV41::HasAntiTankRifleGrenadeV836(const AWarfareFrontlineSoldier* U)
{
    if(!IsRifleGrenadier(U)||!U->Battle||FrontlineUseLegacyNo44V836()
        ||WarfareFRRUCampaignV82::Active(U->Battle)||U->GetFaction()!=EFrontlineFaction::British)return false;
    // Late-campaign issue cutoff, not a claim about its historical invention.
    // Earlier sections retain their fragmentation rifle bomb, not an invented AP round.
    const auto* Mission=U->Battle->GetEncounterCampaignMission();
    return Mission&&Mission->CalendarDateV41>=19180723;
}
float FrontlineSpecialistsV41::RifleGrenadeScatterRadius(float RangeCm,float Suppression,bool Trained)
{
    return (90.f+FMath::Max(0.f,RangeCm)*.050f+FMath::Clamp(Suppression,0.f,1.f)*170.f)*(Trained?.85f:1.f)*1.20f;
}
FQuat FrontlineSpecialistsV41::RifleGrenadeRotation(const FVector& Velocity,bool FrenchVB)
{
    // The receiver VB nose is -Y; the retained No.23 nose is +Y.
    return Velocity.Rotation().Quaternion()*FRotator(0,FrenchVB?90:-90,0).Quaternion();
}
float FrontlineSpecialistsV41::ReloadSeconds(const AWarfareFrontlineSoldier* U)
{
    if(!IsSpecialist(U)||!U->Battle)return 0;
    float Seconds=IsAntiTankGunner(U)?(WarfareFRRUCampaignV82::Active(U->Battle.Get())?8.f:6.4f):7.f;
    if(U->Battle->HasTechnology(25,U->Team))if(const auto* Squad=U->Battle->FindSquad(U->SquadId))
        for(const auto& W:Squad->Members)if(auto* Other=W.Get();Other&&Other!=U&&Other->IsActiveCombatant()&&FVector::DistSquared2D(U->Feet(),Other->Feet())<FMath::Square(650.f)){Seconds*=.85f;break;}
    return Seconds;
}
bool FrontlineSpecialistsV41::Fire(AWarfareFrontlineSoldier* U,AWarfareFrontlineSoldier* Target,AFrontlineTank* Tank,FRandomStream& Random)
{
    auto* B=U?U->Battle.Get():nullptr;
    if(!B||!IsSpecialist(U)||!U->IsActiveCombatant()||U->bMoving||B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)return false;
    const FVector From=U->GetMuzzleLocation();
    if(IsRifleGrenadier(U))
    {
        const bool Infantry=Target&&Target->IsActiveCombatant()&&Target->Team!=U->Team;
        const bool Armour=Tank&&Tank->IsOperational()&&Tank->HasEnteredField()&&Tank->Team!=U->Team;
        if(!Infantry&&!Armour)return false;
        const bool AntiArmour=Armour&&!Infantry&&HasAntiTankRifleGrenadeV836(U);
        const FVector Aim=Infantry?Target->Feet():AntiArmour?Tank->ObservedArmourAimPointV837(U->Feet()):Tank->GrenadeAimPointFor(U->Feet());
        const float Range=FVector::Dist2D(U->Feet(),Aim);
        const float RangeLimit=AntiArmour?AntiTankRifleGrenadeRangeCmV836:FrontlineWeapons::ProfileFor(U).FireRangeCm;
        if(Range>RangeLimit*B->EncounterVisibilityScale())return false;
        const bool National=WarfareFRRUCampaignV82::Active(B);
        auto* Shape=National?WarfareFRRUPresentationV82::RifleGrenadeMesh(U)
            :LoadObject<UStaticMesh>(nullptr,AntiArmour?TEXT("/Game/Warfare/Characters/V836/Weapons/SM_No44_RifleGrenade_V836.SM_No44_RifleGrenade_V836")
                :TEXT("/Game/Warfare/Characters/V41/Weapons/SM_No23_RifleGrenade_V41.SM_No23_RifleGrenade_V41"));
        if(!Shape)return false;
        // The heavy 1918 charge is still a scattered, committed rifle bomb.
        // Neither its observed target nor its release point follows a moving hull.
        const float Spread=RifleGrenadeScatterRadius(Range,U->Suppression,B->HasTechnology(24,U->Team))
            *(Armour&&!Infantry&&!National?.82f:1.f);
        const float A=Random.FRandRange(0,2*PI),R=FMath::Sqrt(Random.FRand())*Spread;
        FFrontlineStrike S;S.SourceV65=FFrontlineKillSourceV65::Soldier(U);S.Team=U->Team;S.Origin=From;S.Target=Aim+FVector(FMath::Cos(A)*R,FMath::Sin(A)*R*.85f,0);
        if(National)
        {float Height=0;if(!WarfareFRRUWorldV82::TryHeight(B,S.Target.X,S.Target.Y,Height))return false;S.Target.Z=Height+3;}
        else S.Target.Z=AntiArmour?FMath::Max(Aim.Z,double(FrontlineInfantryV45::SurfaceHeight(B,S.Target)+3.f))
            :AWarfareFrontlineGround::HeightAt(S.Target.X,S.Target.Y)+3;
        S.LaunchedAt=B->Elapsed;S.ImpactAt=B->Elapsed+FMath::Clamp(.65f+Range/2100.f,1.05f,2.1f);S.bGrenade=true;S.bRifleLaunched=true;S.Radius=FrontlineGrenade::BlastRadiusCm;
        S.bFrenchVBRound=National;S.bAntiTankRifleGrenadeV836=AntiArmour;
        auto* Visual=B->GetWorld()->SpawnActor<AActor>(From,FRotator::ZeroRotator);if(!Visual)return false;
        auto* Mesh=NewObject<UStaticMeshComponent>(Visual);Visual->SetRootComponent(Mesh);Visual->AddInstanceComponent(Mesh);Mesh->SetStaticMesh(Shape);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->RegisterComponent();Visual->SetActorLocation(From);S.Projectile=Visual;
        const float Flight=S.ImpactAt-S.LaunchedAt;Visual->SetActorRotation(RifleGrenadeRotation((S.Target-S.Origin)/Flight+FVector(0,0,490.f*Flight),S.bFrenchVBRound));
        B->Strikes.Add(S);++B->GrenadesThrown;++U->GrenadesReleased;
    }
    else if(Tank)
    {
        if(!Tank->IsOperational()||!Tank->HasEnteredField()||Tank->Team==U->Team)return false;
        FVector Aim=Tank->ObservedArmourAimPointV837(U->Feet());const float Range=FVector::Dist(From,Aim);
        if(Range>FrontlineWeapons::ProfileFor(U).FireRangeCm*B->EncounterVisibilityScale())return false;
        // The old low aim was grounded against the mud under the track, rather
        // than transformed with the actual hull. On a slope it could spend a
        // 13 mm round in soil before reaching any plate or running gear.
        const bool Host=!WarfareFRRUCampaignV82::Active(B);
        if(Random.FRand()<(Host?.20f:.40f))
        {
            if(Host)
            {
                FVector Local=Tank->GetActorTransform().InverseTransformPosition(Tank->GrenadeAimPointFor(U->Feet()));
                Local.Z=95.f;Aim=Tank->GetActorTransform().TransformPosition(Local);
            }
            else Aim=Tank->GrenadeAimPointFor(U->Feet())+FVector(0,0,45);
        }
        const float Spread=(22.f+Range*.012f+U->Suppression*85.f)*(B->HasTechnology(24,U->Team)?.85f:1.f);
        Aim+=FVector(Random.FRandRange(-Spread,Spread),Random.FRandRange(-Spread,Spread),Random.FRandRange(-Spread*.65f,Spread*.65f));
        const FVector Direction=(Aim-From).GetSafeNormal();FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(V41Tankgewehr),true);
        for(const AWarfareFrontlineSoldier* Other:B->Units)Query.AddIgnoredActor(Other);
        FVector End=Aim+Direction*550;
        const bool HitSurface=B->GetWorld()->LineTraceSingleByChannel(Hit,From,End,ECC_Visibility,Query);if(HitSurface)End=Hit.ImpactPoint;
        if(ReadbackAllowedV833(B))
        {
            auto& Stats=Mission(B).Readback;++Stats.Fired;
            if(auto* Surface=HitSurface?Cast<AFrontlineTank>(Hit.GetActor()):nullptr;Surface&&Surface->Team!=U->Team)++Stats.PhysicalHullHits;
            else if(HitSurface)++Stats.WorldInterceptions;
        }
        const float Flight=FMath::Clamp(float(FVector::Dist(From,End))/78000.f,.012f,.12f);
        const float Obliquity=HitSurface?FMath::Clamp(float(FMath::Abs(FVector::DotProduct(Direction,Hit.ImpactNormal))),.20f,1.f):0;
        const float Penetration=Host?.88f-Range*.000040f:.58f-Range*.000045f;
        const float Chance=FMath::Clamp(Penetration*(B->HasTechnology(24,U->Team)?1.15f:1.f)*Obliquity*(1-U->Suppression*.55f),.04f,.65f);
        Mission(B).Rounds.Add({HitSurface?Cast<AFrontlineTank>(Hit.GetActor()):nullptr,End,B->Elapsed+Flight,Random.FRand(),Chance,U->Team,FFrontlineKillSourceV65::Soldier(U)});
        Mission(B).Rounds.Last().Direction=Direction;
        Mission(B).Rounds.Last().SurfaceNormalV837=HitSurface?Hit.ImpactNormal:-Direction;
        if(auto* Round=B->GetWorld()->SpawnActor<AFrontlineOrdnanceV36>())
        {Round->SetGroundImpactEnabled(false);Round->SetHeavyBulletVisualV8216();if(!Round->Configure(B,From,End,B->Elapsed,B->Elapsed+Flight,true,true))Round->Destroy();}
    }
    else
    {
        if(!Target||!Target->IsActiveCombatant()||Target->Team==U->Team)return false;
        // The 13.2 mm round is a committed high-energy direct shot, with the
        // same hit probability and a short real flight. It does not borrow a
        // service-rifle discharge or create a delayed miniature explosion.
        const float Range=FVector::Dist2D(U->Feet(),Target->Feet());
        if(Range>FrontlineWeapons::ProfileFor(U).FireRangeCm*B->EncounterVisibilityScale())return false;
        FVector Aim=Target->Soldier->GetBoneLocation(TEXT("spine_03"));
        FCollisionQueryParams Query(SCENE_QUERY_STAT(V75HeavyRifleInfantry),true);
        for(const AWarfareFrontlineSoldier* Other:B->Units)if(Other)Query.AddIgnoredActor(Other);
        if(B->GetWorld()->LineTraceTestByChannel(From,Aim,ECC_Visibility,Query))
        {
            Aim=Target->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5);
            if(B->GetWorld()->LineTraceTestByChannel(From,Aim,ECC_Visibility,Query))return false;
        }
        const float Flight=FMath::Clamp(float(FVector::Dist(From,Aim))/78000.f,.012f,.12f);
        FAPRound Round;Round.Point=Aim;Round.Arrival=B->Elapsed+Flight;Round.Roll=Random.FRand();
        Round.Chance=B->BulletHitChance(U,Target);Round.Team=U->Team;Round.SourceV65=FFrontlineKillSourceV65::Soldier(U);
        Round.Infantry=Target;Round.Direction=(Aim-From).GetSafeNormal();Mission(B).Rounds.Add(Round);
        if(auto* Visual=B->GetWorld()->SpawnActor<AFrontlineOrdnanceV36>())
        {Visual->SetGroundImpactEnabled(false);Visual->SetHeavyBulletVisualV8216();if(!Visual->Configure(B,From,Aim,B->Elapsed,B->Elapsed+Flight,true,true))Visual->Destroy();}
    }
    U->ShowWeaponDischarge();++B->ShotsFired;
    if(auto* A=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
    {
        if(IsAntiTankGunner(U))A->PlayHeavyWeaponV8215(U->GetFaction()==EFrontlineFaction::German,From,U->SquadId);
        else A->PlayWeapon(false,U->GetFaction()==EFrontlineFaction::German,From,U->SquadId);
    }
    return true;
}
void FrontlineSpecialistsV41::Tick(AWarfareFrontlineGameMode* B)
{
    if(!B)return;auto& M=Mission(B);
    if(B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE){M.Rounds.Reset();return;}
    for(int32 I=M.Rounds.Num()-1;I>=0;--I)
    {
        const auto R=M.Rounds[I];if(B->Elapsed<R.Arrival)continue;
        // Arrival owns a compact direct-hit effect: hot metal flecks for an
        // armour surface, or a directional dirt punch for a missed infantry
        // shot. The casualty's own short exit spray stays separate from HE.
        FVector ImpactPoint=R.Point;
        if(auto* Infantry=R.Infantry.Get();Infantry&&R.Roll>=R.Chance)
            ImpactPoint.Z=AWarfareFrontlineGround::HeightAt(ImpactPoint.X,ImpactPoint.Y)+4.f;
        if(auto* Impact=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(ImpactPoint,FRotator::ZeroRotator))
            Impact->ConfigureHeavyBulletV8215(R.Direction,R.Tank.IsValid());
        if(R.Roll<R.Chance)if(auto* V=R.Tank.Get();V&&V->IsOperational()&&V->Team!=R.Team)
        {
            const FVector Local=V->GetActorTransform().InverseTransformPosition(R.Point);
            // The observed impact stays in world space. Movement can carry a
            // hull clear before arrival; neither the shot nor damage follows it.
            if(V->ContainsHullPointV75(R.Point))
            {
                const float HullBefore=V->GetHullIntegrity(),TracksBefore=V->GetTrackIntegrity(0)+V->GetTrackIntegrity(1);
                const bool Host=!WarfareFRRUCampaignV82::Active(B);
                if(Local.Z<105&&FMath::Abs(Local.Y)>95)V->ReceiveModuleHit(Local.Y<0?EFrontlineTankModule::LeftTrack:EFrontlineTankModule::RightTrack,Host?90.f:75.f,0,R.SourceV65);
                else V->ReceiveModuleHit(EFrontlineTankModule::Hull,Host?82.f:64.f,0,R.SourceV65);
                if(Host)V->ReceiveRailContactV837(R.Point,82.f,R.SourceV65);
                const float HullDamage=HullBefore-V->GetHullIntegrity(),TrackDamage=TracksBefore-V->GetTrackIntegrity(0)-V->GetTrackIntegrity(1);
                if(Host&&HullDamage+TrackDamage>0)if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld()))
                    Fire->EmitArmourContactV836(R.Point,R.SurfaceNormalV837,FMath::Clamp((HullDamage+TrackDamage)/90.f,.2f,1.f),false);
                if(ReadbackAllowedV833(B)&&HullDamage+TrackDamage>0){++M.Readback.AppliedHits;M.Readback.HullDamage+=HullDamage;M.Readback.TrackDamage+=TrackDamage;}
            }
        }
        if(auto* U=R.Infantry.Get();U&&U->IsActiveCombatant()&&U->Team!=R.Team)
        {
            const bool Intercepted=FVector::Dist2D(U->Feet(),R.Point)<82.f;
            U->ReceiveHit(R.Roll<R.Chance&&Intercepted?1.f:0.f,.32f,R.Team,R.Direction,
                EFrontlineDamageKind::HeavyBullet,1.1f,R.SourceV65);
            if(U->IsAlive())U->ShowBulletImpact(FVector(R.Point.X,R.Point.Y,AWarfareFrontlineGround::HeightAt(R.Point.X,R.Point.Y)+4.f));
        }
        M.Rounds.RemoveAtSwap(I);
    }
}
void FrontlineSpecialistsV41::Reset(AWarfareFrontlineGameMode* B){Missions.Remove(B);}
FrontlineSpecialistsV41::FArmourRoundReadbackV833 FrontlineSpecialistsV41::ReadArmourRoundsV833(AWarfareFrontlineGameMode* B)
{
    if(!ReadbackAllowedV833(B))return {};
    return Mission(B).Readback;
}
void FrontlineSpecialistsV41::NoteGrenadeArmourV833(AWarfareFrontlineGameMode* B,const FFrontlineKillSourceV65& Source,int32 VictimTeam,float HullDamage,float TrackDamage)
{
    if(!ReadbackAllowedV833(B)||Source.Team==VictimTeam||Source.SquadId==INDEX_NONE||Source.Role==INDEX_NONE
        ||Source.VehicleSerial!=INDEX_NONE||Source.Support!=INDEX_NONE||HullDamage+TrackDamage<=0)return;
    auto& Stats=Mission(B).Readback;++Stats.GrenadeAppliedHits;Stats.GrenadeHullDamage+=HullDamage;Stats.GrenadeTrackDamage+=TrackDamage;
}

struct FFrontlineSpecialistsV41Review
{
#if WITH_EDITOR
    static TSharedRef<FJsonObject> RunFrenchVBMechanicsV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(AWarfareFrontlineSoldier*)> Observe)
    {
        TSharedPtr<FJsonObject> Report=MakeShared<FJsonObject>();bool Passed=true;TArray<TSharedPtr<FJsonValue>> Checks;
        const auto Check=[&](const TCHAR* Name,bool Good){Passed&=Good;auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Good);Checks.Add(MakeShared<FJsonValueObject>(Row));Report->SetArrayField(TEXT("checks"),Checks);Report->SetBoolField(TEXT("allPassed"),Passed);};
        Report->SetBoolField(TEXT("actorsPlaced"),false);Report->SetBoolField(TEXT("actorsTeleported"),false);Report->SetBoolField(TEXT("simulationClockFixture"),false);
        Report->SetBoolField(TEXT("actualDamageResolution"),false);Report->SetBoolField(TEXT("flightStoppedBeforeImpact"),false);
        Report->SetBoolField(TEXT("explicitDischargeInvocation"),true);Report->SetBoolField(TEXT("renderedLiveCycleAccepted"),false);
        FString User,Case;FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
        const bool Flags=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("FRRUVBMechanicsFixture"))
            &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82Review"))
            &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82NaturalReview"))
            &&!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly"))&&!FParse::Param(FCommandLine::Get(),TEXT("FRRUBlockedFireProbe"))
            &&FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalCase="),Case)&&Case==TEXT("chemin-vb")
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!FPaths::IsRelative(User)
            &&Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase);
        FPaths::NormalizeDirectoryName(User);const FString Prefix=Project+TEXT("/Saved/FRRU/V82NaturalReview/chemin-vb/");const FString Suffix=User.Mid(Prefix.Len());FGuid Run;
        Check(TEXT("strict_french_vb_disposable_guid_user"),Flags&&User.StartsWith(Prefix,ESearchCase::IgnoreCase)&&Suffix.Len()==37
            &&Suffix.Mid(32).Equals(TEXT("/User"),ESearchCase::IgnoreCase)&&FGuid::ParseExact(Suffix.Left(32),EGuidFormats::Digits,Run)&&Run.IsValid()
            &&WarfareFRRUCampaignV82::MissionId(B)==TEXT("fr.chemin_dames")&&WarfareFRRUCampaignV82::Nation(B,0)==EFRRUNationV81::French);
        if(!Passed)return Report.ToSharedRef();
        const auto Units=B->Units;const auto Squads=B->Squads;const auto Strikes=B->Strikes;const auto Tracers=B->Tracers;const auto Smoke=B->SmokeZones;
        const float Clock=B->Elapsed;const auto Phase=B->Phase;const int32 Outro=B->BattleOutroWinner,Shots=B->ShotsFired,Grenades=B->GrenadesThrown,NextId=B->NextSquadId;
        const auto Random=B->Random;const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];const FString Message=B->LastMessage,AssetFailure=B->AssetFailure;
        TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
        ON_SCOPE_EXIT
        {
            for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
            B->Units=Units;B->Squads=Squads;B->Strikes=Strikes;B->Tracers=Tracers;B->SmokeZones=Smoke;B->Elapsed=Clock;B->Phase=Phase;B->BattleOutroWinner=Outro;
            B->ShotsFired=Shots;B->GrenadesThrown=Grenades;B->NextSquadId=NextId;B->Random=Random;B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->LastMessage=Message;B->AssetFailure=AssetFailure;
            Check(TEXT("fixture_arrays_clock_and_counters_restored"),B->Units.Num()==Units.Num()&&B->Squads.Num()==Squads.Num()&&B->Strikes.Num()==Strikes.Num()&&B->Elapsed==Clock&&B->ShotsFired==Shots&&B->GrenadesThrown==Grenades);
        };
        B->Units.Reset();B->Squads.Reset();B->Strikes.Reset();B->Tracers.Reset();B->SmokeZones.Reset();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
        const auto Make=[&](int32 Side,int32 Member,int32 Id,EFrontlineRole Role,const FVector& Feet)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;U->Initialize(B,Id,Side,Role,Member);if(!IsValid(U))return static_cast<AWarfareFrontlineSoldier*>(nullptr);U->SetActorTickEnabled(false);
            U->SetActorLocation(Feet+FVector(0,0,88));U->SetActorRotation(FRotator(0,Side==0?0:180,0));U->bFieldEntered=true;U->bMoving=false;B->Units.Add(U);
            Report->SetBoolField(TEXT("actorsPlaced"),true);Report->SetBoolField(TEXT("actorsTeleported"),true);return U;
        };
        auto* Operator=Make(0,0,941871,EFrontlineRole::RifleGrenadier,FVector(-1800,0,2200));
        auto* Escort1=Make(0,1,941871,EFrontlineRole::RifleGrenadier,FVector(-1850,-100,2200));
        auto* Escort2=Make(0,2,941871,EFrontlineRole::RifleGrenadier,FVector(-1850,100,2200));
        auto* Target=Make(1,0,941872,EFrontlineRole::Rifle,FVector(100,0,2200));
        Check(TEXT("actual_french_operator_two_escorts_and_target"),Operator&&Escort1&&Escort2&&Target);if(!Passed)return Report.ToSharedRef();
        FFrontlineSquad Crew;Crew.Id=941871;Crew.Team=0;Crew.Role=EFrontlineRole::RifleGrenadier;Crew.Members={Operator,Escort1,Escort2};B->Squads.Add(Crew);
        FFrontlineSquad Enemy;Enemy.Id=941872;Enemy.Team=1;Enemy.Role=EFrontlineRole::Rifle;Enemy.Members={Target};B->Squads.Add(Enemy);
        const auto* Equipment=WarfareFRRUEquipmentV82::Resolve(Operator);
        Check(TEXT("actual_national_vb_one_round_4_2s_catalog"),Equipment&&Equipment->WeaponId==TEXT("fr.vb")&&Equipment->Capacity==1&&FMath::IsNearlyEqual(Equipment->ReloadSeconds,4.2f,.001f)&&Operator->RoundsRemaining==1
            &&B->AssetFailure.IsEmpty()&&WarfareFRRUPresentationV82::HasBoundWeapon(Operator));
        if(!Passed)return Report.ToSharedRef();
        const auto EvaluatePresentation=[&](AWarfareFrontlineSoldier* U,float DT)
        {
            U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();WarfareFRRUPresentationV82::TickUnit(U,DT);
            TArray<USkeletalMeshComponent*> Parts;U->GetComponents(Parts);
            for(auto* Part:Parts)if(Part&&Part->GetFName()==TEXT("NationalReloadPairV82")){Part->TickAnimation(0,false);Part->RefreshBoneTransforms();}
        };
        for(auto* U:{Operator,Escort1,Escort2,Target}){U->ReturnToIdle();U->SetMotion(U->GetAimMotion());EvaluatePresentation(U,.35f);}
        Report->SetStringField(TEXT("placementScope"),TEXT("Explicit elevated 2200cm actor fixture; no native route/arrival or combat acceptance."));
        Report->SetStringField(TEXT("operatorFeet"),Operator->Feet().ToString());Report->SetStringField(TEXT("targetFeet"),Target->Feet().ToString());
        Check(TEXT("actual_observed_target_and_muzzle_clear"),B->HasSight(Operator,Target)&&WarfareFRRUWorldV82::FiringLine(Operator,Target));
        if(!Passed)return Report.ToSharedRef();Observe(Operator);
        const float TargetHealth=Target->Health;const FVector Muzzle=Operator->GetMuzzleLocation();Operator->DischargeWeapon(Target);
        Check(TEXT("actual_discharge_commits_one_french_vb_round"),Operator->ShotsDischarged==1&&Operator->RoundsRemaining==0&&B->Strikes.Num()==1&&B->Strikes[0].bFrenchVBRound&&B->Strikes[0].bRifleLaunched&&B->Strikes[0].Origin.Equals(Muzzle,.1));
        if(!Passed)return Report.ToSharedRef();Observe(Operator);
        const float Impact=B->Strikes[0].ImpactAt;float ReloadStarted=-1;constexpr float Step=1.f/60.f;
        for(int32 Frame=1;Frame<=720&&Operator->ReloadsCompleted==0;++Frame)
        {
            B->Elapsed=Clock+Frame*Step;Report->SetBoolField(TEXT("simulationClockFixture"),true);
            if(!B->Strikes.IsEmpty())
            {
                if(B->Elapsed<Impact)B->TickCommittedSupportV8217();
                else{for(auto& S:B->Strikes)if(S.Projectile.IsValid())S.Projectile->Destroy();B->Strikes.Reset();Report->SetBoolField(TEXT("flightStoppedBeforeImpact"),true);}
            }
            Operator->UpdateWeaponCycle(Step);if(Operator->WeaponPhase==EFrontlineWeaponPhase::Reload&&ReloadStarted<0)ReloadStarted=B->Elapsed;
            EvaluatePresentation(Operator,Step);
            if(Frame%3==0||Operator->ReloadsCompleted>0)Observe(Operator);
        }
        Report->SetNumberField(TEXT("controlledClockStepSeconds"),Step);Report->SetNumberField(TEXT("controlledHostSeconds"),B->Elapsed-Clock);
        Report->SetNumberField(TEXT("actualReloadSeconds"),ReloadStarted<0?-1:B->Elapsed-ReloadStarted);
        Check(TEXT("same_operator_restores_one_round_after_real_catalog_cycle"),Operator->ShotsDischarged==1&&Operator->ReloadsStarted==1&&Operator->ReloadsCompleted==1&&Operator->RoundsRemaining==1
            &&ReloadStarted>=0&&FMath::Abs(B->Elapsed-ReloadStarted-4.2f)<=Step+.005f);
        Check(TEXT("target_health_unchanged_no_strike_resolution"),Target->Health==TargetHealth&&B->Strikes.IsEmpty());
        return Report.ToSharedRef();
    }
#endif
    static void RunV75(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        FString UserDir;if(!B||!B->IsProgressionVolatile()||!(FParse::Param(FCommandLine::Get(),TEXT("WFV75GameplayReview"))||(FParse::Param(FCommandLine::Get(),TEXT("WFV8215Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8216Review"))))
            ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
        {Check(TEXT("heavy_rifle_requires_disposable_review_profile"),false);return;}
        if((FParse::Param(FCommandLine::Get(),TEXT("WFV8215Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8216Review")))
            &&!FrontlineEffectsReviewSafetyV8216::Allowed(B)){Check(TEXT("heavy_rifle_requires_exact_disposable_profile_allowlist"),false);return;}
        const auto Units=B->Units;const auto Squads=B->Squads;const float Clock=B->Elapsed;const auto Saved=Mission(B);
        TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
        B->Units.Reset();B->Squads.Reset();Mission(B).Rounds.Reset();
        ON_SCOPE_EXIT{for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();B->Units=Units;B->Squads=Squads;B->Elapsed=Clock;Mission(B)=Saved;};
        const int32 German=B->GetFactionForTeam(0)==EFrontlineFaction::German?0:1,British=1-German,Dir=B->GetTeamDirection(German);
        auto Make=[&](EFrontlineRole Role,int32 Team,int32 Member,int32 Id,FVector P)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            U->Initialize(B,Id,Team,Role,Member);U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));
            U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->WeaponPhase=EFrontlineWeaponPhase::Ready;B->Units.Add(U);return U;
        };
        auto* AT=Make(EFrontlineRole::AntiTankRifle,German,0,750501,FVector(-Dir*2000,1100,2200));
        auto* Bearer=Make(EFrontlineRole::AntiTankRifle,German,1,750501,FVector(-Dir*2050,1200,2200));
        auto* Target=Make(EFrontlineRole::Rifle,British,0,750502,FVector(Dir*100,1100,2200));
        if(!AT||!Bearer||!Target){Check(TEXT("heavy_rifle_live_actors_exist"),false);return;}
        FFrontlineSquad Crew;Crew.Id=750501;Crew.Team=German;Crew.Role=EFrontlineRole::AntiTankRifle;Crew.Members={AT,Bearer};B->Squads.Add(Crew);
        auto* G=FrontlineMachinegunsV30::Get(AT);if(!G){Check(TEXT("heavy_rifle_real_supported_component_exists"),false);return;}
        for(int32 I=0;I<80&&!G->IsSupported();++I)G->Update(.05f,false);
        AT->ReturnToIdle();AT->SetMotion(AT->GetAimMotion());AT->Soldier->TickAnimation(.35f,false);AT->Soldier->RefreshBoneTransforms();
        Check(TEXT("heavy_rifle_has_deployed_supported_pose"),G->CanFire());
        FRandomStream Random(754101);const float Health=Target->Health;
        const bool Shot=FrontlineSpecialistsV41::Fire(AT,Target,nullptr,Random);
        Check(TEXT("13mm_rifle_commits_deferred_direct_infantry_shot"),Shot&&Mission(B).Rounds.Num()==1&&Target->Health==Health&&Mission(B).Rounds[0].Arrival>B->Elapsed);
        if(Shot&&!Mission(B).Rounds.IsEmpty())
        {
            auto& R=Mission(B).Rounds[0];R.Roll=0;R.Chance=1;B->Elapsed=R.Arrival+.001f;FrontlineSpecialistsV41::Tick(B);
            Check(TEXT("13mm_actual_infantry_hit_uses_heavy_bullet_fatal_handoff"),!Target->IsAlive()&&Target->GetDeathState().Kind==EFrontlineDamageKind::HeavyBullet&&Mission(B).Rounds.IsEmpty());
        }
        auto* Tank=B->GetWorld()->SpawnActor<AFrontlineTank>();const bool Ready=Tank&&Tank->Configure(B,EFrontlineVehicle::MarkIVMale,British,750503);
        Check(TEXT("AP_target_uses_actual_collision_hull"),Ready);if(!Ready)return;
        Tank->SetActorLocation(FVector(0,1100,0));Tank->Simulate(.001f);Tank->SetActorLocation(FVector(0,1100,2200));Tank->SetActorRotation(FRotator::ZeroRotator);
        bool Hit=false;FAPRound Captured;
        for(int32 I=0;I<12&&!Hit;++I)
        {
            FRandomStream Roll(750703+I*37);if(!FrontlineSpecialistsV41::Fire(AT,nullptr,Tank,Roll))continue;
            if(!Mission(B).Rounds.IsEmpty()&&Mission(B).Rounds.Last().Tank.Get()==Tank){Captured=Mission(B).Rounds.Last();Hit=true;}
        }
        Check(TEXT("AP_trace_physically_intersects_real_armour_with_crew_ignored"),Hit&&Tank->ContainsHullPointV75(Captured.Point));
        if(Hit)
        {
            const float Before=Tank->GetHullIntegrity()+Tank->GetTrackIntegrity(0)+Tank->GetTrackIntegrity(1);
            Captured.Roll=0;Mission(B).Rounds={Captured};B->Elapsed=Captured.Arrival+.001f;FrontlineSpecialistsV41::Tick(B);
            const float After=Tank->GetHullIntegrity()+Tank->GetTrackIntegrity(0)+Tank->GetTrackIntegrity(1);
            Check(TEXT("AP_committed_strike_resolves_82_hull_or_90_track_damage"),FMath::IsNearlyEqual(Before-After,82.f,.02f)||FMath::IsNearlyEqual(Before-After,90.f,.02f));
            Captured.Arrival=B->Elapsed+.03f;Mission(B).Rounds={Captured};Tank->AddActorWorldOffset(FVector(0,1400,0));B->Elapsed+=.04f;FrontlineSpecialistsV41::Tick(B);
            Check(TEXT("AP_does_not_follow_armour_that_moves_clear"),FMath::IsNearlyEqual(After,Tank->GetHullIntegrity()+Tank->GetTrackIntegrity(0)+Tank->GetTrackIntegrity(1),.01f));
        }
        auto* Jaeger=Make(EFrontlineRole::Marksman,German,0,750505,FVector(-Dir*1200,-300,2200));
        auto* UK=Make(EFrontlineRole::Marksman,British,0,750506,FVector(-Dir*1200,-300,2200));
        Check(TEXT("German_veteran_Jaeger_patrol_is_three_riflemen_with_distinct_contact_range"),Jaeger&&UK&&B->GetSquadSize(EFrontlineRole::Marksman,German)==3
            &&FrontlineWeapons::ProfileFor(Jaeger).AdvanceContactCm<FrontlineWeapons::ProfileFor(UK).AdvanceContactCm);
        auto* Service=Make(EFrontlineRole::Rifle,German,0,750507,FVector(-Dir*1200,-300,2200));
        bool VeteranProfiles=Jaeger&&Service;
        for(int32 I=0;I<3;++I)
        {
            auto* Veteran=I==0?Jaeger:Make(EFrontlineRole::Marksman,German,I,750505,FVector(-Dir*1200,-300-I*90,2200));
            if(!Veteran||!Service){VeteranProfiles=false;continue;}
            const auto& P=FrontlineWeapons::ProfileFor(Veteran);const auto& R=FrontlineWeapons::ProfileFor(Service);
            VeteranProfiles&=P.HitProbabilityScale>R.HitProbabilityScale&&P.AimedCycleSeconds<R.AimedCycleSeconds
                &&P.FireRangeCm==5500.f&&P.AdvanceContactCm==3900.f;
        }
        Check(TEXT("v8215_all_three_actual_Jaeger_members_keep_veteran_accuracy_and_cycle"),VeteranProfiles);
    }
    #include "WarfareFrontlineATProjectileChecksV84412.inl"
    static AWarfareFrontlineSoldier* Visual(AWarfareFrontlineGameMode* B,EFrontlineRole Role)
    {
        FString UserDir;if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return nullptr;
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Empty();B->Squads.Empty();
        if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))V->ResetBattle();
        B->Elapsed=5;B->BattleOutroWinner=INDEX_NONE;B->Phase=EFrontlinePhase::Fighting;
        const int32 Dir=B->GetTeamDirection(0);const FVector P(-350*Dir,100,AWarfareFrontlineGround::HeightAt(-350*Dir,100));
        auto Make=[&](int32 Team,int32 Member,EFrontlineRole InRole,const FVector& Where)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            U->Initialize(B,941801+Team,Team,InRole,Member);U->SetActorTickEnabled(false);U->SetActorLocation(Where+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));
            U->bMoving=false;U->Suppression=0;U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->RoundsRemaining=U->GetMagazineCapacity();U->MotionRandom.Initialize(419731);B->Units.Add(U);return U;
        };
        auto* U=Make(0,0,Role,P);auto* Bearer=Make(0,1,Role,P+FVector(-Dir*40,-110,0));if(!U||!Bearer)return U;
        FFrontlineSquad Crew;Crew.Id=941801;Crew.Team=0;Crew.Role=Role;Crew.Members={U,Bearer};B->Squads.Add(Crew);
        AWarfareFrontlineSoldier* Target=nullptr;AFrontlineTank* Tank=nullptr;
        const FVector To(P.X+Dir*2200,P.Y,AWarfareFrontlineGround::HeightAt(P.X+Dir*2200,P.Y));
        if(Role==EFrontlineRole::AntiTankRifle)
        {
            Tank=B->GetWorld()->SpawnActor<AFrontlineTank>();if(!Tank||!Tank->Configure(B,EFrontlineVehicle::MarkIVMale,1,941803))return U;
            Tank->Tags.Add(TEXT("V41SpecialistFixture"));Tank->SetActorLocation(To);Tank->Simulate(.001f);
            for(auto* Man:{U,Bearer})if(auto* Gun=FrontlineMachinegunsV30::Get(Man))for(int32 I=0;I<70&&!Gun->IsSupported();++I)Gun->Update(.05f,false);
        }
        else Target=Make(1,0,EFrontlineRole::Rifle,To);
        // Evaluate the completed aim blend before taking the actual muzzle.
        // Production has already played deployment; this synchronous fixture
        // must not fire from the intervening carry-to-prone blend instead.
        for(auto* Man:{U,Bearer}){Man->ReturnToIdle();Man->SetMotion(Man->GetAimMotion());Man->Soldier->TickAnimation(.35f,false);Man->Soldier->RefreshBoneTransforms();}
        U->DischargeWeapon(Target,Tank);B->SetActorTickEnabled(true);return U;
    }
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        FString UserDir;
        if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
        {Check(TEXT("specialists_require_isolated_review"),false);return;}
        const auto Units=B->Units;const auto Squads=B->Squads;const auto Strikes=B->Strikes;const auto Tracers=B->Tracers;
        const auto SavedMission=Mission(B);const float Clock=B->Elapsed;const auto Phase=B->Phase;const int32 Outro=B->BattleOutroWinner;
        const int32 Shots=B->ShotsFired,Grenades=B->GrenadesThrown;const FString Message=B->LastMessage;
        auto& Research=const_cast<FFrontlineArmyProgress&>(B->GetActiveResearch());const auto SavedResearch=Research;
        TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
        B->Units.Empty();B->Squads.Empty();B->Strikes.Empty();B->Tracers.Empty();Mission(B).Rounds.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
        ON_SCOPE_EXIT
        {
            for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
            B->Units=Units;B->Squads=Squads;B->Strikes=Strikes;B->Tracers=Tracers;B->Elapsed=Clock;B->Phase=Phase;B->BattleOutroWinner=Outro;
            B->ShotsFired=Shots;B->GrenadesThrown=Grenades;B->LastMessage=Message;Research=SavedResearch;Mission(B)=SavedMission;
        };
        const int32 British=B->GetFactionForTeam(0)==EFrontlineFaction::British?0:1,German=1-British;
        auto Make=[&](EFrontlineRole Role,int32 Team,int32 Member,int32 SquadId,const FVector& P)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            U->Initialize(B,SquadId,Team,Role,Member);U->SetActorTickEnabled(false);U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));
            U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->bMoving=false;U->Suppression=0;U->RoundsRemaining=U->GetMagazineCapacity();B->Units.Add(U);return U;
        };
        const int32 UKDir=B->GetTeamDirection(British),DEDir=B->GetTeamDirection(German);
        auto* UK=Make(EFrontlineRole::RifleGrenadier,British,0,941501,FVector(-UKDir*1800,0,2200));
        auto* Enemy=Make(EFrontlineRole::Rifle,German,0,941502,FVector(UKDir*100,0,2200));
        auto* AT=Make(EFrontlineRole::AntiTankRifle,German,0,941503,FVector(-DEDir*2000,1100,2200));
        auto* Bearer=Make(EFrontlineRole::AntiTankRifle,German,1,941503,FVector(-DEDir*2050,1200,2200));
        Check(TEXT("both_specialists_and_rifle_assistant_are_real_actors"),UK&&Enemy&&AT&&Bearer);if(!UK||!Enemy||!AT||!Bearer)return;
        FFrontlineSquad Crew;Crew.Id=941503;Crew.Team=German;Crew.Role=EFrontlineRole::AntiTankRifle;Crew.Trench=INDEX_NONE;Crew.Members={AT,Bearer};B->Squads.Add(Crew);
        Check(TEXT("specialists_have_one_round_and_assistant_keeps_ordinary_rifle"),UK->GetMagazineCapacity()==1&&AT->GetMagazineCapacity()==1&&Bearer->GetMagazineCapacity()==5&&!AT->IsAutomaticWeapon()&&FrontlineWeapons::ProfileFor(Bearer).FireRangeCm<FrontlineWeapons::ProfileFor(AT).FireRangeCm);
        const FVector UKMuzzle=UK->GetMuzzleLocation();const float EnemyHealth=Enemy->Health;UK->DischargeWeapon(Enemy);
        const bool Launched=B->Strikes.Num()==1&&UK->ShotsDischarged==1&&UK->RoundsRemaining==0;
        Check(TEXT("No23_launches_real_ballistic_projectile_from_muzzle_without_instant_hit"),Launched&&B->Strikes[0].bRifleLaunched&&B->Strikes[0].bGrenade&&B->Strikes[0].Projectile.IsValid()&&B->Strikes[0].Origin.Equals(UKMuzzle,.1)&&B->Strikes[0].ImpactAt>B->Elapsed&&Enemy->Health==EnemyHealth);
        Check(TEXT("rifle_grenade_uses_rifle_recoil_not_hand_throw"),Launched&&UK->WeaponPhase==EFrontlineWeaponPhase::Recoil&&UK->GrenadeThrowsStarted==0);
        if(Launched)
        {
            auto* Mesh=Cast<UStaticMeshComponent>(B->Strikes[0].Projectile->GetRootComponent());const UStaticMesh* Shape=Mesh?Mesh->GetStaticMesh().Get():nullptr;
            bool Axis=Shape!=nullptr;if(Shape){const FVector Size=Shape->GetBoundingBox().GetSize();Axis&=Size.Y>27&&Size.Y<29&&Size.X>5&&Size.X<6.2&&Size.Z>5&&Size.Z<6.2;}
            for(const FVector V:{FVector(1400,0,760),FVector(-1400,180,-610),FVector(0,800,150),FVector(80,-800,-1200)})
                Axis&=FrontlineSpecialistsV41::RifleGrenadeRotation(V).RotateVector(FVector::RightVector).Equals(V.GetSafeNormal(),.0001);
            const auto& S=B->Strikes[0];const float Flight=S.ImpactAt-S.LaunchedAt;const FVector LaunchVelocity=(S.Target-S.Origin)/Flight+FVector(0,0,490.f*Flight);
            Axis&=S.Projectile->GetActorQuat().RotateVector(FVector::RightVector).Equals(LaunchVelocity.GetSafeNormal(),.0001);
            Check(TEXT("actual_No23_Y_axis_mesh_faces_ballistic_velocity_in_both_directions_and_pitch"),Axis);
        }
        if(Launched)
        {
            const FVector Committed=B->Strikes[0].Target;Enemy->AddActorWorldOffset(FVector(0,650,0));
            Check(TEXT("rifle_grenade_does_not_follow_target_after_launch"),B->Strikes[0].Target.Equals(Committed,.001));
            UK->UpdateWeaponCycle(UK->FirePoseClock+.01f);const float Reload=UK->FirePoseClock;UK->DischargeWeapon(Enemy);
            Check(TEXT("rifle_grenade_has_seven_second_physical_single_round_reload"),UK->WeaponPhase==EFrontlineWeaponPhase::Reload&&UK->ShotsDischarged==1&&UK->RoundsRemaining==0&&Reload>=4.16f&&Reload<=7.01f);
        }
        auto* Tank=B->GetWorld()->SpawnActor<AFrontlineTank>();const bool TankReady=Tank&&Tank->Configure(B,EFrontlineVehicle::MarkIVMale,British,941504);
        Check(TEXT("AP_review_uses_real_collision_hull"),TankReady);if(!TankReady)return;
        Tank->SetActorLocation(FVector(0,1100,0));Tank->Simulate(.001f);Tank->SetActorLocation(FVector(0,1100,2200));Tank->SetActorRotation(FRotator::ZeroRotator);
        auto* Gun=FrontlineMachinegunsV30::Get(AT);Check(TEXT("AT_has_all_twelve_authored_supported_actions"),Gun&&Gun->HasAssets());if(!Gun)return;
        AT->DischargeWeapon(nullptr,Tank);Check(TEXT("heavy_AT_cannot_fire_while_carried"),AT->ShotsDischarged==0&&Mission(B).Rounds.IsEmpty());
        B->Units={AT,Bearer};for(int32 I=0;I<60&&!Gun->IsSupported();++I)Gun->Update(.05f,false);
        Check(TEXT("AT_deployment_is_required_before_aimed_fire"),Gun->CanFire()&&Gun->Deployments==1&&Gun->LastDeploymentSeconds>.5f);
        AT->SetMotion(AT->GetAimMotion());AT->Soldier->TickAnimation(.35f,false);AT->Soldier->RefreshBoneTransforms();
        const float Hull=Tank->GetHullIntegrity(),Left=Tank->GetTrackIntegrity(0),Right=Tank->GetTrackIntegrity(1);
        bool RealHullHit=false,Deferred=true;int32 HullRound=INDEX_NONE;
        for(int32 I=0;I<12&&HullRound==INDEX_NONE;++I)
        {
            AT->WeaponPhase=EFrontlineWeaponPhase::Ready;AT->RoundsRemaining=1;AT->MotionRandom.Initialize(410701+I*37);AT->DischargeWeapon(nullptr,Tank);
            if(!Mission(B).Rounds.IsEmpty())
            {
                auto& Round=Mission(B).Rounds.Last();Deferred&=Round.Arrival>B->Elapsed&&Round.Chance>=.04f&&Round.Chance<=.65f;
                if(Round.Tank.Get()==Tank){HullRound=Mission(B).Rounds.Num()-1;RealHullHit=true;}
            }
        }
        Check(TEXT("AT_shot_physically_intersects_actual_hull_before_deferred_penetration"),RealHullHit&&Deferred&&Tank->GetHullIntegrity()==Hull&&Tank->GetTrackIntegrity(0)==Left&&Tank->GetTrackIntegrity(1)==Right);
        if(HullRound!=INDEX_NONE)
        {
            const FAPRound Captured=Mission(B).Rounds[HullRound];Mission(B).Rounds={Captured};Mission(B).Rounds[0].Roll=0;
            B->Elapsed=Captured.Arrival+.001f;FrontlineSpecialistsV41::Tick(B);
            const float Lost=Hull-Tank->GetHullIntegrity()+Left-Tank->GetTrackIntegrity(0)+Right-Tank->GetTrackIntegrity(1);
            Check(TEXT("committed_AP_strike_resolves_one_bounded_hull_or_track_hit"),Lost>0&&Lost<=90.01f&&Mission(B).Rounds.IsEmpty());
            const float AfterHull=Tank->GetHullIntegrity(),AfterLeft=Tank->GetTrackIntegrity(0),AfterRight=Tank->GetTrackIntegrity(1);
            auto Miss=Captured;Miss.Roll=0;Miss.Arrival=B->Elapsed+.025f;Mission(B).Rounds={Miss};Tank->AddActorWorldOffset(FVector(0,1400,0));B->Elapsed+=.05f;FrontlineSpecialistsV41::Tick(B);
            Check(TEXT("AP_round_cannot_follow_a_hull_that_moves_clear"),Tank->GetHullIntegrity()==AfterHull&&Tank->GetTrackIntegrity(0)==AfterLeft&&Tank->GetTrackIntegrity(1)==AfterRight);
        }
        AT->WeaponPhase=EFrontlineWeaponPhase::Recoil;AT->RoundsRemaining=0;AT->FirePoseClock=.1f;AT->UpdateWeaponCycle(.11f);const float APReload=AT->FirePoseClock;
        AT->UpdateWeaponCycle(APReload-.01f);const bool EmptyUntilComplete=AT->RoundsRemaining==0;AT->UpdateWeaponCycle(.02f);
        Check(TEXT("AT_reload_has_no_free_round_before_completed_feed_action"),APReload>=4.75f&&APReload<=8.01f&&EmptyUntilComplete&&AT->RoundsRemaining==1&&AT->WeaponPhase==EFrontlineWeaponPhase::Ready);
        // Test the actual research/helper interaction on the player team without
        // changing campaign ownership or writing a save.
        const int32 OriginalTeam=AT->Team;AT->Team=Bearer->Team=0;B->Squads[0].Team=0;Research.TechnologyMask|=1u<<25;
        const float Assisted=FrontlineSpecialistsV41::ReloadSeconds(AT);Bearer->Health=0;const float Solo=FrontlineSpecialistsV41::ReloadSeconds(AT);
        Check(TEXT("ammunition_carrier_reload_benefit_ends_when_assistant_dies"),FMath::IsNearlyEqual(Assisted,5.44f)&&FMath::IsNearlyEqual(Solo,6.4f));AT->Team=Bearer->Team=OriginalTeam;
        bool SameBallistics=true,Earlier=true;for(bool Mortar:{false,true})
        {
            const auto A=FrontlineFireSupportV26::Plan(Mortar,41026,false,false,false,false,0),Z=FrontlineFireSupportV26::Plan(Mortar,41026,false,false,false,false,2);
            SameBallistics&=A.Num()==Z.Num();for(int32 I=0;I<A.Num()&&I<Z.Num();++I){Earlier&=FMath::IsNearlyEqual(A[I].ImpactDelay-Z[I].ImpactDelay,2.f,.001f);SameBallistics&=A[I].Offset.Equals(Z[I].Offset,.001)&&A[I].TerminalFlight==Z[I].TerminalFlight&&A[I].Salvo==Z[I].Salvo;}
        }
        Check(TEXT("telephone_brings_committed_indirect_salvos_two_seconds_earlier_without_retargeting"),Earlier&&SameBallistics);
    }
};
void FrontlineSpecialistsV41::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check){FFrontlineSpecialistsV41Review::Run(B,Check);}
void FrontlineSpecialistsV41::RunChecksV75(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check){FFrontlineSpecialistsV41Review::RunV75(B,Check);}
#if WITH_EDITOR
TSharedRef<FJsonObject> FrontlineSpecialistsV41::RunFrenchVBMechanicsV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(AWarfareFrontlineSoldier*)> Observe)
{return FFrontlineSpecialistsV41Review::RunFrenchVBMechanicsV87(B,Observe);}
#endif
AWarfareFrontlineSoldier* FrontlineSpecialistsV41::SetupVisualFixture(AWarfareFrontlineGameMode* B,EFrontlineRole Role){return FFrontlineSpecialistsV41Review::Visual(B,Role);}
TSharedRef<FJsonObject> FrontlineRunATProjectileChecksV84412(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineSpecialistsV41Review::RunATProjectilesV84412(B,Check);}
