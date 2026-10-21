#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontlineReportV839.h"
#include "WarfareFrontlineReviewV828.h"
#include "WarfareFrontlineChoiceTargetsV78.h"
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "FRRU/WarfareFRRUCampaignInternalV82.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineFrontsV76.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineGameplayV49.h"
#include "WarfareFrontlineBalanceV50.h"
#include "WarfareFrontlineGameplayV51.h"
#include "WarfareFrontlineReviewV50.h"
#include "WarfareFrontlineReviewV51.h"
#include "WarfareFrontlineReviewV52.h"
#include "WarfareFrontlineReviewV53.h"
#include "WarfareFrontlineReviewV54.h"
#include "WarfareFrontlineBalanceV52.h"
#include "WarfareFrontlineDebugConsoleV37.h"
#include "WarfareFrontlineMotionV34.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlinePerformanceV33.h"
#include "WarfareFrontlineDistributionReview.h"
#include "WarfareFrontlineWoundFX.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlineSupportArrivalV30.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineWaterReviewV72.h"
#include "WarfareFrontlineTerrainTacticsV54.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineVoiceContext.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlineMineV8214.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineSupportV32.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineReviewV29.h"
#include "WarfareFrontlineReviewV34.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineReviewV42.h"
#include "WarfareFrontlineReviewV43.h"
#include "WarfareFrontlineReviewV45.h"
#include "WarfareFrontlineReviewV46.h"
#include "WarfareFrontlineReviewV47.h"
#include "WarfareFrontlineReviewV48.h"
#include "WarfareFrontlineReviewV49.h"
#include "WarfareFrontlinePromoV49.h"
#include "WarfareFrontlineCharacterV29.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Dom/JsonObject.h"

bool FrontlineUseLegacyCombatV835();

namespace FrontlineTargetReuseV834
{
struct FReview
{
    uint64 Acquisitions=0,SightRequests=0,QueryBuilds=0,TargetChecks=0,TargetFailures=0;
};
FReview& State(){static FReview S;return S;}
bool Enabled(const AWarfareFrontlineGameMode* B)
{
    static const bool Requested=FParse::Param(FCommandLine::Get(),TEXT("WFV834Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
    return Requested&&B&&B->IsProgressionVolatile();
}
TSharedPtr<FJsonObject> Review()
{
    const auto& S=State();auto J=MakeShared<FJsonObject>();
    J->SetNumberField(TEXT("acquisitions"),double(S.Acquisitions));
    J->SetNumberField(TEXT("local_sight_requests"),double(S.SightRequests));
    J->SetNumberField(TEXT("local_query_builds"),double(S.QueryBuilds));
    J->SetNumberField(TEXT("avoided_local_query_builds"),double(S.SightRequests-S.QueryBuilds));
    J->SetNumberField(TEXT("same_frame_base_target_checks"),double(S.TargetChecks));
    J->SetNumberField(TEXT("same_frame_base_target_failures"),double(S.TargetFailures));
    return J;
}
}

namespace
{
float FloorZ(const FVector& P) { return AWarfareFrontlineGround::HeightAt(P.X,P.Y); }
bool ValidTeam(int32 Team) { return Team==0 || Team==1; }
bool ValidRole(EFrontlineRole Role) { return uint8(Role)<uint8(EFrontlineRole::Count); }

bool HasSightWithParamsV834(const AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* From,
    const AWarfareFrontlineSoldier* To,const FCollisionQueryParams& Params)
{
    FHitResult Hit;const FVector Origin=FrontlineMachinegunsV30::SightOrigin(From);
    const bool National=WarfareFRRUCampaignV82::Active(B);
    const FVector Primary=FrontlineMachinegunsV30::SightTarget(To);
    if((!National||WarfareFRRUWorldV82::HasSight(B,Origin,Primary))&&!B->GetWorld()->LineTraceSingleByChannel(Hit,Origin,Primary,ECC_Visibility,Params))return true;
    // Keep the real exposed-head fallback and physical parapet test unchanged.
    const FVector Head=To->Soldier?To->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5):To->Feet()+FVector(0,0,145);
    return To->Soldier&&(!National||WarfareFRRUWorldV82::HasSight(B,Origin,Head))&&!B->GetWorld()->LineTraceSingleByChannel(Hit,Origin,Head,ECC_Visibility,Params);
}

float FormationLaneOffset(int32 Roster,int32 Member,float PersonalY,bool National)
{
    Roster=FMath::Max(1,Roster);
    // A six-man original roster must fit inside its 630 cm slot, even when
    // both edge men lean toward the next section. Keep smaller crews and the
    // national formation contract intact; casualties never recenter the lane.
    // Six/six and six/five worst edge gaps are 74 cm, above 56 cm body clearance.
    const bool Compact=!National&&Roster==6;
    return (Member-(Roster-1)*.5f)*(Compact?108.f:125.f)+PersonalY*(Compact?8.f/28.f:1.f);
}

// A destination reserves a resident slot, but a full friendly trench is never
// a wall. Keep formation lanes valid while moving past a trench without a slot.
int32 OpenTrenchSlot(const AWarfareFrontlineGameMode& B,int32 Index,int32 Team,int32 IgnoreSquad)
{
    if (!B.Trenches.IsValidIndex(Index)) return INDEX_NONE;
    bool Taken[FrontlineBattlefield::TrenchCapacity]={};
    // Actual occupants always win. A formation lane is not a second booking of
    // an occupied slot, even when its incoming squad has a lower identifier.
    for (const auto& S:B.Squads)
        if (S.Id!=IgnoreSquad && S.Team==Team && S.LivingCount()>0 && S.Slot>=0 && S.Slot<FrontlineBattlefield::TrenchCapacity
            && S.Trench==Index) Taken[S.Slot]=true;
    // Existing movers yield only to earlier bookings. A newer mover may still
    // carry the same fallback formation lane while approaching an enemy trench;
    // it must not lock an earlier squad out. New recruits inspect all bookings.
    for (const auto& S:B.Squads)
        if (S.Id!=IgnoreSquad && S.Team==Team && S.LivingCount()>0 && S.Slot>=0 && S.Slot<FrontlineBattlefield::TrenchCapacity
            && S.Trench==INDEX_NONE && S.bAdvancing && S.Destination==Index && (IgnoreSquad<0 || S.Id<IgnoreSquad)
            && !Taken[S.Slot]) Taken[S.Slot]=true;
    for (int32 I=0;I<FrontlineBattlefield::TrenchCapacity;++I) if (!Taken[I]) return I;
    return INDEX_NONE;
}
bool TrenchOpposed(const AWarfareFrontlineGameMode& B,int32 Index,int32 Team)
{
    if (!B.Trenches.IsValidIndex(Index)) return false;
    if (B.TrenchSquadCount(Index,1-Team)>0) return true;
    for (const AWarfareFrontlineSoldier* U:B.Units)
        if (IsValid(U) && U->Team!=Team && U->IsAlive() && FMath::Abs(U->Feet().X-B.Trenches[Index].X)<=400.f
            && FMath::Abs(U->Feet().Y)<=FrontlineBattlefield::HalfFrontage) return true;
    return false;
}
int32 ForwardTrenchDestination(const AWarfareFrontlineGameMode& B,int32 First,int32 Team,int32 IgnoreSquad)
{
    for (int32 I=First;B.Trenches.IsValidIndex(I);I+=B.GetTeamDirection(Team))
    {
        // Enemy occupation always takes precedence over the historical owner,
        // full slots or the friendly bypass lever. An abandoned enemy trench
        // remains a capture destination, never an impassable ownership flag.
        if (TrenchOpposed(B,I,Team) || B.Trenches[I].Owner!=Team) return I;
        if (!B.Trenches[I].bBypass && OpenTrenchSlot(B,I,Team,IgnoreSquad)!=INDEX_NONE) return I;
    }
    return B.GetGoalTrenchIndex(Team);
}
int32 OpposedTrenchOnRoute(const AWarfareFrontlineGameMode& B,const FFrontlineSquad& S)
{
    const int32 Dir=B.GetTeamDirection(S.Team);
    const FVector Centre=S.Centre();
    const float End=B.Trenches.IsValidIndex(S.Destination)?B.Trenches[S.Destination].X:Dir*FrontlineBattlefield::GoalHalfLength;
    for (int32 I=B.GetHomeTrenchIndex(S.Team);B.Trenches.IsValidIndex(I);I+=Dir)
        if ((B.Trenches[I].X-Centre.X)*Dir>=-180.f && (End-B.Trenches[I].X)*Dir>=0 && TrenchOpposed(B,I,S.Team)) return I;
    return INDEX_NONE;
}
}

int32 FFrontlineSquad::LivingCount() const
{
    int32 Count=0; for (const TWeakObjectPtr<AWarfareFrontlineSoldier>& U:Members) if (U.IsValid() && U->IsAlive()) ++Count;
    return Count;
}

FVector FFrontlineSquad::Centre() const
{
    FVector Sum=FVector::ZeroVector; int32 Count=0;
    for (const TWeakObjectPtr<AWarfareFrontlineSoldier>& U:Members) if (U.IsValid() && U->IsAlive()) { Sum+=U->Feet(); ++Count; }
    return Count?Sum/Count:FVector::ZeroVector;
}

AWarfareFrontlineGameMode::AWarfareFrontlineGameMode()
{
    PrimaryActorTick.bCanEverTick=true; DefaultPawnClass=nullptr;
    PlayerControllerClass=AWarfareFrontlineController::StaticClass(); HUDClass=AWarfareFrontlineHUD::StaticClass();
}

// Team 0 always means the human army. Physical travel belongs to faction,
// keeping campaign/profile keys and every friendly command independent of side.
int32 AWarfareFrontlineGameMode::GetTeamDirection(int32 Team) const
{ if(WarfareFRRUCampaignV82::Active(this))return Team==1?-1:1;return GetFactionForTeam(Team)==EFrontlineFaction::British?1:-1; }
int32 AWarfareFrontlineGameMode::GetHomeTrenchIndex(int32 Team) const
{ return GetTeamDirection(Team)>0?0:FrontlineBattlefield::TrenchCount-1; }
int32 AWarfareFrontlineGameMode::GetGoalTrenchIndex(int32 Team) const
{ return GetTeamDirection(Team)>0?FrontlineBattlefield::TrenchCount:-2; }
float AWarfareFrontlineGameMode::GetHomeCameraFocusX() const
{
    if(WarfareFRRUCampaignV82::Active(this))
    {FVector Home;float Radius=0;return WarfareFRRUWorldV82::NodePose(this,TEXT("home"),Home,Radius)?float(Home.X)+GetTeamDirection(0)*1300.f:0.f;}
    return FrontlineBattlefield::TrenchX(GetHomeTrenchIndex(0))+GetTeamDirection(0)*1300.f;
}

void AWarfareFrontlineGameMode::BeginPlay()
{
    Super::BeginPlay();
    const FString CharacterRoot=TEXT("/Game/Warfare/Characters/V11Centimeter");
    LoadedSoldier=LoadObject<USkeletalMesh>(nullptr,*(CharacterRoot+TEXT("/Meshes/SK_Soldier_V11.SK_Soldier_V11")));
    auto Animation=[&CharacterRoot](const TCHAR* Name)
    {
        const FString Path=CharacterRoot+TEXT("/Animations/")+Name+TEXT(".")+Name;
        return FrontlineMotionV34::Load(Path);
    };
    IdleAnimation=Animation(TEXT("A_LowReady_V07")); WalkAnimation=Animation(TEXT("A_Walk_V07")); RunAnimation=Animation(TEXT("A_Run_V07"));
    AimAnimation=Animation(TEXT("A_Aim_V07")); FireAnimation=Animation(TEXT("A_Fire_V07"));
    BoltAnimation=Animation(TEXT("A_BoltCycle_V11"));
    TrenchCycleAnimation=Animation(TEXT("A_TrenchCycle_V11"));
    ReloadAnimation=Animation(TEXT("A_Reload_V11"));
    AutomaticFireAnimation=Animation(TEXT("A_AutomaticFire_V11"));
    DeathAnimation=Animation(TEXT("A_DeathBack_V11"));
    const TCHAR* MeshNames[]={TEXT("SK_British_Rifle_V12"),TEXT("SK_British_Lewis_V12"),TEXT("SK_British_Mills_V12"),
        TEXT("SK_German_Gewehr98_V12"),TEXT("SK_German_MG0815_V12"),TEXT("SK_German_Assault_V12")};
    for (const TCHAR* Name:MeshNames)
        RoleSoldierMeshes.Add(LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Characters/V12/Meshes/%s.%s"),Name,Name)));
    // Presentation identities keep the original weapon/animation role indices.
    // Only the visible roster expands; all meshes use the approved cm skeleton.
    for (const TCHAR* Army:{TEXT("British"),TEXT("German")})
        for (const TCHAR* Identity:{TEXT("Rifle"),TEXT("Machinegun"),TEXT("Assault"),TEXT("Marksman"),TEXT("Sergeant"),TEXT("AmmoBearer")})
        {
            const FString Name=FString::Printf(TEXT("SK_%s_%s_V16"),Army,Identity);
            V16SoldierMeshes.Add(FrontlineCharacterV29::Load(FString::Printf(TEXT("/Game/Warfare/Characters/V16/Meshes/%s.%s"),*Name,*Name)));
        }
    auto RoleAnimation=[](const TCHAR* Name)
    { return FrontlineMotionV34::Load(FString::Printf(TEXT("/Game/Warfare/Characters/V12/Animations/%s.%s"),Name,Name)); };
    MGIdleAnimation=RoleAnimation(TEXT("A_MG_LowReady_V12")); MGAimAnimation=RoleAnimation(TEXT("A_MG_Aim_V12"));
    MGWalkAnimation=RoleAnimation(TEXT("A_MG_Walk_V12")); MGRunAnimation=RoleAnimation(TEXT("A_MG_Run_V12"));
    LewisReloadAnimation=RoleAnimation(TEXT("A_Lewis_Reload_V12")); GermanMGReloadAnimation=RoleAnimation(TEXT("A_MG0815_Reload_V12"));
    AutomaticFireAnimation=RoleAnimation(TEXT("A_MG_Fire_V12"));
    GrenadeAnimation=FrontlineMotionV34::Load(TEXT("/Game/Warfare/Characters/V24/Animations/A_Grenade_Overarm_V24.A_Grenade_Overarm_V24"));
    DeathAnimations={DeathAnimation,RoleAnimation(TEXT("A_DeathFront_V12")),RoleAnimation(TEXT("A_DeathSide_V12"))};
    if (!LoadedSoldier || !IdleAnimation || LoadedSoldier->GetSkeleton()!=IdleAnimation->GetSkeleton())
        AssetFailure=TEXT("角色或持枪待机动作缺失，请检查资源安装。");
    auto Match=[&](TObjectPtr<UAnimationAsset>& Asset) { if (Asset && (!LoadedSoldier || Asset->GetSkeleton()!=LoadedSoldier->GetSkeleton())) Asset=nullptr; };
    Match(WalkAnimation); Match(RunAnimation); Match(AimAnimation); Match(FireAnimation); Match(BoltAnimation); Match(TrenchCycleAnimation); Match(ReloadAnimation); Match(AutomaticFireAnimation);
    Match(DeathAnimation); Match(GrenadeAnimation);
    if (!BoltAnimation || !TrenchCycleAnimation || !ReloadAnimation || !AutomaticFireAnimation || !DeathAnimation) AssetFailure=TEXT("V11 动作资源缺失，请重新安装完整版本。");
    Match(MGIdleAnimation); Match(MGAimAnimation); Match(MGWalkAnimation); Match(MGRunAnimation); Match(LewisReloadAnimation); Match(GermanMGReloadAnimation);
    for (auto& Asset:DeathAnimations) Match(Asset);
    bool RolesReady=RoleSoldierMeshes.Num()==6;
    for (const auto& Mesh:RoleSoldierMeshes) RolesReady&=Mesh && LoadedSoldier && Mesh->GetSkeleton()==LoadedSoldier->GetSkeleton();
    RolesReady&=V16SoldierMeshes.Num()==12;
    for (const auto& Mesh:V16SoldierMeshes) RolesReady&=Mesh && LoadedSoldier && Mesh->GetSkeleton()==LoadedSoldier->GetSkeleton();
    for (const auto& Asset:DeathAnimations) RolesReady&=Asset!=nullptr;
    if (!GrenadeAnimation || !RolesReady || !MGIdleAnimation || !MGAimAnimation || !MGWalkAnimation || !MGRunAnimation || !LewisReloadAnimation || !GermanMGReloadAnimation)
        AssetFailure=TEXT("兵种模型或配套动作未能完整载入，请检查资源安装。");
    // Spatial weapons, battlefield beds and squad voices share one bounded audio subsystem.
    LoadProgression(); ResetEncounter();
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV828Review"))){StartFrontlineReviewV828(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFPromoV49"))){StartFrontlinePromoV49(this);return;}
    if(StartFrontlineBalanceV52(this))return;
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterReview"))){StartFrontlineWaterReviewV72(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV54Review"))){StartFrontlineV54Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV53Review"))){StartFrontlineV53Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV52Review"))){StartFrontlineV52Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV51Review"))){StartFrontlineV51Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV50Review"))){StartFrontlineV50Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV49Review"))){StartFrontlineV49Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV48Review"))){StartFrontlineV48Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV47Review"))){StartFrontlineV47Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV46Review"))){StartFrontlineV46Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV45Review"))) {StartFrontlineV45Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV43Review"))) {StartFrontlineV43Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV42Review"))) {StartFrontlineV42Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))) {StartFrontlineV41Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFDistributionCheck"))) {StartFrontlineDistributionReview(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV34Review")))
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,[this]()
        {StartFrontlineV34Review(Cast<AWarfareFrontlineController>(GetWorld()->GetFirstPlayerController()));}));return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV29Review")))
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,[this]()
        {StartFrontlineV29Review(Cast<AWarfareFrontlineController>(GetWorld()->GetFirstPlayerController()));}));return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV28Review")) || FParse::Param(FCommandLine::Get(),TEXT("WFV28FieldReview")))
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,[this]()
        {StartFrontlineV28Review(Cast<AWarfareFrontlineController>(GetWorld()->GetFirstPlayerController()));}));return;
    }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFV22Benchmark"))) { BeginV22PerformanceReview(); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFV33Benchmark"))) { StartFrontlineV33Performance(this); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFV22ImageReview"))) { BeginV22ImageReview(); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFV23MechanicsVerify"))) { BeginV23MechanicsVerification(); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFV24OfficerVerify"))) { BeginV24OfficerVerification(); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFV24GrenadeReview"))) { BeginV24GrenadeReview(); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFV24OfficerVisual"))) { BeginV24OfficerVisual(); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFV18AgileVerify")))
    {
        // Let the map's shared-height ground finish BeginPlay/collision creation.
        GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this,&AWarfareFrontlineGameMode::BeginV18AgileVerification)); return;
    }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFV17AgileVerify"))) { BeginV17AgileVerification(); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineV14Verify"))) { BeginV14GameplayVerification(); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineV15TechnologyVerify"))) { BeginV15TechnologyVerification(); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFProgressionVerify"))) { BeginProgressionVerification(); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineV13Verify"))) { BeginV13FeatureReview(); return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineProfile"))) BeginPerformanceReview();
    if (FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineMotionReview")) || FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineLookReview"))) BeginMotionReview();
    else if (FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineVerify"))) BeginVerification();
}

const TCHAR* AWarfareFrontlineGameMode::RoleName(EFrontlineRole InRole)
{
    switch (InRole)
    {
        case EFrontlineRole::RifleGrenadier:return TEXT("No.23 枪榴弹组");
        case EFrontlineRole::AntiTankRifle:return TEXT("Tankgewehr 反坦克步枪组");
        case EFrontlineRole::MobileGun:return TEXT("Lewis 机动枪组");
        case EFrontlineRole::Signals:return TEXT("前沿观测组");
        case EFrontlineRole::Flamethrower:return TEXT("喷火分队");
        case EFrontlineRole::Pioneer:return TEXT("工兵排障队");
        case EFrontlineRole::Lewis:return TEXT("Lewis 支援组");
        case EFrontlineRole::Mills:return TEXT("Mills 突击班");
        case EFrontlineRole::Marksman:return TEXT("SMLE 精确射手");
        case EFrontlineRole::Officer:return TEXT("英军军士组");
        default:return TEXT("SMLE 步枪班");
    }
}
int32 AWarfareFrontlineGameMode::RoleSize(EFrontlineRole InRole,int32 Team)
{ return InRole==EFrontlineRole::Signals?3:InRole==EFrontlineRole::MobileGun?4:InRole==EFrontlineRole::Pioneer?4:InRole==EFrontlineRole::AntiTankRifle?2:(InRole==EFrontlineRole::RifleGrenadier || InRole==EFrontlineRole::Flamethrower?3:InRole==EFrontlineRole::Marksman?2:(InRole==EFrontlineRole::Officer || InRole==EFrontlineRole::Lewis?3:(InRole==EFrontlineRole::Mills?4:6))); }
// Company readiness is a game balance budget, not a historical loading time.
// The two factions receive the same rifle section size; their weapon profiles
// retain the historical differences without a hidden manpower/readiness trade.
float AWarfareFrontlineGameMode::RoleRecruitSeconds(EFrontlineRole InRole)
{
    // All roles still occupy one common queue. Six riflemen formerly arrived
    // four times as fast, per man, as a gun crew; tactical choices could not
    // repay that opportunity cost. Specialist tools now cost a comparable wait.
    switch(InRole)
    {
        case EFrontlineRole::MobileGun:return 13.5f*FrontlinePacingV24::BaseRecruitScale;
        case EFrontlineRole::Signals:return 15.5f*FrontlinePacingV24::BaseRecruitScale;
        case EFrontlineRole::RifleGrenadier:return 12.f*FrontlinePacingV24::BaseRecruitScale;
        case EFrontlineRole::AntiTankRifle:return 32.f*.70f*FrontlinePacingV24::BaseRecruitScale;
        case EFrontlineRole::Flamethrower:return 13.f*FrontlinePacingV24::BaseRecruitScale;
        case EFrontlineRole::Pioneer:return 10.5f*FrontlinePacingV24::BaseRecruitScale;
        case EFrontlineRole::Lewis:return 13.5f*FrontlinePacingV24::BaseRecruitScale;
        case EFrontlineRole::Mills:return 10.5f*FrontlinePacingV24::BaseRecruitScale;
        case EFrontlineRole::Marksman:return 12.f*FrontlinePacingV24::BaseRecruitScale;
        case EFrontlineRole::Officer:return 9.f*FrontlinePacingV24::BaseRecruitScale;
        default:return 11.f*FrontlinePacingV24::BaseRecruitScale;
    }
}
float AWarfareFrontlineGameMode::RoleFireRange(EFrontlineRole InRole)
{ return InRole==EFrontlineRole::Marksman?5600.f:(InRole==EFrontlineRole::Lewis?4800.f:(InRole==EFrontlineRole::Mills?3400.f:4400.f)); }
float AWarfareFrontlineGameMode::RoleAdvanceContactRange(EFrontlineRole InRole)
{ return InRole==EFrontlineRole::Marksman?4100.f:(InRole==EFrontlineRole::Lewis?3200.f:(InRole==EFrontlineRole::Mills?750.f:(InRole==EFrontlineRole::Officer?2100.f:2500.f))); }

void AWarfareFrontlineGameMode::ResetEncounter()
{
    if(!WarfareFRRUWorldV82::Reset(this)){LastMessage=WarfareFRRUCampaignV82::Status(this);return;}
    FrontlineChoiceTargetsV78::Reset(this);
    if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>()) Audio->ResetEncounterAudio();
    // Cancel delayed fuel/fire and support emitters before their source actors
    // disappear. An abandoned encounter must not leak into the next deployment.
    FrontlineFieldV32::Reset(this);
    if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(GetWorld()))Vehicles->ResetBattle();
    UGameplayStatics::SetGamePaused(this,false); UGameplayStatics::SetGlobalTimeDilation(this,1); BattleSpeed=1;
    for (AWarfareFrontlineSoldier* U:Units) if (IsValid(U)) U->Destroy();
    for (TActorIterator<AWarfareFrontlineImpact> It(GetWorld());It;++It) It->Destroy();
    for (TActorIterator<AWarfareFrontlineSmoke> It(GetWorld());It;++It) It->Destroy();
    for (TActorIterator<AWarfareFrontlineWoundFX> It(GetWorld());It;++It) It->Destroy();
    for (TActorIterator<AFrontlineSupportMissionV28> It(GetWorld());It;++It) It->Destroy();
    for (auto& Strike:Strikes) if (Strike.Projectile.IsValid()) Strike.Projectile->Destroy();
    Units.Empty(); Squads.Empty(); Strikes.Empty(); Tracers.Empty(); Trenches.Empty(); SmokeZones.Empty();
    const auto* SelectedOperation=GetSelectedCampaignMission();
    FrontlineBattlefield::ConfigureOperationDepth(FrontlineGameplayV33::OperationDepthScale(SelectedOperation?SelectedOperation->Id:INDEX_NONE),SelectedOperation?SelectedOperation->Id:INDEX_NONE);
    if(WarfareFRRUCampaignV82::Active(this))
    {
        FString Scene;uint32 Revision=0;int32 Solids=0;FVector2D Minimum,Maximum;
        if(WarfareFRRUWorldV82::SceneInfo(this,Scene,Revision,Solids,Minimum,Maximum))FrontlineBattlefield::ConfigurePhysicalScene(float(Maximum.X),float(Maximum.Y));
    }
    for (int32 I=0;I<FrontlineBattlefield::TrenchCount;++I)
    {
        FFrontlineTrench T; T.X=FrontlineBattlefield::TrenchX(I);
        const int32 LeftTeam=GetTeamDirection(0)>0?0:1;
        T.Owner=(FrontlineBattlefield::TrenchCount%2==1&&I==FrontlineBattlefield::CentreTrench)?INDEX_NONE
            :I<FrontlineBattlefield::TrenchCount/2?LeftTeam:1-LeftTeam;
        Trenches.Add(T);
    }
    for (int32 Team=0;Team<2;++Team)
    {
        Morale[Team]=100; Casualties[Team]=0; DamageDealt[Team]=0; CombatRecordV65[Team]=FFrontlineCombatRecordV65();
        Supplies[Team]=0; CommandPoints[Team]=0;
        SuppliesEarned[Team]=SuppliesSpent[Team]=CommandEarned[Team]=CommandSpent[Team]=0;
        SmokeAffectedShots[Team]=0; RallySuppressionRemoved[Team]=RallyMoraleRestored[Team]=0;
        for (int32 RoleIndex=0;RoleIndex<RoleCount;++RoleIndex) RecruitReadyAt[Team][RoleIndex]=0;
        ArtilleryReadyAt[Team]=EffectiveArtillerySeconds(Team);
        SmokeReadyAt[Team]=FrontlineGameplayV43::PreparationSeconds(12.f,Team, GetCampaignDifficulty()); RallyReadyAt[Team]=0;
        MortarReadyAt[Team]=FrontlineGameplayV43::PreparationSeconds(SupportCooldownSeconds(EFrontlineSupport::Mortar),Team, GetCampaignDifficulty());
        for(int32 I=0;I<3;++I)ExtendedSupportReadyAt[Team][I]=FrontlineGameplayV43::PreparationSeconds((Team?74.f:52.f)+I*15.f,Team, GetCampaignDifficulty());
        LeaderOrders[Team]=0; LeaderSuppressionRemoved[Team]=0;
    }
    FrontlineReportV839::Begin(this);
    BattleOutroElapsed=0;BattleOutroWinner=INDEX_NONE;
    Elapsed=0; Phase=EFrontlinePhase::Briefing; NextSquadId=1; EnemyRecruitStep=0;
    EnemyDecisionRandom.Initialize(SelectedOperation?SelectedOperation->RandomSeed+540209:731917);
    EnemyThinkAt=1.f; EnemyRecruitAt=FrontlineGameplayV43::PreparationSeconds(EnemyDecisionRandom.FRandRange(2.6f,4.4f),1, GetCampaignDifficulty()); EnemyAssaultAt=9;
    EnemyWaveStartedAt=0.f; EnemyWaveRecruitCount=0; EnemyCounterattackTrench=INDEX_NONE; EnemyCounterattackUntil=0.f;
    EnemyFireAssessAt=EnemyDecisionRandom.FRandRange(8.f,13.f); EnemyFireExecuteAt=0;
    bEnemyRoleCommitted=bEnemyFirePending=bEnemyPendingMortar=false; EnemyPlannedRole=EFrontlineRole::Rifle;
    EnemyObservedFireTarget=FVector::ZeroVector;
    ArtilleryReadyAt[1]+=FrontlineGameplayV43::PreparationSeconds(EnemyDecisionRandom.FRandRange(11.f,27.f),1, GetCampaignDifficulty());
    bCampaignMissionAdvanced=false; CampaignHoldElapsed=0.f;
    if (const auto* Mission=GetSelectedCampaignMission())
    {
        Morale[0]=Mission->PlayerMorale; Morale[1]=Mission->EnemyMorale;
        EnemyAssaultAt=Mission->EnemyAssaultDelay*FrontlinePacingV24::TempoIntervalScale;
        ArtilleryReadyAt[0]=Mission->PlayerArtilleryDelay*EffectiveArtillerySeconds(0)/SupportCooldownSeconds(EFrontlineSupport::Artillery);
        ArtilleryReadyAt[1]=FrontlineGameplayV43::PreparationSeconds(Mission->EnemyArtilleryDelay+EnemyDecisionRandom.FRandRange(4.f,14.f),1, GetCampaignDifficulty());
        SmokeReadyAt[0]=Mission->PlayerSmokeDelay; MortarReadyAt[0]=Mission->PlayerMortarDelay;
    }
    // Rebuild capacity from the mission baseline on every encounter. Losses and
    // recoveries keep their own amounts; retry cannot multiply capacity twice.
    for(int32 Team=0;Team<2;++Team)Morale[Team]*=FrontlineMoraleV36::CapacityMultiplierV85(this);
    EncounterId=FGuid::NewGuid();BoundCampaignId.Invalidate();BoundCampaignEncounterId.Invalidate();BoundCampaignMission=INDEX_NONE;
    FrontlineFrontsV76::Reset(this);FrontlineAchievementsV76::ResetEncounter(this);FrontlineDifficultyV30::Reset(this);FrontlineMoraleV36::Reset(this);FrontlineFieldV32::Reset(this); bEncounterRewarded=false; LastResearchReward=0; ResearchBeforeOutcomeV828=INDEX_NONE;
    ShotsFired=RecruitCommands=AdvanceCommands=ArtilleryCommands=GrenadesThrown=TrenchCaptures=0;
    SmokeCommands=RallyCommands=MortarCommands=MortarRoundsResolved=0;
    for(int32 I=0;I<3;++I)ExtendedSupportCommands[I]=0;
    ExtendedSupportThinkAt=8.f;
    FriendlyFireDamage=PeakSuppression=0; EndReason.Empty(); Random.Initialize(191708);
    LastMessage=TEXT("选择兵力投入战场，再逐壕下达命令。");
    if (auto* PC=Cast<AWarfareFrontlineController>(GetWorld()->GetFirstPlayerController())) PC->SelectedTrench=INDEX_NONE;
    if (!AssetFailure.IsEmpty()) UE_LOG(LogTemp,Error,TEXT("Frontline: %s No substitute units spawned."),*AssetFailure);
}

void AWarfareFrontlineGameMode::BeginBattle()
{
    if(WarfareFRRUCampaignV82::Active(this))
    {const FString RosterBlock=WarfareFRRUCampaignV82::DeploymentBlockReason(this);if(!RosterBlock.IsEmpty()){LastMessage=RosterBlock;return;}}
    if(WarfareFRRUCampaignV82::Active(this)&&!WarfareFRRUCampaignV82::WorldReady(this))
    {LastMessage=WarfareFRRUCampaignV82::WorldBlockReason(this);return;}
    if (Phase!=EFrontlinePhase::Briefing || !AssetFailure.IsEmpty()) return;
    if(WarfareFRRUCampaignV82::Active(this))
    {
        FString Error;if(!WarfareFRRUWorldV82::Begin(this,Error)){LastMessage=Error;return;}
        Phase=EFrontlinePhase::Fighting;FrontlineMoraleV36::Reset(this);FrontlineAchievementsV76::BeginEncounter(this);LastMessage=TEXT("Orders issued. Reinforcements are approaching.");
        if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->SetBattleActive(true);return;
    }
    FrontlineChoiceTargetsV78::Reset(this);
    BindCampaignEncounter();FrontlineFieldV32::Begin(this);
    Phase=EFrontlinePhase::Fighting;FrontlineMoraleV36::Reset(this);FrontlineAchievementsV76::BeginEncounter(this); LastMessage=TEXT("开始部署 · 双方兵力从后方增援进入战场。");
    const auto* Operation=GetEncounterCampaignMission();
    UE_LOG(LogTemp,Display,TEXT("WF_V33_DEPLOYMENT mission=%d difficulty=%d trench_pitch_cm=%.0f entry_half_cm=%.0f initial_squads=%d"),
        Operation?Operation->Id:INDEX_NONE,int32(GetCampaignDifficulty()),FrontlineBattlefield::TrenchSpacing,FrontlineBattlefield::SpawnHalfLength,Squads.Num());
    if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>()) Audio->SetBattleActive(true);
}
bool AWarfareFrontlineGameMode::RestartBattle()
{
    // A replay retires the ended encounter just like returning to operations.
    // Keep the result and its final support kills available if storage fails.
    if(!FrontlineAchievementsV76::FinalizeBattleKillsV8217(this))return false;
    ResetEncounter();return Phase==EFrontlinePhase::Briefing;
}
void AWarfareFrontlineGameMode::AbandonBattleToOperations()
{
    // Reset is deliberately outside FinishBattle/AwardEncounterResearch. The
    // selected operation, assignment, research and unlocked route do not change.
    ResetEncounter();
    if(auto* PC=Cast<AWarfareFrontlineController>(GetWorld()->GetFirstPlayerController()))PC->CancelTargeting();
    LastMessage=TEXT("部队撤回集结地，等待新的作战命令。");
}
void AWarfareFrontlineGameMode::SetPaused(bool bPaused) { if (Phase==EFrontlinePhase::Fighting) UGameplayStatics::SetGamePaused(this,bPaused); }
void AWarfareFrontlineGameMode::SetBattleSpeed(float Multiplier)
{
    // Persisted commands and older callers requesting 2x adopt the new fast rate.
    if(Multiplier==2.f)Multiplier=1.5f;
    // The ordinary HUD offers half, normal and 1.5x speed; private 4x remains.
    // All simulation and effects share world dilation; reject invalid input.
    const bool Supported=Multiplier==.5f || Multiplier==1.f || Multiplier==1.5f || Multiplier==4.f;
    BattleSpeed=Supported?Multiplier:1.f;
    UGameplayStatics::SetGlobalTimeDilation(this,BattleSpeed);
}
float AWarfareFrontlineGameMode::RecruitCooldown(EFrontlineRole InRole,int32 Team) const
{ return ValidTeam(Team) && ValidRole(InRole)?FMath::Max(0.f,RecruitReadyAt[Team][int32(InRole)]-Elapsed):MAX_flt; }
float AWarfareFrontlineGameMode::ArtilleryCooldown(int32 Team) const
{ return ValidTeam(Team)?FMath::Max(0.f,ArtilleryReadyAt[Team]-Elapsed):MAX_flt; }

FString AWarfareFrontlineGameMode::RecruitBlockReason(EFrontlineRole InRole,int32 Team) const
{
    if (!ValidTeam(Team) || !ValidRole(InRole)) return TEXT("兵种不可用。");
    if (!IsRoleUnlocked(InRole,Team)) return RoleUnlockReason(InRole,Team);
    if(!WarfareFRRUCampaignV82::Active(this))if(const FString Loan=FrontlineCampaignV52::LoanRecruitBlockReason(this,InRole,Team);!Loan.IsEmpty())return Loan;
    if(const FString Loadout=FrontlineCampaignV32::RoleBlockReason(this,InRole,Team);!Loadout.IsEmpty())return Loadout;
    if (!AssetFailure.IsEmpty()) return AssetFailure;
    if (Phase!=EFrontlinePhase::Fighting) return TEXT("请先开始战斗。");
    if (UGameplayStatics::IsGamePaused(this)) return TEXT("暂停中，请先继续。");
    if (RecruitCooldown(InRole,Team)>.01f) return TEXT("该兵种仍在整备。");
    return FString();
}
FString AWarfareFrontlineGameMode::AdvanceBlockReason(int32 Index,int32 Team) const
{
    if (!ValidTeam(Team) || !Trenches.IsValidIndex(Index) || Trenches[Index].Owner!=Team) return TEXT("请选择己方战壕。");
    if (Phase!=EFrontlinePhase::Fighting) return TEXT("请先开始战斗。");
    if (UGameplayStatics::IsGamePaused(this)) return TEXT("暂停中，请先继续。");
    if (TrenchSquadCount(Index,Team)==0) return TEXT("壕内没有可出击的小队。");
    bool Available=false;
    for (const auto& S:Squads) if (S.Team==Team && S.Trench==Index && S.LivingCount()>0 && !S.bHoldPosition) Available=true;
    if (!Available) return TEXT("该壕小队均在留守；可单独选择小队出击。");
    return FString();
}
FString AWarfareFrontlineGameMode::ArtilleryBlockReason(int32 Team) const
{
    return SupportBlockReason(EFrontlineSupport::Artillery,Team);
}

int32 AWarfareFrontlineGameMode::ActiveSquads(int32 Team) const
{ int32 N=0; for (const FFrontlineSquad& S:Squads) if (S.Team==Team && S.LivingCount()>0) ++N; return N; }
int32 AWarfareFrontlineGameMode::LivingSoldiers(int32 Team) const
{ int32 N=0; for (const AWarfareFrontlineSoldier* U:Units) if (U->Team==Team && U->IsAlive()) ++N; return N; }
int32 AWarfareFrontlineGameMode::TrenchSquadCount(int32 Index,int32 Team) const
{ int32 N=0; for (const FFrontlineSquad& S:Squads) if (S.Team==Team && S.Trench==Index && S.LivingCount()>0) ++N; return N; }
FFrontlineSquad* AWarfareFrontlineGameMode::FindSquad(int32 Id)
{ for (FFrontlineSquad& S:Squads) if (S.Id==Id) return &S; return nullptr; }
const FFrontlineSquad* AWarfareFrontlineGameMode::FindSquad(int32 Id) const
{ for (const FFrontlineSquad& S:Squads) if (S.Id==Id) return &S; return nullptr; }

int32 AWarfareFrontlineGameMode::FreeSlot(int32 Index,int32 Team,int32 IgnoreSquad) const
{
    return OpenTrenchSlot(*this,Index,Team,IgnoreSquad);
}
int32 AWarfareFrontlineGameMode::FirstAvailableTrench(int32 Team) const
{
    return ForwardTrenchDestination(*this,GetHomeTrenchIndex(Team),Team,INDEX_NONE);
}

bool AWarfareFrontlineGameMode::SpawnSquad(int32 Team,EFrontlineRole InRole,int32 InitialTrench)
{
    const bool National=WarfareFRRUCampaignV82::Active(this);
    if (!ValidTeam(Team) || !ValidRole(InRole) || (!National&&(!LoadedSoldier||!IdleAnimation))
        || (InitialTrench!=INDEX_NONE && !Trenches.IsValidIndex(InitialTrench))) return false;
    FString BindingError;
    if(National&&!WarfareFRRUPresentationV82::RoleAssetsReady(this,InRole,Team,BindingError))
    {if(Team==0)LastMessage=BindingError;return false;}
    const FString PreviousAssetFailure=AssetFailure;
    TArray<AWarfareFrontlineSoldier*> CandidateMembers;
    FFrontlineSquad S; S.Id=NextSquadId++; S.Team=Team; S.Role=InRole;
    S.Trench=InitialTrench; S.Destination=InitialTrench>=0?InitialTrench:FirstAvailableTrench(Team);
    S.LastPassedTrench=InitialTrench>=0?InitialTrench:GetHomeTrenchIndex(Team)-GetTeamDirection(Team);
    S.bAdvancing=InitialTrench==INDEX_NONE; S.bReinforcement=S.bAdvancing;
    const int32 Slot=Trenches.IsValidIndex(S.Destination)?FreeSlot(S.Destination,Team):INDEX_NONE;
    S.Slot=Slot>=0?Slot:S.Id%3;
    const int32 SquadId=S.Id; Squads.Add(S);
    auto RejectNationalSpawn=[&]()
    {
        for(auto* Candidate:CandidateMembers)if(Candidate){Units.Remove(Candidate);Candidate->Destroy();}
        Squads.RemoveAll([&](const FFrontlineSquad& Row){return Row.Id==SquadId;});
        // Initialization reports to AssetFailure for the legacy preparation
        // path. A rejected national transaction must not poison later roles.
        AssetFailure=PreviousAssetFailure;if(Team==0)LastMessage=BindingError;
        if(NextSquadId==SquadId+1)NextSquadId=SquadId;return false;
    };
    const int32 Direction=GetTeamDirection(Team);
    const int32 Roster=GetSquadSize(InRole,Team);
    for (int32 I=0;I<Roster;++I)
    {
        const float TailPhase=FMath::Frac((SquadId*17+I*31)*.6180339887f);
        FVector P(InitialTrench>=0?Trenches[InitialTrench].X:FrontlinePacingV24::ReinforcementStartX(Direction,I,TailPhase),(S.Slot-1)*630.f+FormationLaneOffset(Roster,I,0,National),0);
        if(InitialTrench>=0)P.X=FrontlineFieldV32::TrenchCentreX(InitialTrench,P.Y)+(I%2?36.f:-36.f);
        P.Z=FloorZ(P)+88;
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AWarfareFrontlineSoldier* U=GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(P,FRotator(0,GetTeamDirection(Team)>0?0:180,0),Params);
        if (U)
        {
            if(National)CandidateMembers.Add(U);
            U->bFieldEntered=InitialTrench>=0;
            U->Initialize(this,SquadId,Team,InRole,I);
            if(National&&(!U->Soldier||!U->Soldier->GetSkeletalMeshAsset()||!WarfareFRRUPresentationV82::HasBoundWeapon(U)))
            {BindingError=AssetFailure.IsEmpty()?TEXT("The section's equipment could not be prepared."):AssetFailure;return RejectNationalSpawn();}
            if(National)FrontlineFieldV32::InitializeUnit(U);
            U->CommandReadyAt=Elapsed+6.f+U->GetMotionPhase()*2.f;
            P.X+=U->FormationOffset.X; P.Y=(S.Slot-1)*630.f+FormationLaneOffset(Roster,I,U->FormationOffset.Y,National);
            if(InitialTrench>=0)P.X=FrontlineFieldV32::TrenchCentreX(InitialTrench,P.Y)+(I%2?36.f:-36.f)+U->FormationOffset.X;
            if(InitialTrench>=0)P=FrontlineInfantryV45::TrenchSlot(U,InitialTrench,P);
            P.Z=FrontlineInfantryV45::SurfaceHeight(this,P)+88;
            U->SetActorLocation(P);
            if(National&&!WarfareFRRUWorldV82::PlaceOnGround(U,U->Feet()))
            {BindingError=TEXT("The section's footing could not be prepared.");return RejectNationalSpawn();}
            if(!National)Units.Add(U); FindSquad(SquadId)->Members.Add(U);
        }
        else if(National){BindingError=TEXT("The section could not enter the field.");return RejectNationalSpawn();}
    }
    if(National)
    {
        auto* Created=FindSquad(SquadId);
        if(!Created||Created->Members.Num()!=GetSquadSize(InRole,Team))
        {BindingError=TEXT("The section could not be assembled.");return RejectNationalSpawn();}
        for(auto* Candidate:CandidateMembers)Units.Add(Candidate);
        WarfareFRRUWorldV82::Recruited(this,SquadId);
    }
    const bool Accepted=FindSquad(SquadId)&&FindSquad(SquadId)->LivingCount()>0;
    if(Accepted)FrontlineReportV839::RegisterSquad(this,Team,SquadId,int32(InRole),FindSquad(SquadId)->LivingCount());
    return Accepted;
}

bool AWarfareFrontlineGameMode::Recruit(EFrontlineRole InRole,int32 Team)
{
    if(!WarfareFRRUCampaignV82::Active(this))InRole=FrontlineGameplayV52::RecruitVariant(this,InRole,Team);
    const FString Reason=RecruitBlockReason(InRole,Team); if (!Reason.IsEmpty()) { if (Team==0) LastMessage=Reason; return false; }
    if (!SpawnSquad(Team,InRole)) return false;
    FrontlineAchievementsV76::Recruited(this,int32(InRole),Team);if(!WarfareFRRUCampaignV82::Active(this))FrontlineCampaignV52::OnRecruit(this,InRole,Team);
    // Choosing any squad starts the same replenishment cycle for every role.
    // Rifle becomes ready first; waiting preserves the option of Lewis or assault.
    for (int32 RoleIndex=0;RoleIndex<RoleCount;++RoleIndex)
        RecruitReadyAt[Team][RoleIndex]=Elapsed+EffectiveRecruitSeconds(EFrontlineRole(RoleIndex),Team)*
            (Team==1 && GetEncounterCampaignMission()?GetEncounterCampaignMission()->EnemyRecruitScale:1.f)*
            (Team==1?FrontlineDifficultyV30::Doctrine(GetCampaignDifficulty()).RecruitScale:1.f);
    ++RecruitCommands;
    if (Team==0)
    {
        LastMessage=FString::Printf(TEXT("%s · 增援途中"),GetRecruitName(InRole,Team));
    }
    return true;
}

TArray<int32> AWarfareFrontlineGameMode::GetTrenchSquadSlots(int32 Index,int32 Team) const
{
    TArray<int32> Slots; Slots.Init(INDEX_NONE,FrontlineBattlefield::TrenchCapacity);
    if (!ValidTeam(Team) || !Trenches.IsValidIndex(Index)) return Slots;
    for (const auto& S:Squads)
        if (S.Team==Team && S.Trench==Index && S.LivingCount()>0 && Slots.IsValidIndex(S.Slot)) Slots[S.Slot]=S.Id;
    return Slots;
}

FString AWarfareFrontlineGameMode::SquadAdvanceBlockReason(int32 Index,int32 Id,int32 Team) const
{
    if (!ValidTeam(Team) || !Trenches.IsValidIndex(Index) || Trenches[Index].Owner!=Team) return TEXT("请选择己方战壕。");
    if (Phase!=EFrontlinePhase::Fighting) return TEXT("请先开始战斗。");
    if (UGameplayStatics::IsGamePaused(this)) return TEXT("暂停中，请先继续。");
    const auto* S=FindSquad(Id);
    if (!S || S->Team!=Team || S->Trench!=Index || S->LivingCount()==0 || S->bAdvancing) return TEXT("该小队已不在壕内。");
    return FString();
}

bool AWarfareFrontlineGameMode::AdvanceSquad(int32 Index,int32 Id,int32 Team)
{
    const FString Reason=SquadAdvanceBlockReason(Index,Id,Team);
    if (!Reason.IsEmpty()) { if (Team==0) LastMessage=Reason; return false; }
    auto* S=FindSquad(Id);
    const bool bStormSection=S->Members.ContainsByPredicate([](const TWeakObjectPtr<AWarfareFrontlineSoldier>& Member)
        {return Member.IsValid() && Member->IsAlive() && Member->IsStormtrooper();});
    // Their one departure call belongs to the actual crossing, after the men
    // leave the trench. An order-time call would consume that squad's voice slot.
    if (!bStormSection)
        if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
            Audio->PlayVoice(EFrontlineVoiceEvent::Advance,GetFactionForTeam(Team)==EFrontlineFaction::German,S->Centre(),S->Id);
    S->LastPassedTrench=Index;
    S->Destination=ForwardTrenchDestination(*this,Index+GetTeamDirection(Team),Team,Id);
    const int32 Slot=FreeSlot(S->Destination,Team,Id);
    if (Slot!=INDEX_NONE) S->Slot=Slot;
    else S->Slot=FMath::Clamp(S->Slot,0,FrontlineBattlefield::TrenchCapacity-1);
    S->Trench=INDEX_NONE; S->bAdvancing=true; S->bReinforcement=false; S->bHoldPosition=false;
    ++AdvanceCommands;
    if (Team==0) LastMessage=FString::Printf(TEXT("%s · 小队出壕"),GetRecruitName(S->Role,Team));
    return true;
}

bool AWarfareFrontlineGameMode::SetSquadHold(int32 Index,int32 Id,bool bHold,int32 Team)
{
    auto* S=FindSquad(Id);
    if (Phase!=EFrontlinePhase::Fighting || UGameplayStatics::IsGamePaused(this) || !S || !ValidTeam(Team) ||
        !Trenches.IsValidIndex(Index) || Trenches[Index].Owner!=Team || S->Team!=Team || S->Trench!=Index || S->LivingCount()==0) return false;
    S->bHoldPosition=bHold;
    if (Team==0) LastMessage=bHold?TEXT("该小队留守掩护。"):TEXT("已解除该小队的留守命令。");
    return true;
}

bool AWarfareFrontlineGameMode::SetTrenchBypass(int32 Index,bool bBypass,int32 Team)
{
    if (!ValidTeam(Team) || !Trenches.IsValidIndex(Index) || Trenches[Index].Owner!=Team || Phase!=EFrontlinePhase::Fighting
        || UGameplayStatics::IsGamePaused(this)) return false;
    Trenches[Index].bBypass=bBypass;
    if (Team==0) LastMessage=bBypass?TEXT("入壕已关闭 · 后续部队继续向前，驻军留在原位。"):TEXT("入壕已开启 · 后续部队可进入空余席位。");
    return true;
}

bool AWarfareFrontlineGameMode::AdvanceTrench(int32 Index,int32 Team)
{
    const FString Reason=AdvanceBlockReason(Index,Team); if (!Reason.IsEmpty()) { if (Team==0) LastMessage=Reason; return false; }
    auto HasLivingMachinegun=[](const FFrontlineSquad& Squad)
    {
        for (const auto& Member:Squad.Members)
            if (Member.IsValid() && Member->IsAlive() && Member->IsMachineGunner()) return true;
        return false;
    };
    bool HasAssaultingInfantry=false,HasRearguardGun=false;
    if (HasFactionTechnology(EFrontlineFaction::German,8,Team))
        for (const FFrontlineSquad& S:Squads)
            if (S.Team==Team && S.Trench==Index && S.LivingCount()>0)
            {
                if (HasLivingMachinegun(S)) HasRearguardGun=true;
                else if (!S.bHoldPosition) HasAssaultingInfantry=true;
            }
    const bool LeaveMachineguns=HasAssaultingInfantry && HasRearguardGun;
    int32 Moved=0;
    for (FFrontlineSquad& S:Squads) if (S.Team==Team && S.Trench==Index && S.LivingCount()>0)
    {
        if (S.bHoldPosition || (LeaveMachineguns && HasLivingMachinegun(S))) continue;
        if (AdvanceSquad(Index,S.Id,Team)) ++Moved;
    }
    if (Moved==0) return false;
    if (Team==0)
    {
        LastMessage=FString::Printf(TEXT("该壕 %d 支小队出击 · 留守小队继续掩护。"),Moved);
    }
    return true;
}

bool AWarfareFrontlineGameMode::ShouldFinishFriendlyTrenchEntryV69(const AWarfareFrontlineSoldier* U) const
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUWorldV82::ShouldFinishFriendlyPostEntry(U);
    const auto* S=U?FindSquad(U->SquadId):nullptr;
    if(!S||!S->bAdvancing||!Trenches.IsValidIndex(S->Destination)
        ||Trenches[S->Destination].Owner!=S->Team||TrenchOpposed(*this,S->Destination,S->Team)
        ||FreeSlot(S->Destination,S->Team,S->Id)==INDEX_NONE)return false;
    const float Ahead=(FrontlineFieldV32::TrenchCentreX(S->Destination,U->Feet().Y)-U->Feet().X)*GetTeamDirection(S->Team);
    // Finish the last metres into an available friendly bay. Stopping on the
    // parapet to deploy lowers a machine gun's eye behind that very parapet;
    // contact then disappears and the same man is ordered to pack and move.
    if(Ahead < -185.f || Ahead > 430.f)return false;
    for(const AWarfareFrontlineSoldier* Other:Units)
        if(IsValid(Other)&&Other->Team!=U->Team&&Other->IsActiveCombatant()
            &&FVector::DistSquared2D(Other->Feet(),U->Feet())<FMath::Square(600.f))return false;
    return true;
}

FVector AWarfareFrontlineGameMode::SoldierDestination(const AWarfareFrontlineSoldier* U) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Frontline_SoldierDestination);
    FVector NationalDestination;if(WarfareFRRUWorldV82::Destination(U,NationalDestination))return NationalDestination;
    const FFrontlineSquad* S=FindSquad(U->SquadId); if (!S) return U->Feet();
    FVector ReleasedExit;
    if(FrontlineArmourV63::PioneerEgressDestinationV8445(U,ReleasedExit))return ReleasedExit;
    FVector RepairDestination;
    if(FrontlineFieldSitesV59::EngineerDestinationV60(U,RepairDestination))return RepairDestination;
    int32 Index=S->Trench>=0?S->Trench:S->Destination;
    if (S->bAdvancing)
    {
        const int32 Opposed=OpposedTrenchOnRoute(*this,*S);
        if (Opposed!=INDEX_NONE) Index=Opposed;
    }
    float X=Trenches.IsValidIndex(Index)?Trenches[Index].X:GetTeamDirection(S->Team)*(FrontlinePacingV24::CombatHalfLength()+450.f);
    // Advance through the objective line into the rear area. Never halt the
    // first man precisely on an invisible quorum marker; contacts still stop him.
    if (S->bAdvancing && Trenches.IsValidIndex(Index) && TrenchOpposed(*this,Index,S->Team)) X-=GetTeamDirection(S->Team)*410.f;
    // The original roster fixes each man's lane across research, casualties
    // and crew takeovers; current role size must not recenter a moving squad.
    FVector P(X+(U->MemberIndex%2?36.f:-36.f),(S->Slot-1)*630.f+FormationLaneOffset(S->Members.Num(),U->MemberIndex,U->FormationOffset.Y,WarfareFRRUCampaignV82::Active(this)),0);
    P.X+=U->FormationOffset.X;
    if(Trenches.IsValidIndex(Index))P.X+=FrontlineFieldV32::TrenchCentreX(Index,P.Y)-Trenches[Index].X;
    if(Trenches.IsValidIndex(Index)&&!(S->bAdvancing&&TrenchOpposed(*this,Index,S->Team)))
    {
        // A defending rifleman belongs on the actual firing step, not in a
        // random front/back rank on the trench floor with the parapet at eye height.
        if(!S->bAdvancing&&!U->IsSectionLeader())
            P.X=FrontlineFieldV32::TrenchCentreX(Index,P.Y)+GetTeamDirection(U->Team)*125.f;
        P=FrontlineInfantryV45::TrenchSlot(U,Index,P,!S->bAdvancing&&!U->IsSectionLeader());
    }
    FrontlineFieldV32::MineWorkDestinationV8215(U,P);
    P.Z=FrontlineInfantryV45::SurfaceHeight(const_cast<AWarfareFrontlineGameMode*>(this),P); return P;
}

void AWarfareFrontlineGameMode::CaptureTrench(int32 Index,int32 Team)
{
    if (Trenches[Index].Owner==Team) return;
    const int32 Previous=Trenches[Index].Owner; Trenches[Index].Owner=Team; ++TrenchCaptures;
    Trenches[Index].bBypass=false; // A captured entry control belongs to its new garrison.
    FrontlineMoraleV36::OnCapture(this,Index,Previous,Team);
    FrontlineAchievementsV76::EnemyTrenchCaptured(this,Previous,Team);
    // The loss of a manned line is a command event, not an omniscient targeting
    // cue. Counterattack reserves still recruit at home through normal readiness.
    if (Team==0 && Previous==1)
    {
        EnemyCounterattackTrench=Index; EnemyCounterattackUntil=Elapsed+48.f;
        EnemyWaveRecruitCount=0; EnemyWaveStartedAt=Elapsed;
        EnemyRecruitAt=FMath::Min(EnemyRecruitAt,Elapsed+3.f); bEnemyRoleCommitted=false;
        EnemyAssaultAt=FMath::Min(EnemyAssaultAt,Elapsed+4.f);
        UE_LOG(LogTemp,Display,TEXT("V18 enemy counterattack assembled for trench=%d until=%.2f"),Index,EnemyCounterattackUntil);
    }
    else if (Team==1 && Index==EnemyCounterattackTrench)
    { EnemyCounterattackTrench=INDEX_NONE; EnemyCounterattackUntil=0.f; }
    LastMessage=Team==0?TEXT("战壕已占领，可为增援提供掩护。"):TEXT("敌军占据了前方战壕，注意战线压力。");
}

void AWarfareFrontlineGameMode::UpdateSquads()
{
    if(WarfareFRRUWorldV82::UpdateSquads(this))return;
    TRACE_CPUPROFILER_EVENT_SCOPE(Frontline_UpdateSquads);
    for (FFrontlineSquad& S:Squads)
    {
        if (S.LivingCount()==0) { S.Trench=INDEX_NONE; S.bAdvancing=false; continue; }
        if (!S.bAdvancing) continue;
        const int32 Dir=GetTeamDirection(S.Team);
        // Member positions cannot change during this synchronous update. Reuse
        // this local value for the passage, goal and arrival checks, then discard.
        const FVector Centre=S.Centre();
        // Advance this cursor only at an actual trench exit, not at order time.
        // It also prevents a newly vacated trench behind the squad pulling it back.
        for (int32 Passed=S.LastPassedTrench+Dir;Trenches.IsValidIndex(Passed);Passed+=Dir)
        {
            if ((Centre.X-FrontlineFieldV32::TrenchCentreX(Passed,Centre.Y))*Dir<180.f || TrenchOpposed(*this,Passed,S.Team)) break;
            if (Trenches[Passed].Owner==S.Team && (Trenches[Passed].bBypass || FreeSlot(Passed,S.Team,S.Id)==INDEX_NONE))
                if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
                    Audio->PlayVoice(EFrontlineVoiceEvent::PassFullTrench,GetFactionForTeam(S.Team)==EFrontlineFaction::German,Centre,S.Id);
            S.LastPassedTrench=Passed;
        }
        // Follow the assigned hull instead of automatically stopping in the
        // first trench. The passage cursor above remains valid if it is lost.
        if(FrontlineArmourV63::SquadIsEscorting(this,S.Id))continue;
        S.Destination=ForwardTrenchDestination(*this,S.LastPassedTrench+Dir,S.Team,S.Id);
        const int32 Opposed=OpposedTrenchOnRoute(*this,S);
        if (Opposed!=INDEX_NONE) S.Destination=Opposed;
        if (!Trenches.IsValidIndex(S.Destination))
        {
            if (FrontlineGameplayV43::HasCrossedEnemyEnd(this,S))
            {
                FinishBattle(S.Team,S.Team==0?TEXT("我军已突破敌军端线。"):TEXT("敌军突破了我方端线。")); return;
            }
            continue;
        }
        const int32 Index=S.Destination;
        const int32 Slot=FreeSlot(Index,S.Team,S.Id);
        if (Slot!=INDEX_NONE) S.Slot=Slot;
        if (FMath::Abs(Centre.X-FrontlineFieldV32::TrenchCentreX(Index,Centre.Y))>185 || TrenchOpposed(*this,Index,S.Team) || Slot==INDEX_NONE) continue;
        CaptureTrench(Index,S.Team);
        S.Slot=Slot; S.Trench=Index; S.bAdvancing=false; S.bReinforcement=false;
        S.LastPassedTrench=Index;
        if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
            Audio->PlayVoice(EFrontlineVoiceEvent::ArriveTrench,GetFactionForTeam(S.Team)==EFrontlineFaction::German,Centre,S.Id);
    }
}

float AWarfareFrontlineGameMode::TrenchCoverValue(int32 Index) const
{
    if (!Trenches.IsValidIndex(Index)) return 0.f;
    // The central observation position is battered; the two second lines have
    // deeper cuts and more surviving sandbags. Rear trenches retain baseline cover.
    const bool NeutralLine=FrontlineBattlefield::TrenchCount%2==1&&Index==FrontlineBattlefield::CentreTrench;
    return NeutralLine?.66f:(Index>0&&Index<FrontlineBattlefield::TrenchCount-1?.82f:.75f);
}

float AWarfareFrontlineGameMode::CoverFor(const AWarfareFrontlineSoldier* U) const
{
    if(!U)return 0.f;
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUWorldV82::Cover(U);
    const FFrontlineSquad* S=FindSquad(U->SquadId);
    if (S && Trenches.IsValidIndex(S->Trench) && Trenches[S->Trench].Owner==U->Team
        && FMath::Abs(U->Feet().X-FrontlineFieldV32::TrenchCentreX(S->Trench,U->Feet().Y))<140)
    {
        const float Deep=TrenchCoverValue(S->Trench);
        extern bool FrontlineUseLegacyBalanceV8447();
        if(FrontlineUseLegacyBalanceV8447())return Deep;
        // Soldiers standing on the firing step expose head and shoulders.
        // Only a stationary physical reload lowers them back into the cut.
        return FMath::Lerp(FMath::Max(0.f,Deep-.07f),Deep,FrontlineBalanceV50::TrenchReloadShelterBlendV8447(U));
    }
    return 0;
}
bool AWarfareFrontlineGameMode::HasSight(const AWarfareFrontlineSoldier* From,const AWarfareFrontlineSoldier* To) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Frontline_HasSight);
    if(!From || !To || !From->IsActiveCombatant() || !To->IsActiveCombatant())return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FrontlineSight),true);
    for (const AWarfareFrontlineSoldier* U:Units) Params.AddIgnoredActor(U);
    return HasSightWithParamsV834(this,From,To,Params);
}
AWarfareFrontlineSoldier* AWarfareFrontlineGameMode::AcquireTarget(const AWarfareFrontlineSoldier* Unit,bool bAdvanceContact) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Frontline_AcquireTarget);
    if (!Unit || !Unit->IsActiveCombatant()) return nullptr;
    const float Range=(bAdvanceContact?FrontlineWeapons::AdvanceContactRangeFor(Unit):FrontlineWeapons::ProfileFor(Unit).FireRangeCm)*EncounterVisibilityScale();
    const bool CoveredBombApproachV835=bAdvanceContact&&!FrontlineUseLegacyCombatV835()
        &&Unit->Role==EFrontlineRole::Mills&&!Unit->IsStormtrooper();
    const float CoveredBombRangeV835=FrontlineGrenade::TrajectoryReferenceRangeCm*FMath::Min(1.f,EncounterVisibilityScale())*AssaultContactScale(Unit);
    const bool Review=FrontlineTargetReuseV834::Enabled(this);
    if(Review)++FrontlineTargetReuseV834::State().Acquisitions;
    // This invocation owns the ignore list. No visibility results or query
    // parameters survive it; independent HasSight/Pick calls remain fresh.
    TOptional<FCollisionQueryParams> SightParams;
    const auto Sight=[&](const AWarfareFrontlineSoldier* Other)
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(Frontline_HasSight);
        if(Review)++FrontlineTargetReuseV834::State().SightRequests;
        if(!SightParams.IsSet())
        {
            SightParams.Emplace(SCENE_QUERY_STAT(FrontlineSight),true);
            for(const AWarfareFrontlineSoldier* U:Units)SightParams->AddIgnoredActor(U);
            if(Review)++FrontlineTargetReuseV834::State().QueryBuilds;
        }
        return HasSightWithParamsV834(this,Unit,Other,SightParams.GetValue());
    };
    const auto Select=[&](bool Reuse)
    {
    AWarfareFrontlineSoldier* Best=nullptr; float BestScore=MAX_flt;
    AWarfareFrontlineSoldier* Mounted=nullptr;float MountedScore=MAX_flt;
    const auto* Mount=!bAdvanceContact&&Unit->IsMachineGunner()&&!WarfareFRRUCampaignV82::Active(this)
        ?FrontlineMachinegunsV30::Get(Unit):nullptr;
    const bool PreserveSector=Mount&&Mount->IsSupported();
    for (AWarfareFrontlineSoldier* Other:Units) if (Other->Team!=Unit->Team && Other->IsActiveCombatant())
    {
        const float Distance=FVector::Dist2D(Unit->Feet(),Other->Feet());
        const double Along=(Other->Feet().X-Unit->Feet().X)*GetTeamDirection(Unit->Team);
        const bool NearOpposingLine=Distance<=110.f;
        const bool RearDefender=FrontlineGameplayV49::ShareRearSector(this,Unit->Team,Unit->Feet(),Other->Feet());
        if (bAdvanceContact && !NearOpposingLine && Along<0 && !RearDefender && Distance>900.f) continue;
        // A marksman observes automatic weapons first; smoke and physical sight still apply.
        const bool CounterObserver=Unit->IsScopedMarksman() && HasFactionTechnology(EFrontlineFaction::British,13,Unit->Team);
        const float Priority=CounterObserver && Other->IsSectionLeader()?20000.f:
            Unit->Role==EFrontlineRole::Marksman && (Unit->MemberIndex==0||Unit->GetFaction()==EFrontlineFaction::German) && Other->IsMachineGunner()?10000.f:0.f;
        const float Score=(bAdvanceContact?Distance+(Other->bMoving?-250.f:0.f)
            :FrontlineMachinegunsV30::TargetScoreV828(Unit,Other))-Priority;
        const float Concealment=bAdvanceContact && Unit->Role!=EFrontlineRole::Mills && DirectFireMultiplier(Unit->Feet(),Other->Feet())<1.f?.65f:1.f;
        const float RangeDistance=bAdvanceContact?Distance:FrontlineWeapons::DirectFireRangeDistanceCm(Unit,Other->Feet());
        // A bomb team pauses inside its real throwing reach to assault a seen
        // defended trench. Open contacts retain the service-rifle approach.
        // Sight below still uses the actual banks, torso and exposed head.
        const bool CoveredBombContactV835=CoveredBombApproachV835&&Distance<=CoveredBombRangeV835&&CoverFor(Other)>0.f;
        const bool InRange=RangeDistance<=Range*(bAdvanceContact?Concealment*AssaultContactScale(Unit):1.f)||CoveredBombContactV835;
        const bool ValidContact=bAdvanceContact && NearOpposingLine;
        const bool MountedCandidate=PreserveSector&&InRange&&Score<MountedScore
            &&Mount->CoversYaw((Other->Feet()-Unit->Feet()).Rotation().Yaw);
        if((Score<BestScore||MountedCandidate)&&(ValidContact||(InRange&&(Reuse?Sight(Other):HasSight(Unit,Other)))))
        {
            if(Score<BestScore){Best=Other;BestScore=Score;}
            if(MountedCandidate&&Mount->HasInfantryFiringLine(Other)){Mounted=Other;MountedScore=Score;}
        }
    }
    // Keep working the installed, physically clear sector while it has targets.
    // A lower pressure score on the opposite flank does not justify packing
    // every burst. Close defence and a genuinely empty/blocked sector may turn.
    if(Mounted&&(!Best||FVector::DistSquared2D(Unit->Feet(),Best->Feet())>FMath::Square(900.f)))return Mounted;
    return Best;
    };
    AWarfareFrontlineSoldier* Best=Select(true);
    if(Review)
    {
        // Re-run only the original pure base selection in the same frame.
        // Pick and officer notification retain exactly one production call.
        ++FrontlineTargetReuseV834::State().TargetChecks;
        if(Select(false)!=Best)++FrontlineTargetReuseV834::State().TargetFailures;
    }
    if (!bAdvanceContact) Best=FrontlineChoiceTargetsV78::Pick(this,Unit,Best);
    if (Best && Best->IsCommissionedOfficer()) ReportOfficerSpotted(Unit,Best);
    return Best;
}
float AWarfareFrontlineGameMode::BulletHitChance(const AWarfareFrontlineSoldier* Shooter,const AWarfareFrontlineSoldier* Target) const
{
    if (!Shooter || !Target || Shooter->Team==Target->Team || !Shooter->IsActiveCombatant() || !Target->IsActiveCombatant()) return 0.f;
    const auto& Profile=FrontlineWeapons::ProfileFor(Shooter);
    const float Distance=FVector::Dist2D(Shooter->Feet(),Target->Feet());
    if (!FMath::IsFinite(Distance) || FrontlineWeapons::DirectFireRangeDistanceCm(Shooter,Target->Feet())>Profile.FireRangeCm*EncounterVisibilityScale()) return 0.f;
    const float AimedAccuracy=FMath::Clamp(Profile.AccuracyNear-Distance/Profile.AccuracyFalloffCm,Profile.AccuracyFloor,Profile.AccuracyNear);
    const float Accuracy=FMath::Min(.96f,AimedAccuracy*(Shooter->IsAutomaticWeapon() || Shooter->IsCommissionedOfficer()?1.f:RifleAccuracyScale(Shooter->Team)))
        *(1-Shooter->Suppression*.65f)*DirectFireMultiplier(Shooter->Feet(),Target->Feet())*FrontlineBalanceV50::ShooterReadiness(Shooter);
    const float Cover=WarfareFRRUCampaignV82::Active(this)?WarfareFRRUWorldV82::CoverFrom(Target,Shooter->Feet()):CoverFor(Target);
    // Cover, stable firing positions and defensive research affect whether the
    // round reaches the soldier. They never reduce a landed hit's consequence.
    const float Exposure=FrontlineBalanceV50::TargetExposure(Shooter,Target,Cover);
    const float FiringPosition=CoverFor(Shooter)>0.f?1.3f:1.f;
    return FrontlineWeapons::FirearmHitChance(Accuracy*Profile.HitProbabilityScale*Exposure*FiringPosition
        *FrontlineTerrainTacticsV54::DirectFireHitScale(Shooter->Feet(),Target->Feet(),Target->bMoving)
        *IncomingBulletHitChanceScale(Target)*FrontlineFieldSitesV59::PreparedFrontCoverV60(Shooter,Target)
        *FrontlineBalanceV50::CoveringFireHitScaleV8215(Shooter),Shooter->IsAutomaticWeapon());
}

void AWarfareFrontlineGameMode::Fire(AWarfareFrontlineSoldier* Shooter,AWarfareFrontlineSoldier* Target)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Frontline_Fire);
    if (Phase!=EFrontlinePhase::Fighting || !Shooter || !Target || Shooter->Team==Target->Team || !Shooter->IsActiveCombatant() || !Target->IsActiveCombatant()) return;
    if(WarfareFRRUCampaignV82::Active(this)&&!WarfareFRRUWorldV82::FiringLine(Shooter,Target))return;
    // Direct callers obey the same live observation gate as weapon discharge.
    // HasSight retains the trench gun's measured origin and exposed-head fallback.
    if(!WarfareFRRUCampaignV82::Active(this)&&!HasSight(Shooter,Target))return;
    const float Distance=FVector::Dist2D(Shooter->Feet(),Target->Feet()),Cover=WarfareFRRUCampaignV82::Active(this)?WarfareFRRUWorldV82::CoverFrom(Target,Shooter->Feet()):CoverFor(Target);
    const FrontlineWeapons::FProfile& Profile=FrontlineWeapons::ProfileFor(Shooter);
    if (FrontlineWeapons::DirectFireRangeDistanceCm(Shooter,Target->Feet())>Profile.FireRangeCm*EncounterVisibilityScale()) return;
    const bool Support=Shooter->IsAutomaticWeapon();
    if (Support) Shooter->LastAutomaticFireAt=Elapsed;
    const float SmokeFactor=DirectFireMultiplier(Shooter->Feet(),Target->Feet());
    const bool bHit=Random.FRand()<BulletHitChance(Shooter,Target);
    FVector MissImpact=Target->Feet()+FVector(-GetTeamDirection(Shooter->Team)*(75+(ShotsFired%5)*18),((ShotsFired%7)-3)*11,0);
    MissImpact.Z=FloorZ(MissImpact)+8;
    // Occasional visible rounds, with flight time in metres per second.
    // Muzzle light and earth strikes carry the ordinary rifle feedback.
    if (Support && Shooter->ShotsDischarged%3==0)
    {
        const FVector From=Shooter->GetMuzzleLocation(),To=bHit?Target->Soldier->GetBoneLocation(TEXT("spine_03")):MissImpact;
        const float Flight=FVector::Dist(From,To)/74000.f;
        Tracers.Add({From,To,Flight,Flight});
    }
    if (!bHit) Target->ShowBulletImpact(MissImpact);
    Shooter->ShowWeaponDischarge();
    if(ShotsFired==0)UE_LOG(LogTemp,Display,TEXT("WF_V33_FIRST_CONTACT t=%.2f team=%d weapon=%s distance_cm=%.0f"),Elapsed,Shooter->Team,Profile.WeaponId,Distance);
    ++ShotsFired; if (SmokeFactor<1.f) ++SmokeAffectedShots[Shooter->Team];
    const float PressureScale=OutgoingPressureScale(Shooter,Target);
    const float ExposedCrossing=Target->bMoving && Cover<=0.f?1.10f:1.f;
    Target->ReceiveHit(bHit?1.f:0.f,Profile.DirectPressure*PressureScale*(1-Cover*.20f)*ExposedCrossing*SmokeFactor*IncomingBulletPressureScale(Target),Shooter->Team,Target->Feet()-Shooter->Feet(),EFrontlineDamageKind::Bullet,1.f,FFrontlineKillSourceV65::Soldier(Shooter));
    const float CloseAssaultRadius=Shooter->IsStormtrooper() && Cover>0.f && Distance<=1000.f
        && HasFactionTechnology(EFrontlineFaction::German,14,Shooter->Team)?450.f:0.f;
    const bool AreaGun=Shooter->IsMachineGunner();
    const float PressureRadius=FMath::Max(CloseAssaultRadius,FMath::Max(Profile.AdjacentRadiusCm,AdjacentPressureRadius(Shooter)));
    FCollisionQueryParams PressureSight(SCENE_QUERY_STAT(V828AreaPressure),true);
    if(AreaGun)for(const AWarfareFrontlineSoldier* U:Units)if(U)PressureSight.AddIgnoredActor(U);
    for (AWarfareFrontlineSoldier* Other:Units)
        if (Other!=Target && Other->Team==Target->Team && Other->IsActiveCombatant())
        {
            const float Gap=FVector::Dist2D(Other->Feet(),Target->Feet());if(Gap>=PressureRadius)continue;
            float AreaScale=1.f,Visibility=SmokeFactor;
            if(AreaGun)
            {
                if(FrontlineWeapons::DirectFireRangeDistanceCm(Shooter,Other->Feet())>Profile.FireRangeCm*EncounterVisibilityScale())continue;
                const FVector Origin=FrontlineMachinegunsV30::SightOrigin(Shooter),Point=FrontlineMachinegunsV30::SightTarget(Other);
                Visibility=DirectFireMultiplier(Origin,Point);if(Visibility<.9f)continue;
                if(!HasSightWithParamsV834(this,Shooter,Other,PressureSight))continue;
                // A wider beaten zone controls nearby ranks; walls and smoke
                // still block it and deep cover remains an effective counter.
                AreaScale=FMath::Lerp(1.f,.65f,Gap/PressureRadius)*(1.f-CoverFor(Other)*.65f);
            }
            Other->ReceiveHit(0,Profile.AdjacentPressure*(Shooter->IsMachineGunner()?1.30f:1.f)*AreaScale
                *OutgoingPressureScale(Shooter,Other)*Visibility*IncomingBulletPressureScale(Other),Shooter->Team);
        }
    if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
    {
        Audio->PlayWeapon(Support,Shooter->GetFaction()==EFrontlineFaction::German,Shooter->GetMuzzleLocation(),Shooter->SquadId);
        if (Shooter->ShotsDischarged==0) Audio->PlayVoice(Shooter->IsScopedMarksman()?EFrontlineVoiceEvent::SniperReady:
            Shooter->IsStormtrooper()?EFrontlineVoiceEvent::StormContact:EFrontlineVoiceEvent::Contact,
            Shooter->GetFaction()==EFrontlineFaction::German,Shooter->Feet(),Shooter->SquadId);
        if (Shooter->IsScopedMarksman() && Target->IsAlive() && Target->Suppression>.35f)
            Audio->PlayVoice(EFrontlineVoiceEvent::UnderSniper,Target->GetFaction()==EFrontlineFaction::German,Target->Feet(),Target->SquadId);
        const auto* TargetSquad=FindSquad(Target->SquadId);
        if (Target->IsAlive() && Shooter->IsMachineGunner() && Target->Suppression>.45f && TargetSquad && !TargetSquad->bAdvancing && Trenches.IsValidIndex(TargetSquad->Trench))
            Audio->PlayVoice(EFrontlineVoiceEvent::UnderMachinegun,Target->GetFaction()==EFrontlineFaction::German,Target->Feet(),Target->SquadId);
    }
}
void AWarfareFrontlineGameMode::ThrowGrenade(AWarfareFrontlineSoldier* Shooter,AWarfareFrontlineSoldier* Target)
{
    if (Phase!=EFrontlinePhase::Fighting || !Shooter || !Target || !Shooter->IsActiveCombatant() || !Target->IsActiveCombatant()
        || Shooter->Role!=EFrontlineRole::Mills || Shooter->Team==Target->Team || FVector::Dist2D(Shooter->Feet(),Target->Feet())>FrontlineGrenade::MaxRangeCm) return;
    // The convenience API obeys the same preparation and release notify;
    // it cannot bypass personal aiming error or spawn an instant perfect throw.
    Shooter->StartGrenadeThrow(Target);
}

void AWarfareFrontlineGameMode::ThrowGrenadeAt(AWarfareFrontlineSoldier* Shooter,const FVector& Target)
{
    if (Phase!=EFrontlinePhase::Fighting || !Shooter || !Shooter->IsActiveCombatant() || Shooter->Role!=EFrontlineRole::Mills || Target.ContainsNaN()) return;
    FFrontlineStrike Strike; Strike.SourceV65=FFrontlineKillSourceV65::Soldier(Shooter); Strike.Team=Shooter->Team; Strike.Target=Target;
    // The fixed dispersed landing point rests on the same built earthwork
    // surface as infantry. Projecting onto only base terrain buried bombs in
    // real parapets and wrongly screened both their burst and fragments.
    Strike.Target.Z=(!FrontlineUseLegacyCombatV835()&&!WarfareFRRUCampaignV82::Active(this)
        ?FrontlineInfantryV45::SurfaceHeight(this,Target):FloorZ(Target))+3.f;
    Strike.ImpactAt=Elapsed+FrontlineGrenade::FlightSeconds(FVector::Dist2D(Shooter->Feet(),Target));
    Strike.Radius=FrontlineGrenade::BlastRadiusCm; Strike.LaunchedAt=Elapsed; Strike.bGrenade=true;
    Strike.bOrdinaryHandGrenadeV835=!Shooter->IsStormtrooper();
    Strike.Origin=Shooter->GetGrenadeReleaseLocation();
    const TCHAR* Name=Shooter->GetFaction()==EFrontlineFaction::British?TEXT("SM_MillsBomb_V12"):TEXT("SM_StickGrenade_V12");
    auto* Asset=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Characters/V12/Projectiles/%s.%s"),Name,Name));
    if (Asset)
    {
        AActor* Visual=GetWorld()->SpawnActor<AActor>(Strike.Origin,FRotator::ZeroRotator);
        auto* Mesh=NewObject<UStaticMeshComponent>(Visual); Visual->SetRootComponent(Mesh);
        Mesh->SetStaticMesh(Asset); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCastShadow(true);
        Mesh->RegisterComponent(); Visual->SetActorLocation(Strike.Origin); Strike.Projectile=Visual;
    }
    Strikes.Add(Strike); ++GrenadesThrown;
    if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
        Audio->PlayVoice(EFrontlineVoiceEvent::GrenadeThrowWarning,Shooter->GetFaction()==EFrontlineFaction::German,Strike.Origin,Shooter->SquadId);
    WarnFrontlineSoldiers(GetWorld(),EFrontlineVoiceEvent::GrenadeWarning,Target,900.f);
}

bool AWarfareFrontlineGameMode::RequestArtillery(const FVector& Target,int32 Team)
{
    const FString Reason=ArtilleryBlockReason(Team); if (!Reason.IsEmpty()) { if (Team==0) LastMessage=Reason; return false; }
    if (Target.ContainsNaN() || FMath::Abs(Target.X)>FrontlineBattlefield::SupportHalfLength || FMath::Abs(Target.Y)>FrontlineBattlefield::HalfFrontage)
    { if (Team==0) LastMessage=TEXT("请选择战线上有效的炮击落点。"); return false; }
    if(Team==1 && !IsFireSupportAreaSafe(Target,EFrontlineSupport::Artillery,Team))return false;
    const bool Observed=HasFactionTechnology(EFrontlineFaction::British,10,Team) && HasForwardObserver(Target,Team);
    const bool AirRegistered=FrontlineSupportV32::HasRegistrationV37(this,Team,Target);
    const bool Coordinated=HasFactionTechnology(EFrontlineFaction::British,11,Team) && IsInFriendlySmoke(Target,Team);
    const auto Plan=FrontlineFireSupportV26::Plan(false,FrontlineDifficultyV30::SupportSeed(this,Team,191726+ArtilleryCommands*1237+Team*131),Observed,false,Coordinated,AirRegistered,FrontlineCampaignV41::TelephoneResponseRelief(this,Team),FrontlineGameplayV51::HasObservation(this,Team,Target));
    for(const auto& Round:Plan)
    {
        FFrontlineStrike Strike;Strike.SourceV65=FFrontlineKillSourceV65::FireSupport(Team,int32(EFrontlineSupport::Artillery));Strike.Team=Team;Strike.Radius=FrontlineFireSupportV26::FieldGunBlastRadius;Strike.Target=Target+FVector(Round.Offset.X,Round.Offset.Y,0);
        Strike.Target.X=FrontlineFireSupportV26::ReflectInside(Strike.Target.X,FrontlineBattlefield::SupportHalfLength);
        Strike.Target.Y=FrontlineFireSupportV26::ReflectInside(Strike.Target.Y,FrontlineBattlefield::HalfFrontage);
        Strike.Target.Z=FloorZ(Strike.Target);Strike.Origin=Strike.Target+FVector(-GetTeamDirection(Team)*1300,0,1800);
        Strike.ImpactAt=Elapsed+Round.ImpactDelay+(WarfareFRRUCampaignV82::Active(this)?WarfareFRRUEquipmentV82::SupportArrival(this,EFrontlineSupport::Artillery,Team)-Plan[0].ImpactDelay:0.f);Strike.LaunchedAt=Strike.ImpactAt-Round.TerminalFlight;
        Strike.FireMissionId=100000+ArtilleryCommands;Strike.SalvoIndex=Round.Salvo;Strike.RoundIndex=Round.Round;Strike.WarningLeadSeconds=Round.WarningLead;
        Strikes.Add(Strike);
    }
    ArtilleryReadyAt[Team]=Elapsed+EffectiveArtillerySeconds(Team); ++ArtilleryCommands;
    FrontlineReportV839::SupportDelivered(this,Team,int32(EFrontlineSupport::Artillery));
    if (Team==0) LastMessage=AirRegistered?TEXT("Artillery registered by aircraft. Four groups will cover the photographed sector."):TEXT("Artillery ordered. Four groups will cover the selected ground.");
    return true;
}
float AWarfareFrontlineGameMode::ExplosionHitChance(const FFrontlineStrike& Strike,const AWarfareFrontlineSoldier* Target) const
{
    if (!Target || !Target->IsActiveCombatant() || !FMath::IsFinite(Strike.Radius) || Strike.Radius<=0.f) return 0.f;
    const float Distance=FVector::Dist2D(Target->Feet(),Strike.Target);
    if (!FMath::IsFinite(Distance) || Distance>Strike.Radius) return 0.f;
    if(Strike.bShrapnelV64)
    {
        FCollisionQueryParams Q(SCENE_QUERY_STAT(V64ShrapnelScreen),true);
        for(const AWarfareFrontlineSoldier* U:Units)if(U)Q.AddIgnoredActor(U);
        const FVector Torso=FrontlineMachinegunsV30::SightTarget(Target);
        const FVector Head=Target->Soldier?Target->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5):Target->Feet()+FVector(0,0,145);
        if(GetWorld()->LineTraceTestByChannel(Strike.Target,Torso,ECC_Visibility,Q)
            &&GetWorld()->LineTraceTestByChannel(Strike.Target,Head,ECC_Visibility,Q))return 0.f;
        // The descending fan is deadly in open ground. A trench parapet and
        // overhead obstruction screen its shallow balls much more effectively
        // than the pressure wave of a high-explosive shell.
        return FMath::Clamp(.88f*(1-.50f*Distance/Strike.Radius)*(1-CoverFor(Target)*.80f),0.f,.88f);
    }
    if(Strike.bMineV8214)
        return FMath::Clamp(FrontlineMineV8214::HitChanceFalloff(Distance,Strike.Radius)*FrontlineGrenade::BlastExposure(this,Strike.Target,Target),0.f,1.f);
    float Falloff=Strike.bGrenade?FrontlineGrenade::HitChanceFalloff(Distance,Strike.Radius):1-.45f*(Distance/Strike.Radius);
    // A bomb burst inside a trench retains its real fragment-screening rays.
    // Linear radial density prevents an otherwise clear near landing from
    // receiving the squared perimeter penalty as well. Friendly men receive
    // the same profile; the ordinary bomb identity survives its thrower's death.
    if(Strike.bGrenade&&Strike.bOrdinaryHandGrenadeV835&&Strike.SourceV65.Role==int32(EFrontlineRole::Mills)
        &&!FrontlineUseLegacyCombatV835()&&!WarfareFRRUCampaignV82::Active(this)&&CoverFor(Target)>0.f)
        Falloff=1.f-FMath::Clamp((Distance-FrontlineGrenade::FullExposureRadiusCm)
            /FMath::Max(1.f,Strike.Radius-FrontlineGrenade::FullExposureRadiusCm),0.f,1.f);
    const float Exposure=Strike.bGrenade?FrontlineGrenade::BlastExposure(this,Strike.Target,Target):1.f;
    const float AssaultBonus=Strike.bGrenade && Target->Team!=Strike.Team && Target->Suppression>=.55f && CoverFor(Target)>0.f
        && HasFactionTechnology(EFrontlineFaction::German,11,Strike.Team)?1.35f:1.f;
    const float BaseChance=Strike.bGrenade?.82f*GrenadeHitChanceScale(Strike.Team):(Strike.bMortar?.60f:1.25f);
    return FMath::Clamp(BaseChance*Falloff*Exposure*(1-CoverFor(Target)*.30f)*AssaultBonus,0.f,1.f);
}

void AWarfareFrontlineGameMode::ResolveStrike(FFrontlineStrike& Strike)
{
    const bool CommittedSupport=Strike.SourceV65.Support>=0 && Strike.SourceV65.Support<int32(EFrontlineSupport::Count);
    if (Strike.bResolved || !CanResolveCommittedSupportV8217() || (Phase!=EFrontlinePhase::Fighting && !CommittedSupport)) return;
    TGuardValue<bool> SupportResolution(bResolvingCommittedSupportV8217,CommittedSupport);
    Strike.bResolved=true;auto SourceV76=Strike.SourceV65;SourceV76.FireMissionId=Strike.FireMissionId;FrontlineFieldV32::OnBlast(this,Strike);
    if (Strike.Projectile.IsValid()) Strike.Projectile->Destroy();
    auto* Impact=GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Strike.Target,FRotator::ZeroRotator);
    if(Impact)
    {
        if(Strike.bShrapnelV64)Impact->ConfigureAirburstV64(Strike.Radius,Strike.Target-Strike.Origin);
        else if(Strike.bMineV8214)Impact->ConfigureMineV8214(Strike.Radius);
        else Impact->ConfigureBlast(Strike.bGrenade,Strike.bMortar,Strike.bGrenade?FVector::ZeroVector:(Strike.Target-Strike.Origin).GetSafeNormal());
    }
    if (Strike.bMortar&&!Strike.bDirectFire) ++MortarRoundsResolved;
    if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(GetWorld()))
    {
        if(Strike.bShrapnelV64){} // Shrapnel balls cannot penetrate heavy tank armour.
        else if(Strike.bAntiTankRifleGrenadeV836)Vehicles->ApplyAntiTankRifleGrenadeV836(Strike.Target,Strike.Radius,Strike.Team,SourceV76);
        else if(Strike.bGrenade||Strike.bMineV8214)Vehicles->ApplyGrenadeBlast(Strike.Target,Strike.Radius,Strike.Team,SourceV76);
        else Vehicles->ApplyBlast(Strike.Target,Strike.Radius,Strike.ArmorPower>0?Strike.ArmorPower:(Strike.bMortar?230.f:440.f),Strike.Team,SourceV76);
    }
    TArray<FVector> BlastCasualtyFeetV8216;
    for (AWarfareFrontlineSoldier* U:Units) if (U->IsActiveCombatant())
    {
        const float Distance=FVector::Dist2D(U->Feet(),Strike.Target);
        if (FMath::IsFinite(Strike.Radius) && Strike.Radius>0.f && Distance<=Strike.Radius)
        {
            const float Falloff=Strike.bMineV8214?FrontlineMineV8214::HitChanceFalloff(Distance,Strike.Radius):Strike.bGrenade?FrontlineGrenade::HitChanceFalloff(Distance,Strike.Radius):1-.45f*(Distance/Strike.Radius);
            const float Exposure=(Strike.bGrenade||Strike.bMineV8214)?FrontlineGrenade::BlastExposure(this,Strike.Target,U):1.f;
            const bool bHit=Random.FRand()<ExplosionHitChance(Strike,U);
            // A blast can be screened or miss; a struck soldier never survives
            // with fractional health, regardless of distance or friendly fire.
            U->ReceiveHit(bHit?1.f:0.f,Strike.bMineV8214?.95f*Falloff*Exposure:Strike.bShrapnelV64?.62f*Falloff*(1-CoverFor(U)*.50f):Strike.bGrenade?.60f*Falloff*Exposure:(Strike.bMortar?.65f:.90f),Strike.Team,U->Feet()-Strike.Target,(Strike.bGrenade||Strike.bMineV8214)?EFrontlineDamageKind::Grenade:EFrontlineDamageKind::Artillery,Falloff*Exposure*(Strike.bMineV8214?1.1f:Strike.bShrapnelV64?.30f:Strike.bMortar?.65f:1.f),SourceV76);
            if(bHit&&!U->IsAlive())BlastCasualtyFeetV8216.Add(U->Feet());
        }
    }
    if(IsValid(Impact))Impact->SetCasualtyDustV8216(BlastCasualtyFeetV8216);
    if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>()) Audio->PlayImpact(!Strike.bMineV8214&&(Strike.bGrenade || Strike.bMortar || Strike.bShrapnelV64),Strike.Target);
    WarfareFRRUWorldV82::ResolvedStrike(this,Strike);
}
void AWarfareFrontlineGameMode::RegisterCasualty(AWarfareFrontlineSoldier* U,int32 AttackingTeam)
{
    if(!FrontlineMoraleV36::OnCasualty(this,U,AttackingTeam))return;
    ++Casualties[U->Team];
    if (U->IsCommissionedOfficer())
    {
        AWarfareFrontlineSoldier* Witness=nullptr; float Nearest=1200.f;
        FCollisionQueryParams WitnessSight(SCENE_QUERY_STAT(FrontlineOfficerWitness),true);
        for (const AWarfareFrontlineSoldier* Other:Units) WitnessSight.AddIgnoredActor(Other);
        for (AWarfareFrontlineSoldier* Other:Units)
            if (Other!=U && Other->Team==U->Team && Other->IsActiveCombatant())
            {
                const float Distance=FVector::Dist2D(Other->Feet(),U->Feet());
                FHitResult Obstruction;
                if (Distance<Nearest && DirectFireMultiplier(Other->Feet(),U->Feet())>=.95f
                    && !GetWorld()->LineTraceSingleByChannel(Obstruction,Other->Feet()+FVector(0,0,145),U->Feet()+FVector(0,0,135),ECC_Visibility,WitnessSight))
                {Witness=Other;Nearest=Distance;}
            }
        if (Witness) if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
            Audio->PlayVoice(EFrontlineVoiceEvent::OfficerDown,Witness->GetFaction()==EFrontlineFaction::German,Witness->Feet(),Witness->SquadId);
    }
    if (AttackingTeam==0 && U->Team==0 && IsFrontlineExplosiveDamage(U->GetDeathState().Kind)) LastMessage=TEXT("友军遭到爆炸误伤。");
}
void AWarfareFrontlineGameMode::FinishBattle(int32 Winner,const FString& Reason)
{
    if (Phase!=EFrontlinePhase::Fighting) return;
    // The national result commits its earned research inside ResolveOutcome.
    // Freeze the battlefield balance before that transaction, as for UK/DE.
    const int32 PreviousResearchDisplayV87=ResearchBeforeOutcomeV828;
    ResearchBeforeOutcomeV828=GetResearchPoints();
    if(WarfareFRRUCampaignV82::Active(this)&&!WarfareFRRUWorldV82::ResolveOutcome(this,Winner))
    {ResearchBeforeOutcomeV828=PreviousResearchDisplayV87;return;}
    for(int32 I=Strikes.Num()-1;I>=0;--I)if(Strikes[I].SourceV65.Support<0 || Strikes[I].SourceV65.Support>=int32(EFrontlineSupport::Count))
    {if(Strikes[I].Projectile.IsValid())Strikes[I].Projectile->Destroy();Strikes.RemoveAtSwap(I);}
    Phase=Winner==0?EFrontlinePhase::Victory:EFrontlinePhase::Defeat; EndReason=Reason;
    FrontlineFieldV32::BeginOutcome(this,Winner);
    if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>()) Audio->SetBattleActive(false);
    if(!WarfareFRRUCampaignV82::Active(this)){AwardEncounterResearch(Winner);FrontlineAchievementsV76::Finish(this,Winner);}
    else if(WarfareFRRUWorldV82::OutcomePersisted(this))FrontlineAchievementsV76::Finish(this,Winner);
    UGameplayStatics::SetGamePaused(this,false); SetBattleSpeed(1);
    SmokeZones.RemoveAll([](const auto& Zone){return !Zone.bGas;});Tracers.Empty();
    for (AWarfareFrontlineSoldier* U:Units) if (U->IsAlive()) { U->bMoving=false; U->bEngaging=false; U->ReturnToIdle(); }
}

void AWarfareFrontlineGameMode::Tick(float DT)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Frontline_BattleTick);
    if(WarfareFRRUWorldV82::PauseForPendingStory(this))return;
    Super::Tick(DT);FrontlineDebugV37::Tick(this);FrontlineFieldV32::Tick(this,DT);
    if(WarfareFRRUCampaignV82::Active(this)&&Phase!=EFrontlinePhase::Fighting)
    {
        WarfareFRRUWorldV82::Tick(this);
        if(AssetFailure.IsEmpty()&&CanResolveCommittedSupportV8217())TickCommittedSupportV8217();
        return;
    }
    if (Phase!=EFrontlinePhase::Fighting || !AssetFailure.IsEmpty())
    {if(AssetFailure.IsEmpty()&&CanResolveCommittedSupportV8217())TickCommittedSupportV8217();return;}
    Elapsed+=DT;
    FrontlineSpecialistsV41::Tick(this);if(!WarfareFRRUCampaignV82::Active(this))FrontlineGameplayV51::Tick(this,DT);
    for (int32 I=SmokeZones.Num()-1;I>=0;--I) if (Elapsed>=SmokeZones[I].ExpiresAt) SmokeZones.RemoveAtSwap(I);
    for (int32 I=Tracers.Num()-1;I>=0;--I) { Tracers[I].Life-=DT; if (Tracers[I].Life<=0) Tracers.RemoveAtSwap(I); }
    TickCommittedSupportV8217();
    UpdateSquads(); if (Phase!=EFrontlinePhase::Fighting) return;
    UpdateSectionOrders();
    if(WarfareFRRUCampaignV82::Active(this))
    {
        FrontlineMoraleV36::Tick(this,DT);FrontlineAchievementsV76::Tick(this,DT);
        WarfareFRRUWorldV82::Tick(this);
        if(Phase==EFrontlinePhase::Fighting&&Morale[0]<=0)FinishBattle(1,TEXT("The company can no longer hold its orders."));
        return;
    }
    UpdateCampaignObjective(DT); if (Phase!=EFrontlinePhase::Fighting) return;
    FrontlineMoraleV36::Tick(this,DT);FrontlineAchievementsV76::Tick(this,DT);
    if (Morale[0]<=0 || Morale[1]<=0)
    {
        // Simultaneous collapse resolves for the side retaining more living troops.
        const int32 Winner=Morale[0]<=0 && Morale[1]<=0?(LivingSoldiers(0)>LivingSoldiers(1)?0:1):(Morale[1]<=0?0:1);
        FinishBattle(Winner,Winner==0?TEXT("敌军士气瓦解，我军守住了战线。"):TEXT("我军士气瓦解，部队无法继续坚守。")); return;
    }
    TickFrontlineExtendedSupportAI(this);
    if (Elapsed>=EnemyThinkAt) { ThinkEnemy(); EnemyThinkAt=Elapsed+EnemyDecisionRandom.FRandRange(1.1f,2.2f)*FrontlineDifficultyV30::Doctrine(GetCampaignDifficulty()).DecisionScale*FrontlinePacingV24::TempoIntervalScale; }
}
void AWarfareFrontlineGameMode::TickCommittedSupportV8217()
{
    const float Time=CommittedSupportTimeV8217();
    for(int32 I=SmokeZones.Num()-1;I>=0;--I)if(Time>=SmokeZones[I].ExpiresAt)SmokeZones.RemoveAtSwap(I);
    for (int32 I=Strikes.Num()-1;I>=0;--I)
    {
        auto& S=Strikes[I];
        if(!S.bResolved&&!S.bReportRoundRecordedV839&&Time>=S.LaunchedAt&&S.SourceV65.Support>=0&&S.SourceV65.Support<int32(EFrontlineSupport::Count))
        {FrontlineReportV839::SupportRounds(this,S.SourceV65.Team,S.SourceV65.Support);S.bReportRoundRecordedV839=true;}
        if (!S.bResolved && !S.bGrenade && !S.bIncomingSoundPlayed && Time>=S.ImpactAt-S.WarningLeadSeconds)
        {
            S.bIncomingSoundPlayed=true;
            // A warning for each approaching salvo, never for a mission still
            // being prepared; avoid nine stacked copies of the same call.
            if(S.RoundIndex==0)
            {
                if (!S.bMortar) if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>()) Audio->PlayIncomingShell(S.Target);
                WarnFrontlineSoldiers(GetWorld(),EFrontlineVoiceEvent::IncomingArtillery,S.Target,1600.f);
            }
        }
        EnsureFrontlineSupportProjectileV30(this,S);
        // Managed ordnance owns its terminal-flight transform. Writing an old
        // parabola here as well made every frame depend on actor tick order.
        if (!S.bResolved && S.Projectile.IsValid()&&!S.Projectile->IsA<AFrontlineOrdnanceV36>())
        {
            const float Flight=FMath::Max(.01f,S.ImpactAt-S.LaunchedAt), T=FMath::Clamp(Time-S.LaunchedAt,0.f,Flight);
            if (S.bDirectFire)
            {
                S.Projectile->SetActorLocation(FMath::Lerp(S.Origin,S.Target,T/Flight));
                S.Projectile->SetActorRotation((S.Target-S.Origin).Rotation());
            }
            else
            {
                const FVector Velocity=(S.Target-S.Origin)/Flight+FVector(0,0,490.f*Flight);
                S.Projectile->SetActorLocation(S.Origin+Velocity*T-FVector(0,0,490.f*T*T));
                const FVector Tangent=Velocity-FVector(0,0,980.f*T);
                S.Projectile->SetActorRotation(S.bRifleLaunched?FrontlineSpecialistsV41::RifleGrenadeRotation(Tangent,S.bFrenchVBRound)
                    :(S.bGrenade?FRotator(T*320,0,T*95):Tangent.Rotation()).Quaternion());
            }
        }
        if (!Strikes[I].bResolved && Time>=Strikes[I].ImpactAt) ResolveStrike(Strikes[I]);
        if (Strikes[I].bResolved && Time-Strikes[I].ImpactAt>1.3f) Strikes.RemoveAtSwap(I);
    }
}

