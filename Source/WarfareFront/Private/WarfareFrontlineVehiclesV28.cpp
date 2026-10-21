#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineUIAssetsV8213.h"
#include "WarfareFrontlineReportV839.h"
#include "WarfareFrontlineArmourAudioV839.h"
#include "WarfareFrontlineVehicleArtV836.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineArmourWreckV8220.h"
#include "WarfareFRRUArmourV82.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineVisualV85.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontlineGameplayV49.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineTerrainTacticsV54.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineFireV33.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMotionV41.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Dom/JsonObject.h"
#include "Misc/ScopeExit.h"
#include "WarfareFRRUContentV82.h"
#include "WarfareFRRUAssetAuditV82.h"
#include "FRRU/Native/SceneRuntime.h"
#include "StaticMeshResources.h"
#include "Engine/OverlapResult.h"
#include "PhysicsEngine/BodySetup.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include <cfloat>
#include <type_traits>
#include "FRRU/FRRUGermanStaticContactV82.inl"

void FrontlineSetLegacyArmourYieldV84411(bool Legacy);
bool FrontlineUseLegacyArmourYieldV84411();
bool FrontlineReadArmourYieldV84411(const AWarfareFrontlineSoldier* U,FVector& Destination);
TSharedPtr<FJsonObject> FrontlineReadArmourStepV84411(const AWarfareFrontlineSoldier* U);
bool FrontlineArmourYieldDirectPathClearV84411(const AWarfareFrontlineSoldier* U,const FVector& Candidate,const AFrontlineTank* PreviewTank=nullptr,const FTransform* PreviewPose=nullptr,FString* Failure=nullptr);
bool FrontlineCooperativeFloorRouteClearV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,const TArray<FVector>& Path,const FTransform* PreviewPose=nullptr);
bool FrontlineReadHeldArmourLegV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,FVector& Goal,FVector& Leg);
bool FrontlineArmourYieldReachableStationClearV84411(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,const FVector& Point);
bool FrontlineArmourYieldHasCheckedRouteV84411(const AWarfareFrontlineSoldier* U);

namespace
{
bool LegacyNo44V836=false; // Bound only by the strict disposable paired review.
bool LegacyContactTurnV8448=false,LegacyFixedLanesV8448=false,LegacyStandingBodyV8448=false; // Strict disposable counterfactuals only.
bool LegacyContactChoiceV84414=false; // Set only inside the isolated production contact review.
bool LegacyForwardOnlyAcquisitionV85=false; // Strict V85 paired witness, never a gameplay option.
constexpr float TrackWidth=44.f;
float InitialArmourPreparationV836(const AWarfareFrontlineGameMode* B,EFrontlineVehicle Type,int32 Team)
{
    const auto Difficulty=B?B->GetCampaignDifficulty():EFrontlineDifficulty::Rookie;
    const bool National=B&&WarfareFRRUCampaignV82::Active(B);
    const float ModelScale=UFrontlineVehicleSubsystem::Definition(Type).Cooldown/UFrontlineVehicleSubsystem::Definition(EFrontlineVehicle::MarkIVMale).Cooldown;
    const float Readiness=B&&!National?FrontlineArmourV63::ReadinessScale(B,Type,Team):1.f;
    return FrontlineGameplayV43::PreparationSeconds(50.f*ModelScale*Readiness*FrontlinePacingV24::TempoIntervalScale,Team,Difficulty);
}
float MountedInfantryRangeV836(const AWarfareFrontlineGameMode* B,EFrontlineVehicle Type,bool Cannon)
{
    const float Visibility=B?B->EncounterVisibilityScale():1.f;
    const float ChassisRange=UFrontlineVehicleSubsystem::Definition(Type).RangeCm;
    // Keep cannon and the paused national campaign receivers byte-equivalent
    // in meaning. UK/DE mounted MGs cap their own weapon envelope first; weather
    // then shortens it just as it shortens the infantry weapon observing them.
    // Capping after visibility gave long-cannon chassis near-full MG sight in fog.
    if(Cannon||(B&&WarfareFRRUCampaignV82::Active(B)))return FMath::Min(Cannon?6500.f:5100.f,ChassisRange*Visibility);
    return FMath::Min(5100.f,ChassisRange)*Visibility*.9f;
}
float CannonTraverseV837(const AFrontlineTank* V)
{return V&&V->IsRailConvoyV32()?105.f:55.f;}
float ObservedVehicleDistanceV837(const AFrontlineTank* V,const FVector& Point,bool Cannon)
{
    if(!V||!V->IsRailConvoyV32())return V?float(FVector::Dist2D(V->GetActorLocation(),Point)):MAX_flt;
    const int32 Count=Cannon?2:V->GetMachinegunCount();float Best=MAX_flt;
    for(int32 I=0;I<Count;++I)if(!Cannon||V->RailCannonOperationalV837(I))
        Best=FMath::Min(Best,float(FVector::Dist2D(V->WeaponMuzzle(I,Cannon),Point)));
    return Best;
}
bool ArmourHullSeparatedV85(const AFrontlineTank* A,const FTransform& PoseA,const AFrontlineTank* B,const FTransform& PoseB)
{
    const auto* MeshA=A?Cast<UStaticMeshComponent>(A->GetRootComponent()):nullptr;
    const auto* MeshB=B?Cast<UStaticMeshComponent>(B->GetRootComponent()):nullptr;
    if(!MeshA||!MeshB||!MeshA->GetStaticMesh()||!MeshB->GetStaticMesh())return false;
    const FBox BoxA=MeshA->GetStaticMesh()->GetBoundingBox(),BoxB=MeshB->GetStaticMesh()->GetBoundingBox();
    if(!BoxA.IsValid||!BoxB.IsValid)return false;
    const FTransform HullA=MeshA->GetComponentTransform().GetRelativeTransform(A->GetActorTransform())*PoseA;
    const FTransform HullB=MeshB->GetComponentTransform().GetRelativeTransform(B->GetActorTransform())*PoseB;
    const FVector Delta=HullB.TransformPosition(BoxB.GetCenter())-HullA.TransformPosition(BoxA.GetCenter());
    const FVector EA=BoxA.GetExtent(),EB=BoxB.GetExtent();
    const FVector HalfA[]={HullA.TransformVector(FVector(EA.X,0,0)),HullA.TransformVector(FVector(0,EA.Y,0)),HullA.TransformVector(FVector(0,0,EA.Z))};
    const FVector HalfB[]={HullB.TransformVector(FVector(EB.X,0,0)),HullB.TransformVector(FVector(0,EB.Y,0)),HullB.TransformVector(FVector(0,0,EB.Z))};
    FVector Axes[15];int32 Count=0;
    for(const FVector& H:HalfA)Axes[Count++]=H.GetSafeNormal();
    for(const FVector& H:HalfB)Axes[Count++]=H.GetSafeNormal();
    for(const FVector& HA:HalfA)for(const FVector& HB:HalfB)Axes[Count++]=FVector::CrossProduct(HA.GetSafeNormal(),HB.GetSafeNormal()).GetSafeNormal();
    for(const FVector& Axis:Axes)if(!Axis.IsNearlyZero())
    {
        double RadiusA=0,RadiusB=0;
        for(const FVector& H:HalfA)RadiusA+=FMath::Abs(FVector::DotProduct(Axis,H));
        for(const FVector& H:HalfB)RadiusB+=FMath::Abs(FVector::DotProduct(Axis,H));
        if(FMath::Abs(FVector::DotProduct(Delta,Axis))>=RadiusA+RadiusB+.01)return true;
    }
    return false;
}
bool ArmourPivotHullClearV85(const AFrontlineTank* Tank,const FTransform& Candidate)
{
    const auto* Fleet=Tank?UFrontlineVehicleSubsystem::Get(Tank->GetWorld()):nullptr;
    if(!Fleet)return false;
    for(const AFrontlineTank* Other:Fleet->GetVehicles())
        if(IsValid(Other)&&Other!=Tank&&Other->IsOperational()&&!Other->IsHidden()
            &&!ArmourHullSeparatedV85(Tank,Candidate,Other,Other->GetActorTransform()))return false;
    return true;
}
float CrossingLaneCostV55(const FVector& From,float Lane,int32 Direction,
    TFunctionRef<FFrontlineTerrainFeaturesV54(float,float)> FeaturesAt)
{
    // Sample the physical cut ahead of the hull, not just the mud under it.
    // The result ranks lanes; water never becomes an invisible collision wall.
    constexpr float Ahead[]={500.f,1000.f,1500.f,2000.f,2500.f,3000.f};
    float PeakMarsh=0.f,PeakCanal=0.f,MarshSum=0.f,CanalSum=0.f;
    for(float Distance:Ahead)
    {
        const auto F=FeaturesAt(From.X+Direction*Distance,Lane);
        const float Marsh=FMath::Clamp(F.MarshChannel,0.f,1.f);
        const float Canal=FMath::Clamp(F.CanalCut,0.f,1.f);
        PeakMarsh=FMath::Max(PeakMarsh,Marsh);PeakCanal=FMath::Max(PeakCanal,Canal);
        MarshSum+=Marsh;CanalSum+=Canal;
    }
    return 850.f*PeakMarsh+200.f*MarshSum/UE_ARRAY_COUNT(Ahead)
        +2500.f*PeakCanal+350.f*CanalSum/UE_ARRAY_COUNT(Ahead);
}
float VehicleLaneScoreV55(const FVector& From,float Lane,float Preferred,float Desired,int32 Direction,
    bool bRegionalWet,TFunctionRef<FFrontlineTerrainFeaturesV54(float,float)> FeaturesAt)
{
    float Cost=FMath::Abs(Lane-From.Y)*.32f+FMath::Abs(Lane-Preferred)*.08f;
    if(FMath::IsNearlyEqual(Lane,Desired,1.f))Cost-=160.f;
    if(bRegionalWet)Cost+=CrossingLaneCostV55(From,Lane,Direction,FeaturesAt);
    return Cost;
}
const TCHAR* VehicleAssetRootV28=TEXT("/Game/Warfare/Vehicles/V28/");
template<class T> T* Asset(const FString& Name,bool CurrentArt=true)
{
    if constexpr(std::is_same_v<T,UStaticMesh>)
        if(CurrentArt&&Name.StartsWith(TEXT("SM_")))
            if(T* Current=FrontlineVehicleArtV836::ResolveMesh(Name))return Current;
    if constexpr(std::is_same_v<T,UMaterialInterface>)
        if(CurrentArt&&Name.StartsWith(TEXT("M_Vehicle_")))
            if(T* Current=FrontlineVehicleArtV836::ResolveMaterial(Name))return Current;
    if(Name.StartsWith(TEXT("SM_MarkVStar"))||Name.StartsWith(TEXT("SM_Austin"))||Name==TEXT("SM_A7V")||Name==TEXT("SM_A7VGretchen")||Name==TEXT("SM_BeuteMarkIV")||Name==TEXT("SM_BeuteMarkIVFemale"))
    {
        const FString Current=Name.EndsWith(TEXT("_V8218"))?Name:Name+TEXT("_V8218");
        return LoadObject<T>(nullptr,*(TEXT("/Game/Warfare/Vehicles/V8218/")+Current+TEXT(".")+Current));
    }
    if(Name==TEXT("SM_BeuteMarkIV")||Name==TEXT("SM_BeuteMarkIVFemale"))
    {
        const FString Current=Name+TEXT("_V8217");
        return LoadObject<T>(nullptr,*(TEXT("/Game/Warfare/Vehicles/V8217/")+Current+TEXT(".")+Current));
    }
    if(Name==TEXT("SM_MarkVMale")||Name==TEXT("SM_A7VGretchen"))
        return LoadObject<T>(nullptr,*(TEXT("/Game/Warfare/Vehicles/Production/V63/")+Name+TEXT(".")+Name));
    if(Name.Contains(TEXT("Female")))
        return LoadObject<T>(nullptr,*(TEXT("/Game/Warfare/Vehicles/Production/Female/")+Name+TEXT(".")+Name));
    if(Name==TEXT("M_Vehicle_German")||Name==TEXT("M_Vehicle_White")||Name==TEXT("M_Vehicle_Black"))
    {
        const FString Current=Name+TEXT("_V8218");
        return LoadObject<T>(nullptr,*(TEXT("/Game/Warfare/Vehicles/V8218/")+Current+TEXT(".")+Current));
    }
    if(Name.StartsWith(TEXT("M_Vehicle_")))
        if(T* Current=LoadObject<T>(nullptr,*(TEXT("/Game/Warfare/Vehicles/V37/")+Name+TEXT(".")+Name)))return Current;
    // V33 is the production surface rebuild; a missing current mesh must fail visibly.
    if(Name.StartsWith(TEXT("SM_"))||Name.StartsWith(TEXT("M_Vehicle_")))
        return LoadObject<T>(nullptr,*(TEXT("/Game/Warfare/Vehicles/V33/")+Name+TEXT(".")+Name));
    return LoadObject<T>(nullptr,*(FString(VehicleAssetRootV28)+Name+TEXT(".")+Name));
}
bool ValidType(EFrontlineVehicle Type){return uint8(Type)<uint8(EFrontlineVehicle::Count);}
bool Pioneer(EFrontlineVehicle Type){return Type==EFrontlineVehicle::MarkIVPioneer||Type==EFrontlineVehicle::BeutePioneer;}
bool A7VFamily(EFrontlineVehicle Type){return Type==EFrontlineVehicle::A7V||Type==EFrontlineVehicle::A7VGretchen;}
bool Female(EFrontlineVehicle Type){return Type==EFrontlineVehicle::MarkIVFemale||Type==EFrontlineVehicle::BeuteMarkIVFemale||Pioneer(Type);}
bool GermanVehicle(EFrontlineVehicle Type){return A7VFamily(Type)||Type==EFrontlineVehicle::BeuteMarkIV||Type==EFrontlineVehicle::BeuteMarkIVFemale||Type==EFrontlineVehicle::BeutePioneer||Type==EFrontlineVehicle::RailConvoy;}
bool Heavy(EFrontlineVehicle Type){return Type==EFrontlineVehicle::MarkIVMale || Type==EFrontlineVehicle::MarkVMale || Type==EFrontlineVehicle::MarkVStar || Type==EFrontlineVehicle::BeuteMarkIV||Female(Type);}
TArray<FVector2D> RoundTrackCornersV33(const TArray<FVector2D>& In,float Cut,int32 Steps)
{
    TArray<FVector2D> Out;Out.Reserve(In.Num()*(Steps+1));
    for(int32 I=0;I<In.Num();++I)
    {
        const FVector2D P=In[(I+In.Num()-1)%In.Num()],Q=In[I],R=In[(I+1)%In.Num()];
        const FVector2D A=Q+(P-Q).GetSafeNormal()*FMath::Min(double(Cut),(P-Q).Size()*.24);
        const FVector2D B=Q+(R-Q).GetSafeNormal()*FMath::Min(double(Cut),(R-Q).Size()*.24);
        for(int32 J=0;J<=Steps;++J){const double T=double(J)/Steps;Out.Add((1-T)*(1-T)*A+2*T*(1-T)*Q+T*T*B);}
    }
    return Out;
}
FRotator Billboard(UWorld* World)
{
    auto* C=UGameplayStatics::GetPlayerCameraManager(World,0);
    const FRotationMatrix A(C?C->GetCameraRotation():FRotator(-16,-90,0));
    return FRotationMatrix::MakeFromXY(A.GetUnitAxis(EAxis::Y),A.GetUnitAxis(EAxis::Z)).Rotator();
}
FCollisionQueryParams SightParams(UWorld* World,const AActor* Source,const AActor* Target)
{
    FCollisionQueryParams P(SCENE_QUERY_STAT(V28TankSight),true);P.AddIgnoredActor(Source);P.AddIgnoredActor(Target);
    // Soldiers are statistical targets, never opaque sight blockers. The old
    // vehicle-only trace treated a friendly rifle screen as an impenetrable wall.
    if(const auto* B=World?Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode()):nullptr)
        for(const AWarfareFrontlineSoldier* U:B->Units)if(U)P.AddIgnoredActor(U);
    return P;
}
bool ClearSight(UWorld* World,const AActor* Source,const AActor* Target,const FVector& From,const FVector& To,const FCollisionQueryParams* Prepared=nullptr)
{
    if(!World)return false;
    FHitResult Hit;
    if(Prepared)return !World->LineTraceSingleByChannel(Hit,From,To,ECC_Visibility,*Prepared);
    return !World->LineTraceSingleByChannel(Hit,From,To,ECC_Visibility,SightParams(World,Source,Target));
}
bool ClearBombArcV833(const AWarfareFrontlineSoldier* Shooter,const AFrontlineTank* Target,const FVector& Aim,float Flight)
{
    UWorld* World=Shooter?Shooter->GetWorld():nullptr;
    if(!World||!Target||Aim.ContainsNaN()||!FMath::IsFinite(Flight)||Flight<=0.f)return false;
    const FVector From=Shooter->GetMuzzleLocation();
    const FVector Velocity=(Aim-From)/Flight+FVector(0,0,490.f*Flight);
    const FCollisionQueryParams Query=SightParams(World,Shooter,Target);
    // Use the same committed gravity/flight as the rendered projectile. A
    // straight shot at the running gear incorrectly blocked bombs which clear
    // the parapet; the sampled arc still rejects a real wall at the muzzle.
    const int32 Steps=FMath::Clamp(FMath::CeilToInt(FVector::Dist2D(From,Aim)/250.f),8,28);
    FVector Previous=From;
    for(int32 I=1;I<=Steps;++I)
    {
        const float T=Flight*I/Steps;const FVector Next=From+Velocity*T-FVector(0,0,490.f*T*T);
        FHitResult Hit;if(World->LineTraceSingleByChannel(Hit,Previous,Next,ECC_Visibility,Query))return false;
        Previous=Next;
    }
    return true;
}
bool CrewWarning(AFrontlineTank* Tank,EFrontlineVoiceEvent Event,bool EnemyWitness=false)
{
    auto* B=Cast<AWarfareFrontlineGameMode>(Tank->GetWorld()->GetAuthGameMode());if(!B)return false;
    AWarfareFrontlineSoldier* Speaker=nullptr;float Nearest=2200;
    for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsActiveCombatant()&&(EnemyWitness?U->Team!=Tank->Team:U->Team==Tank->Team))
    {const float D=FVector::Dist2D(U->Feet(),Tank->AimPoint());if(D<Nearest&&ClearSight(Tank->GetWorld(),U,Tank,U->GetMuzzleLocation(),Tank->AimPoint())){Nearest=D;Speaker=U;}}
    if(Speaker)if(auto* A=Tank->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())return A->PlayVoice(Event,Speaker->GetFaction()==EFrontlineFaction::German,Speaker->Feet(),Speaker->SquadId);
    return false;
}
}

bool FrontlineUseLegacyNo44V836(){return LegacyNo44V836;}
bool FrontlineUseLegacyStandingBodyV8448(){return LegacyStandingBodyV8448;}

const FFrontlineVehicleEntry& UFrontlineVehicleSubsystem::Definition(EFrontlineVehicle Type)
{
    // Combat values are explicitly game balance; historical weapon counts and silhouettes are separate.
    // V84.4.5: Whippet and Austin are light armour (+30% hull); all other
    // tracked hulls, Pioneer aliases and the armoured train are heavy (+60%).
    // V84.4.12: every playable hull/alias uses 80% of the latest base speed
    // and 125% of the latest hull HP, applied here once before research scales.
    // Module HP and damage-energy coefficients remain independent of hull HP.
    static const FFrontlineVehicleEntry D[]={
        {EFrontlineVehicle::MarkIVMale,TEXT("Mark IV 雄型"),TEXT("两侧六磅炮压住阵位，刘易斯机枪掩护步兵。履带受损后仍可留在原地射击。"),TEXT("MarkIVMale"),0,104,130.f,700.f,6500,2,4},
        {EFrontlineVehicle::Whippet,TEXT("Whippet 快速坦克"),TEXT("固定战斗室内的 Hotchkiss 机枪追击步兵。机动较快，但没有火炮，无法与重装甲正面对射。"),TEXT("Whippet"),0,72,215.6f,357.5f,5100,0,4},
        {EFrontlineVehicle::A7V,TEXT("A7V 突击战车"),TEXT("车首五十七毫米炮与周围机枪守住火线。高大的车身需要步兵照应，履带和枪位可被分别击毁。"),TEXT("A7V"),0,116,145.f,810.f,6600,1,6},
        {EFrontlineVehicle::BeuteMarkIV,TEXT("缴获 Mark IV"),TEXT("修复后披上德军识别标记的英国战车。保留侧炮台与菱形履带，依靠步兵守住身侧。"),TEXT("BeuteMarkIV"),0,108,120.f,630.f,6250,2,4},
        {EFrontlineVehicle::MarkIVFemale,TEXT("Mark IV 雌型"),TEXT("低矮侧舱里的机枪沿壕沟交叉扫射。比雄型更快完成整备，适合伴随步兵压住枪位；没有主炮，无法击穿坦克。"),TEXT("MarkIVFemale"),0,82,138.f,680.f,5100,0,5},
        {EFrontlineVehicle::BeuteMarkIVFemale,TEXT("缴获 Mark IV 雌型"),TEXT("修复车间让缴获的雌型车重回前线。五挺机枪构成移动火墙；维修整备快于火炮型，但必须依靠反坦克炮对付敌方装甲。"),TEXT("BeuteMarkIVFemale"),0,86,125.f,630.f,5100,0,5},
        {EFrontlineVehicle::MarkVMale,TEXT("Mark V 雄型"),TEXT("里卡多发动机与新传动机构改善了操纵。\n双侧炮打击坚固阵位与装甲。机动、防护强于 Mark IV，整备时间更长，侧翼仍需步兵掩护。"),TEXT("MarkVMale"),0,132,165.f,760.f,6500,2,4},
        {EFrontlineVehicle::MarkIVPioneer,TEXT("Mark IV 工兵伴随"),TEXT("雌型坦克掩护两名皇家工兵前进。\n工兵清除铁丝与地雷、抢修履带，阵亡后停止作业。五挺机枪，无主炮；整备最快，机枪点射恢复间隔延长 75%。"),TEXT("MarkIVFemale"),0,66,138.f,680.f,5100,0,5},
        {EFrontlineVehicle::A7VGretchen,TEXT("A7V 501 · Gretchen"),TEXT("A7V 501 的八机枪配置，前部双机枪取代主炮。\n密集火力克制步兵，无法损伤坦克车体。同时仅一辆在役。"),TEXT("A7VGretchen"),0,94,145.f,760.f,4800,0,8},
        {EFrontlineVehicle::BeutePioneer,TEXT("缴获雌型 · 工兵伴随"),TEXT("修复的缴获雌型掩护两名 Pioniere 前进。\n工兵清除铁丝与地雷、抢修履带，阵亡后停止作业。无主炮；整备最快，机枪点射恢复间隔延长 75%。"),TEXT("BeuteMarkIVFemale"),0,70,125.f,630.f,5100,0,5},
        {EFrontlineVehicle::MarkVStar,TEXT("Mark V*"),TEXT("加长的车体横跨宽壕，侧门与后舱枪位遮护车侧。双炮支援突破，转向较慢，整备时间较长。"),TEXT("MarkVStar"),0,142,145.f,840.f,6500,2,4},
        {EFrontlineVehicle::Austin1918,TEXT("Austin 1918"),TEXT("双炮塔维克斯机枪沿道路突进。轮式车机动快、整备短，车体薄，进入泥地与壕沟后速度显著下降。"),TEXT("Austin1918"),0,54,308.f,268.125f,4700,0,2},
        {EFrontlineVehicle::RailConvoy,TEXT("ARMOURED TRAIN"),TEXT("铁路炮车随本次行动临时配属，炮塔与机枪沿铁路掩护步兵。列车运载一支步枪班与工兵，在前方铁路据点下车作战；遭到摧毁将重挫士气。"),TEXT("RailConvoy"),0,145,152.f,2900.f,6500,2,6}};

    return D[ValidType(Type)?int32(Type):0];
}
UFrontlineVehicleSubsystem* UFrontlineVehicleSubsystem::Get(UWorld* World){return World?World->GetSubsystem<UFrontlineVehicleSubsystem>():nullptr;}
TStatId UFrontlineVehicleSubsystem::GetStatId() const {RETURN_QUICK_DECLARE_CYCLE_STAT(UFrontlineVehicleSubsystem,STATGROUP_Tickables);}
bool UFrontlineVehicleSubsystem::DoesSupportWorldType(EWorldType::Type InType)const{return InType==EWorldType::Game || InType==EWorldType::PIE;}
AWarfareFrontlineGameMode* UFrontlineVehicleSubsystem::Battle()const{return GetWorld()?Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode()):nullptr;}
void UFrontlineVehicleSubsystem::ClearEncounter()
{
    FRRUGermanStaticContactV82::ClearIngress(GetWorld());
    WarfareFRRUArmourV82::Reset(this);
    const auto Difficulty=Battle()?Battle()->GetCampaignDifficulty():EFrontlineDifficulty::Rookie;
    for(AFrontlineTank* V:Vehicles)if(IsValid(V))V->Destroy();Vehicles.Empty();NextSerial=0;NextEnemyAt=FrontlineGameplayV43::PreparationSeconds(75.f*FrontlinePacingV24::TempoIntervalScale,1,Difficulty);
    DeploymentCountV37[0]=DeploymentCountV37[1]=0;
    RailReadyAtV75[0]=RailReadyAtV75[1]=42.f;
    for(int32 TeamIndex=0;TeamIndex<2;++TeamIndex)
    {
        ArmourCycleSecondsV40[TeamIndex]=InitialArmourPreparationV836(Battle(),EFrontlineVehicle::MarkIVMale,TeamIndex);
        for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)
        {
            ArmourCycleDurationV84413[TeamIndex][I]=InitialArmourPreparationV836(Battle(),EFrontlineVehicle(I),TeamIndex);
            ReadyAt[TeamIndex][I]=ArmourCycleDurationV84413[TeamIndex][I];
        }
    }
}
void UFrontlineVehicleSubsystem::ResetReadinessForDebug(int32 InTeam)
{
    if(InTeam<0||InTeam>1)return;
    for(float& Time:ReadyAt[InTeam])Time=0;
    for(float& Duration:ArmourCycleDurationV84413[InTeam])Duration=0;
    ArmourCycleSecondsV40[InTeam]=0;
}
void UFrontlineVehicleSubsystem::Deinitialize(){ClearEncounter();Super::Deinitialize();}
TArray<FFrontlineVehicleEntry> UFrontlineVehicleSubsystem::GetRoster(int32 InTeam)const
{
    if(WarfareFRRUCampaignV82::Active(Battle()))
    {
        if(FRRUGermanStaticContactV82::Eligible(Battle(),InTeam,EFrontlineVehicle::A7V))
            return {Definition(EFrontlineVehicle::A7V),Definition(EFrontlineVehicle::BeuteMarkIV)};
        return {};
    }
    const auto* B=Battle();const bool German=B && B->GetFactionForTeam(InTeam)==EFrontlineFaction::German;
    TArray<FFrontlineVehicleEntry> Roster=German?TArray<FFrontlineVehicleEntry>{
        Definition(FrontlineArmourV63::ConfigurationV8218(B,EFrontlineVehicle::A7V,InTeam)),
        Definition(FrontlineArmourV63::ConfigurationV8218(B,EFrontlineVehicle::BeuteMarkIV,InTeam))}:
        TArray<FFrontlineVehicleEntry>{Definition(FrontlineArmourV63::ConfigurationV8218(B,EFrontlineVehicle::MarkIVMale,InTeam)),
        Definition(EFrontlineVehicle::Whippet),Definition(EFrontlineVehicle::MarkVMale),
        Definition(EFrontlineVehicle::MarkVStar),Definition(EFrontlineVehicle::Austin1918)};
    if(German&&RailAvailableV75(InTeam))Roster.Add(Definition(EFrontlineVehicle::RailConvoy));
    return Roster;
}
TArray<FFrontlineVehicleEntry> UFrontlineVehicleSubsystem::GetEquippedRoster(int32 InTeam)const
{
    if(InTeam==1&&FrontlineUltimateArchiveV8215::FullArmory(Battle(),InTeam))
    {
        TArray<FFrontlineVehicleEntry> All;
        for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)
            if(IsUnlocked(EFrontlineVehicle(I),InTeam))All.Add(Definition(EFrontlineVehicle(I)));
        return All;
    }
    const auto R=GetRoster(InTeam);TArray<FFrontlineVehicleEntry> Result;uint32 Eligible=0;
    for(const auto& D:R)if(IsUnlocked(D.Type,InTeam))Eligible|=1u<<uint8(D.Type);
    if(InTeam==0)
    {
        const uint32 Selected=FrontlineArmourV63::GetSelectedMask(Battle(),Eligible);
        for(const auto& D:R)if(Selected&(1u<<uint8(D.Type)))if(Result.Num()<2)Result.Add(D);
    }
    else
    {
        // The opponent receives its own cannon / close-support allocation.
        // It never reads the player's selected armour or clicks.
        const auto* M=Battle()?Battle()->GetEncounterCampaignMission():nullptr;
        const int32 Allocation=M?M->Id:0;TArray<FFrontlineVehicleEntry> Cannon,CloseSupport;
        if(RailAvailableV75(InTeam))for(const auto& D:R)if(D.Type==EFrontlineVehicle::RailConvoy&&(Eligible&(1u<<uint8(D.Type))))Result.Add(D);
        for(const auto& D:R)if(Eligible&(1u<<uint8(D.Type)))
        {if(D.Type==EFrontlineVehicle::RailConvoy)continue;if(D.CannonCount>0)Cannon.Add(D);else CloseSupport.Add(D);}
        if(!Cannon.IsEmpty()&&Result.Num()<2)Result.Add(Cannon[Allocation%Cannon.Num()]);
        if(!CloseSupport.IsEmpty()&&Result.Num()<2)Result.Add(CloseSupport[(Allocation/2)%CloseSupport.Num()]);
        for(const auto& D:R)if(Result.Num()<2&&(Eligible&(1u<<uint8(D.Type)))&&!Result.ContainsByPredicate([&](const auto& E){return E.Type==D.Type;}))Result.Add(D);
    }
    return Result;
}
UTexture2D* UFrontlineVehicleSubsystem::GetPortrait(EFrontlineVehicle InType)const
{
    if(!ValidType(InType))return nullptr;
    if(InType==EFrontlineVehicle::RailConvoy)
    {
        return FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V837/T_Armour_RailConvoy_V837.T_Armour_RailConvoy_V837"));
    }
    if(InType==EFrontlineVehicle::MarkVStar||InType==EFrontlineVehicle::Austin1918)
    {
        const FString Name=FString(TEXT("T_Armour_"))+Definition(InType).AssetKey+TEXT("_V8218");
        return FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V8218/")+Name+TEXT(".")+Name);
    }
    if(uint8(InType)>=6)
    {
        const TCHAR* Keys[]={TEXT("MarkVMale"),TEXT("MarkIVPioneer"),TEXT("A7VGretchen"),TEXT("BeutePioneer")};
        const FString Name=FString(TEXT("T_Armour_"))+Keys[uint8(InType)-6];
        return FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V63/")+Name+TEXT(".")+Name);
    }
    const FString Name=TEXT("T_Portrait_")+FString(Definition(InType).AssetKey);
    const FString Folder=Name.Contains(TEXT("Female"))?TEXT("/Game/Warfare/Vehicles/Production/Female/"):TEXT("/Game/Warfare/Vehicles/V28/");
    return FrontlineUIAssetsV8213::Texture(Folder+Name+TEXT(".")+Name);
}
int32 UFrontlineVehicleSubsystem::UnlockMission(EFrontlineVehicle InType,int32 InTeam)const
{
    const auto* B=Battle();if(!B||!ValidType(InType)||InTeam<0||InTeam>1)return INDEX_NONE;
    const bool German=B->GetFactionForTeam(InTeam)==EFrontlineFaction::German;
    if(German!=GermanVehicle(InType))return INDEX_NONE;
    if(InType==EFrontlineVehicle::RailConvoy){const auto* M=B->Phase==EFrontlinePhase::Briefing?B->GetSelectedCampaignMission():B->GetEncounterCampaignMission();return M&&RailAvailableV75(InTeam)?M->Id:INDEX_NONE;}
    // Company allocations are distinct from a design's first combat date.
    // British female tanks follow the first Cambrai allocation; captured
    // females join the 1918 reserve, after their documented German use begins.
    if(InType==EFrontlineVehicle::MarkVStar)return 16;
    if(InType==EFrontlineVehicle::Austin1918)return FrontlineCampaignV41::RemapLegacyMission(5);
    if(InType==EFrontlineVehicle::MarkVMale)return FrontlineCampaignV41::RemapLegacyMission(5);
    if(InType==EFrontlineVehicle::A7VGretchen)return FrontlineCampaignV41::RemapLegacyMission(4)+1;
    if(InType==EFrontlineVehicle::MarkIVPioneer)return FrontlineCampaignV41::RemapLegacyMission(3)+1;
    if(InType==EFrontlineVehicle::BeutePioneer)return FrontlineCampaignV41::RemapLegacyMission(5)+1;
    if(InType==EFrontlineVehicle::MarkIVFemale)return FrontlineCampaignV41::RemapLegacyMission(3)+1;
    if(InType==EFrontlineVehicle::BeuteMarkIVFemale)return FrontlineCampaignV41::RemapLegacyMission(5)+1;
    return FrontlineCampaignV41::RemapLegacyMission(InType==EFrontlineVehicle::MarkIVMale?3:A7VFamily(InType)?4:5);
}
bool UFrontlineVehicleSubsystem::IsUnlocked(EFrontlineVehicle InType,int32 InTeam)const
{
    const auto* B=Battle();if(!B || !ValidType(InType) || InTeam<0 || InTeam>1)return false;
    if(WarfareFRRUCampaignV82::Active(B))return FRRUGermanStaticContactV82::Eligible(B,InTeam,InType);
    if(InType==EFrontlineVehicle::RailConvoy)return RailAvailableV75(InTeam);
    const bool German=B->GetFactionForTeam(InTeam)==EFrontlineFaction::German;
    if(German!=GermanVehicle(InType)||Pioneer(InType))return false;
    if(FrontlineUltimateArchiveV8215::FullArmory(B,InTeam))return true;
    const auto* M=B->GetEncounterCampaignMission();if(!M)M=B->GetSelectedCampaignMission();if(!M||!M->bArmourAllowedV41)return false;
    if((InType==EFrontlineVehicle::MarkVMale||InType==EFrontlineVehicle::MarkVStar||InType==EFrontlineVehicle::A7VGretchen)&&!FrontlineArmourV63::Has(B,5,InTeam))return false;
    if(InType==EFrontlineVehicle::MarkVStar)return M->CalendarDateV41>=19180808&&M->Id>=UnlockMission(InType,InTeam);
    if(InType==EFrontlineVehicle::Austin1918)return M->CalendarDateV41>=19180808&&M->Id>=UnlockMission(InType,InTeam);
    if(InType==EFrontlineVehicle::MarkVMale)return M->CalendarDateV41>=19180704&&M->Id>=UnlockMission(InType,InTeam);
    if(InType==EFrontlineVehicle::A7VGretchen)return M->CalendarDateV41>=19180424&&M->Id>=UnlockMission(InType,InTeam);
    if(InType==EFrontlineVehicle::MarkIVMale||InType==EFrontlineVehicle::MarkIVFemale||InType==EFrontlineVehicle::MarkIVPioneer)return M->Year>1917 || M->Id>=UnlockMission(InType,InTeam);
    // This campaign's March action is 21 March, before Whippet's 26 March debut.
    if(InType==EFrontlineVehicle::Whippet)return M->Year>=1918 && M->Id>=UnlockMission(InType,InTeam);
    // Later availability is this company's next allocation of repaired captured
    // tanks, not an assertion about when the historical design was invented.
    return M->Year>=1918 && M->Id>=UnlockMission(InType,InTeam);
}
bool UFrontlineVehicleSubsystem::IsDeploymentEligibleV837(EFrontlineVehicle InType,int32 InTeam)const
{
    const auto* B=Battle();if(!B||InTeam<0||InTeam>1||!ValidType(InType)||!IsUnlocked(InType,InTeam))return false;
    if(WarfareFRRUCampaignV82::Active(B))return FRRUGermanStaticContactV82::Eligible(B,InTeam,InType);
    if(InTeam==0&&!FrontlineArmourV63::IsSelected(B,InType))return false;
    if(InType==EFrontlineVehicle::RailConvoy||InType==EFrontlineVehicle::A7VGretchen)
        for(const AFrontlineTank* V:Vehicles)if(IsValid(V)&&V->Team==InTeam&&V->IsOperational()
            &&(InType==EFrontlineVehicle::RailConvoy?V->IsRailConvoyV32():V->Type==InType))return false;
    return true;
}
FString UFrontlineVehicleSubsystem::RecruitBlockReason(EFrontlineVehicle InType,int32 InTeam)const
{
    const auto* B=Battle();if(!B || !ValidType(InType) || InTeam<0 || InTeam>1)return TEXT("编制不可用");
    if(WarfareFRRUCampaignV82::Active(B))
    {
        if(!FRRUGermanStaticContactV82::Eligible(B,InTeam,InType))return TEXT("german_review_date_roster_unavailable");
        if(B->BattleOutroWinner!=INDEX_NONE||B->Phase!=EFrontlinePhase::Fighting)return TEXT("german_battle_not_fighting");
        if(!B->AssetFailure.IsEmpty())return B->AssetFailure;
        if(UGameplayStatics::IsGamePaused(GetWorld()))return TEXT("german_battle_paused");
        if(B->Elapsed<ReadyAt[InTeam][int32(InType)])return TEXT("german_shared_armour_reserve_preparing");
        FString Error;if(!FRRUGermanStaticContactV82::AssetsReady(InType,Error))return Error;
        return {};
    }
    if(B->BattleOutroWinner!=INDEX_NONE)return TEXT("战斗已经结束，车组原地待命");
    if(InType==EFrontlineVehicle::RailConvoy)
    {
        if(!RailAvailableV75(InTeam))return FrontlineArmourV63::L(TEXT("No rail detachment assigned."),TEXT("本次行动未配属铁路分遣队。"));
        for(const AFrontlineTank* V:Vehicles)if(IsValid(V)&&V->Team==InTeam&&V->IsRailConvoyV32()&&V->IsOperational())return FrontlineArmourV63::L(TEXT("The rail convoy is already in action."),TEXT("装甲列车已在行动。"));
    }
    if(Pioneer(InType))return FrontlineArmourV63::L(TEXT("Engineers now accompany researched tank allocations."),TEXT("随车工兵已并入装甲配属研究。"));
    if((InType==EFrontlineVehicle::MarkVStar||InType==EFrontlineVehicle::Austin1918)&&!IsUnlocked(InType,InTeam))
        return FrontlineArmourV63::L(TEXT("Allocated from the August offensive."),TEXT("八月攻势起配属。"));
    if((InType==EFrontlineVehicle::MarkVMale||InType==EFrontlineVehicle::MarkVStar||InType==EFrontlineVehicle::A7VGretchen)&&!FrontlineArmourV63::Has(B,5,InTeam))return FrontlineArmourV63::L(TEXT("Research the specialist tank allocation first."),TEXT("先筹办专门车组配属。"));
    if(InType==EFrontlineVehicle::A7VGretchen)for(const AFrontlineTank* V:Vehicles)if(IsValid(V)&&V->Team==InTeam&&V->Type==InType&&V->IsOperational())return FrontlineArmourV63::L(TEXT("Gretchen is already in the field."),TEXT("Gretchen 已在战场。"));
    if(Female(InType)&&!IsUnlocked(InType,InTeam))return TEXT("等待雌型坦克配属");
    if(InType==EFrontlineVehicle::MarkVMale&&!IsUnlocked(InType,InTeam))return FrontlineArmourV63::L(TEXT("Allocated for the August offensive."),TEXT("八月攻势时配属。"));
    if(InType==EFrontlineVehicle::A7VGretchen&&!IsUnlocked(InType,InTeam))return FrontlineArmourV63::L(TEXT("Available with the April tank detachment."),TEXT("四月战车分遣队抵达后配属。"));
    if(!IsUnlocked(InType,InTeam))return InType==EFrontlineVehicle::Whippet||InType==EFrontlineVehicle::BeuteMarkIV?TEXT("亚眠行动时配属"):
        InType==EFrontlineVehicle::MarkIVMale?TEXT("康布雷行动时配属"):TEXT("春季攻势时配属");
    if(InTeam==0&&!FrontlineArmourV63::IsSelected(B,InType))return FrontlineArmourV63::L(TEXT("Not assigned to this operation."),TEXT("本次出战未配属此装甲。"));
    if(B->Phase!=EFrontlinePhase::Fighting)return TEXT("出发后呼叫装甲支援");
    if(!B->AssetFailure.IsEmpty())return B->AssetFailure;
    if(UGameplayStatics::IsGamePaused(GetWorld()))return TEXT("暂停中，请先继续");
    if(B->Elapsed<ReadyAt[InTeam][int32(InType)])return TEXT("装甲预备队正在整备");
    return FString();
}
float UFrontlineVehicleSubsystem::CooldownFraction(EFrontlineVehicle InType,int32 InTeam)const
{
    const auto* B=Battle();if(!B||!ValidType(InType)||InTeam<0||InTeam>1)return 1;
    // A dispatch starts one workshop cycle for both slots. Each model has its
    // own completion time, frozen with the research and difficulty at dispatch.
    const float Duration=ArmourCycleDurationV84413[InTeam][int32(InType)];
    const float At=ReadyAt[InTeam][int32(InType)];
    return FMath::Clamp((At-B->Elapsed)/FMath::Max(1.f,Duration),0.f,1.f);
}
AFrontlineTank* UFrontlineVehicleSubsystem::Spawn(EFrontlineVehicle InType,int32 InTeam,bool bAtEntry)
{
    auto* B=Battle();if(!B||InTeam<0||InTeam>1)return nullptr;
    const int32 Dir=B->GetTeamDirection(InTeam);
    float X=bAtEntry?FrontlinePacingV24::ReinforcementStartX(Dir,0,.5f):B->Trenches[B->GetHomeTrenchIndex(InTeam)].X;
    // Separate army counters retain all four lanes with interleaved orders.
    static constexpr float Lanes[4]={-300,900,300,-900};
    const bool Rail=InType==EFrontlineVehicle::RailConvoy;
    if(Rail&&!RailAvailableV75(InTeam))return nullptr;
    const float Y=Rail?-1130.f:Lanes[DeploymentCountV37[InTeam]%4]*(InTeam==0?1.f:-1.f);
    auto* V=GetWorld()->SpawnActor<AFrontlineTank>(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)),FRotator(0,Dir>0?0:180,0));
    if(!V || !V->Configure(B,Rail?EFrontlineVehicle::A7V:InType,InTeam,++NextSerial)){if(V)V->Destroy();return nullptr;}
    if(Rail){if(!V->ConfigureRailV32()){V->Destroy();return nullptr;}V->Type=EFrontlineVehicle::RailConvoy;}
    if(bAtEntry)
    {
        // Time the first visible hull, not its centre. The vehicle keeps its
        // real speed; slow heavy tanks no longer inherit a long infantry road.
        X=-Dir*(FrontlinePacingV24::CombatHalfLength()+V->FootprintHalfLengthV37()+Definition(InType).SpeedCm*(V->bNationalGermanGroundV82||Rail?1.f:FrontlineArmourV63::TravelSpeedScaleV8218(B,InTeam))*2.8f);
        for(const AFrontlineTank* Existing:Vehicles)if(IsValid(Existing)&&Existing->IsOperational()&&FMath::Abs(Existing->GetActorLocation().Y-Y)<470.f)
        {
            const float Clearance=V->FootprintHalfLengthV37()+Existing->FootprintHalfLengthV37()+100.f;
            if(FMath::Abs(Existing->GetActorLocation().X-X)<Clearance)X=Existing->GetActorLocation().X-Dir*Clearance;
        }
        V->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)));
    }
    ++DeploymentCountV37[InTeam];V->PreferredLaneV37=V->DesiredLaneV37=Y;
    if(V->bNationalGermanGroundV82)
    {
        if(!V->CommitNationalGroundV82(V->GetActorLocation(),V->HeadingYawV37,0,true)){--DeploymentCountV37[InTeam];V->Destroy();return nullptr;}
    }
    else V->FitGroundV35(V->GetActorLocation(),0);
    Vehicles.Add(V);return V;
}
bool UFrontlineVehicleSubsystem::Recruit(EFrontlineVehicle InType,int32 InTeam)
{
    auto* B=Battle();if(!B)return false;
    const bool National=WarfareFRRUCampaignV82::Active(B);
    if(National&&!FRRUGermanStaticContactV82::Eligible(B,InTeam,InType))return false;
    if(LastEncounter!=B->GetEncounterId()){ClearEncounter();LastEncounter=B->GetEncounterId();}
    const FString Reason=RecruitBlockReason(InType,InTeam);if(!Reason.IsEmpty()){if(InTeam==0)B->LastMessage=Reason;return false;}
    AFrontlineTank* Deployed=Spawn(InType,InTeam,true);
    if(!Deployed){B->LastMessage=TEXT("战车尚未完成检修");return false;}
    if(!National&&InType!=EFrontlineVehicle::Whippet&&InType!=EFrontlineVehicle::Austin1918&&InType!=EFrontlineVehicle::RailConvoy&&FrontlineArmourV63::Has(B,4,InTeam))FrontlineArmourV63::SpawnPioneerEscort(B,Deployed);
    const auto& D=Definition(InType);
    ArmourCycleSecondsV40[InTeam]=FrontlineGameplayV43::PreparationSeconds(D.Cooldown*FrontlinePacingV24::TempoIntervalScale*(National?1.f:FrontlineArmourV63::ReadinessScale(B,InType,InTeam)),InTeam,B->GetCampaignDifficulty());
    for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)
    {
        const auto Type=EFrontlineVehicle(I);
        const float Duration=FrontlineGameplayV43::PreparationSeconds(
            Definition(Type).Cooldown*FrontlinePacingV24::TempoIntervalScale*(National?1.f:FrontlineArmourV63::ReadinessScale(B,Type,InTeam)),InTeam,B->GetCampaignDifficulty());
        ArmourCycleDurationV84413[InTeam][I]=Duration;
        ReadyAt[InTeam][I]=B->Elapsed+Duration;
    }
    if(InTeam==0)B->LastMessage=FString::Printf(TEXT("%s 正沿后方通路进入"),D.Name);
    UE_LOG(LogTemp,Display,TEXT("WF_V28_TANK_RECRUIT type=%d team=%d ready_at=%.2f"),int32(InType),InTeam,ReadyAt[InTeam][int32(InType)]);return true;
}
#include "WarfareFrontlineArchiveArmourDecisionV85.inl"
void UFrontlineVehicleSubsystem::Tick(float DeltaTime)
{
    auto* B=Battle();if(!B)return;
    if(LastEncounter!=B->GetEncounterId()){ClearEncounter();LastEncounter=B->GetEncounterId();}
    if(B->Phase!=EFrontlinePhase::Fighting || UGameplayStatics::IsGamePaused(GetWorld()))return;
    // World delta already includes battle speed. Bound each physical step,
    // while consuming the whole frame for travel, weapon clocks and shells.
    if(!FMath::IsFinite(DeltaTime)||DeltaTime<=0.f)return;
    const int32 Substeps=FMath::Max(1,FMath::CeilToInt(DeltaTime/.1f));
    const float DT=DeltaTime/Substeps;
    for(int32 Step=0;Step<Substeps;++Step)
        for(AFrontlineTank* V:Vehicles)if(IsValid(V))V->Simulate(DT);
    Vehicles.RemoveAll([](const TObjectPtr<AFrontlineTank>& V){return !IsValid(V);});
    // The seven-second surrender/celebration still uses Fighting to animate
    // the field. Vehicle cosmetics continue, but neither army receives armour.
    if(B->BattleOutroWinner!=INDEX_NONE)return;
    if(WarfareFRRUCampaignV82::Active(B)&&!FRRUGermanStaticContactV82::Eligible(B,1,EFrontlineVehicle::A7V))
    {WarfareFRRUArmourV82::TickEnemy(this);return;}
    if(B->Elapsed>=NextEnemyAt)
    {
        NextEnemyAt=B->Elapsed+35.f*FrontlinePacingV24::TempoIntervalScale;
        auto R=GetEquippedRoster(1);
        // Rotate the full roster; new attachments must not be permanently
        // starved behind two older models when their shared queue opens.
        const int32 RailIndex=R.IndexOfByPredicate([](const FFrontlineVehicleEntry& E){return E.Type==EFrontlineVehicle::RailConvoy;});
        int32 First=RailIndex!=INDEX_NONE&&DeploymentCountV37[1]==0?RailIndex:R.IsEmpty()?0:NextSerial%R.Num();
        // The rail action has a concrete counter: cannon armour with infantry
        // on its inner shoulder. Higher difficulties make the choice from
        // genuinely seen hulls, not the player's selection/order or reserves.
        const bool FullArchive=FrontlineUltimateArchiveV8215::FullArmory(B,1);
        bool SeenArmour=false;int32 SeenInfantry=0;
        if(!WarfareFRRUCampaignV82::Active(B)&&B->GetEncounterCampaignMission()
            &&(FullArchive||B->GetEncounterCampaignMission()->bArmoredTrain)&&(RailIndex==INDEX_NONE||DeploymentCountV37[1]>0)
            &&B->GetCampaignDifficulty()!=EFrontlineDifficulty::Rookie)
        {
            for(const AWarfareFrontlineSoldier* Observer:B->Units)if(IsValid(Observer)&&Observer->Team==1&&Observer->IsActiveCombatant())
            {
                const FVector Eye=Observer->Feet()+FVector(0,0,145);
                for(const AFrontlineTank* Target:Vehicles)if(IsValid(Target)&&Target->Team==0&&Target->IsOperational()&&Target->HasEnteredField()
                    &&ObservedVehicleDistanceV837(Target,Eye,false)<6500.f*B->EncounterVisibilityScale()
                    &&B->DirectFireMultiplier(Eye,Target->AimPoint())>=.9f
                    &&ClearSight(GetWorld(),Observer,Target,Eye,Target->AimPoint()))SeenArmour=true;
                if(B->GetCampaignDifficulty()==EFrontlineDifficulty::Survivor)
                    for(const AWarfareFrontlineSoldier* Target:B->Units)if(IsValid(Target)&&Target->Team==0&&Target->IsActiveCombatant()
                        &&FVector::Dist2D(Eye,Target->Feet())<4200.f*B->EncounterVisibilityScale()
                        &&B->DirectFireMultiplier(Eye,Target->Feet()+FVector(0,0,145))>=.9f
                        &&ClearSight(GetWorld(),Observer,Target,Eye,Target->Feet()+FVector(0,0,145)))++SeenInfantry;
                if(SeenArmour||SeenInfantry>=6)break;
            }
            if(SeenArmour||SeenInfantry>=6)
                for(int32 I=0;I<R.Num();++I)
                    if(RecruitBlockReason(R[I].Type,1).IsEmpty()
                        &&(SeenArmour?R[I].CannonCount>0:R[I].MachinegunCount>=5))
                    {First=I;break;}
        }
        if(FullArchive&&!WarfareFRRUCampaignV82::Active(B))
        {
            const int32 Reserve=ArchiveArmourReserveV85(R,DeploymentCountV37[1],SeenArmour,
                [&](EFrontlineVehicle Type){return IsDeploymentEligibleV837(Type,1);});
            if(R.IsValidIndex(Reserve))
            {
                const auto Type=R[Reserve].Type;
                if(RecruitBlockReason(Type,1).IsEmpty())Recruit(Type,1);
                const int32 Next=ArchiveArmourReserveV85(R,DeploymentCountV37[1],SeenArmour,
                    [&](EFrontlineVehicle Candidate){return IsDeploymentEligibleV837(Candidate,1);});
                NextEnemyAt=R.IsValidIndex(Next)?FMath::Max(B->Elapsed+.5f,ReadyAt[1][int32(R[Next].Type)]):B->Elapsed+5.f;
            }
            return;
        }
        for(int32 I=0;I<R.Num();++I)
        {
            const auto& D=R[(First+I)%R.Num()];const FString Reason=RecruitBlockReason(D.Type,1);
#if WITH_EDITOR
            if(WarfareFRRUCampaignV82::Active(B)&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82GermanHostArmourReview")))
                UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_GERMAN_AI_PREFLIGHT host=%.3f type=%d reason=%s"),B->Elapsed,int32(D.Type),*Reason);
#endif
            if(Reason.IsEmpty())
            {
                const bool Recruited=Recruit(D.Type,1);
#if WITH_EDITOR
                if(WarfareFRRUCampaignV82::Active(B)&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82GermanHostArmourReview")))
                    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_GERMAN_AI_RECRUIT host=%.3f type=%d accepted=%d"),B->Elapsed,int32(D.Type),Recruited);
#endif
                if(Recruited)
                {
                    NextEnemyAt=ReadyAt[1][WarfareFRRUCampaignV82::Active(B)?0:int32(D.Type)];
                    if(!WarfareFRRUCampaignV82::Active(B))for(const auto& Available:R)
                        NextEnemyAt=FMath::Min(NextEnemyAt,ReadyAt[1][int32(Available.Type)]);
                }
                break;
            }
        }
    }
}
#include "WarfareFrontlineReadinessChecksV84413.inl"
#include "WarfareFrontlineArmourBlastBalanceV844.inl"

void UFrontlineVehicleSubsystem::ApplyBlast(const FVector& Position,float Radius,float Power,int32 AttackingTeam,const FFrontlineKillSourceV65& SourceV65)
{
    auto Credit=SourceV65;Credit.Team=AttackingTeam;
    if(Position.ContainsNaN()||!FMath::IsFinite(Radius)||Radius<=0||!FMath::IsFinite(Power)||Power<=0)return;
    const bool NewSupport=FrontlineArmourBlastV844::Applies(Battle(),Credit);
    struct FContactV844 {AFrontlineTank* Vehicle;FVector Point;float Energy,Modules;};TArray<FContactV844,TInlineAllocator<16>> Contacts;
    for(AFrontlineTank* V:Vehicles)if(IsValid(V)&&V->IsOperational()&&V->HasEnteredField())
    {
        if(NewSupport)
        {
            const auto* National=WarfareFRRUArmourV82::Definition(V);
            if(!National&&(!V->Body||!V->Body->GetStaticMesh()))continue;
            // Native PoseableMesh actors retain their actual accepted local
            // hull bounds; their empty legacy Body is not an immune vehicle.
            const FTransform Hull=National?V->GetActorTransform():V->Body->GetComponentTransform();
            const FBox Box=National?FBox(National->Minimum,National->Maximum):V->Body->GetStaticMesh()->GetBoundingBox();if(!Box.IsValid)continue;
            const FVector Local=Hull.InverseTransformPosition(Position);
            const FVector Near(FMath::Clamp(Local.X,Box.Min.X,Box.Max.X),FMath::Clamp(Local.Y,Box.Min.Y,Box.Max.Y),FMath::Clamp(Local.Z,Box.Min.Z,Box.Max.Z));
            const FVector Point=Hull.TransformPosition(Near);const float Distance=float(FVector::Distance(Position,Point));if(Distance>=Radius)continue;
            const auto Response=FrontlineArmourBlastV844::Response(V,EFrontlineSupport(Credit.Support));
            float Shelter=1.f;
            // Real geometry screens the blast; remaining pressure can reach
            // around a bank. Men and the receiving hull do not shadow themselves.
            if(Distance>20.f&&GetWorld()->LineTraceTestByChannel(Position+FVector(0,0,8),Point,ECC_Visibility,SightParams(GetWorld(),nullptr,V)))Shelter=.28f;
            const float Falloff=FMath::Pow(FMath::Clamp(1.f-Distance/Radius,0.f,1.f),1.35f);
            Contacts.Add({V,Point,Power*Response.Energy*Falloff*Shelter,Response.Modules});continue;
        }
        // Test blast distance against the hull footprint, not only its centre.
        const FVector Local=V->GetActorTransform().InverseTransformPosition(Position);
        const float HalfX=V->FootprintHalfLengthV37();
        const auto* National=WarfareFRRUArmourV82::Definition(V);
        const FVector Near=National?FVector(
            FMath::Clamp(Local.X,National->Minimum.X,National->Maximum.X),
            FMath::Clamp(Local.Y,National->Minimum.Y,National->Maximum.Y),
            FMath::Clamp(Local.Z,National->Minimum.Z,National->Maximum.Z)):
            FVector(FMath::Clamp(Local.X,-double(HalfX),double(HalfX)),FMath::Clamp(Local.Y,-190.,190.),FMath::Clamp(Local.Z,30.,210.));
        const float Distance=FVector::Dist2D(Local,Near);
        const bool OrdinarySupport=!WarfareFRRUCampaignV82::Active(Battle())&&Credit.Support>=0
            &&Credit.Support<int32(EFrontlineSupport::Count)&&Credit.Support!=int32(EFrontlineSupport::AntiTank);
        if(Distance<=Radius)V->ReceiveImpact(V->GetActorTransform().TransformPosition(Near),Power*(OrdinarySupport?.80f:1.f)*(1.f-.72f*Distance/Radius),true,Credit);
    }
    // Resolve from one immutable contact assessment. A first victim's wreck
    // must not remove cover and amplify the same round on a later victim.
    for(const auto& Contact:Contacts)if(IsValid(Contact.Vehicle)&&Contact.Vehicle->IsOperational())
        Contact.Vehicle->ReceiveImpact(Contact.Point,Contact.Energy,true,Credit,Contact.Modules);
}
FVector AFrontlineTank::GrenadeAimPointFor(const FVector& ShooterFeet)const
{
    if(WarfareFRRUArmourV82::National(this))return WarfareFRRUArmourV82::GrenadeAim(this,ShooterFeet);
    const FVector Local=GetActorTransform().InverseTransformPosition(ShooterFeet);
    const float HalfX=FootprintHalfLengthV37();
    FVector Near(FMath::Clamp(Local.X,-double(HalfX),double(HalfX)),FMath::Clamp(Local.Y,-190.,190.),35);
    if(FMath::Abs(Local.X)<HalfX&&FMath::Abs(Local.Y)<190)
    {
        if(HalfX-FMath::Abs(Local.X)<190-FMath::Abs(Local.Y))Near.X=Local.X<0?-HalfX:HalfX;
        else Near.Y=Local.Y<0?-190:190;
    }
    FVector Point=GetActorTransform().TransformPosition(Near);
    Point.Z=AWarfareFrontlineGround::HeightAt(Point.X,Point.Y)+12.f;
    return Point;
}
FVector AFrontlineTank::ObservedArmourAimPointV837(const FVector& Observer)const
{
    if(!bRailConvoyV32||!Body||!Body->GetStaticMesh()||Observer.ContainsNaN())return AimPoint();
    const FTransform Hull=Body->GetComponentTransform();const FBox Box=Body->GetStaticMesh()->GetBoundingBox();
    const FVector P=Hull.InverseTransformPosition(Observer);
    // Choose the near carriage side/nose, at its actual plate height. The
    // locomotive centre is up to 26 m behind the plate being observed.
    return Hull.TransformPosition(FVector(FMath::Clamp(P.X,Box.Min.X+20.,Box.Max.X-20.),
        P.Y<0?Box.Min.Y+12.:Box.Max.Y-12.,FMath::Clamp(160.,Box.Min.Z+20.,Box.Max.Z-20.)));
}
bool AFrontlineTank::ContainsHullPointV75(const FVector& Point,float Margin)const
{
    if(WarfareFRRUArmourV82::National(this))return WarfareFRRUArmourV82::Contains(this,Point,Margin);
    if(!Body||!Body->GetStaticMesh()||Point.ContainsNaN()||!FMath::IsFinite(Margin))return false;
    const FVector Local=Body->GetComponentTransform().InverseTransformPosition(Point);
    return Body->GetStaticMesh()->GetBoundingBox().ExpandBy(FMath::Clamp(Margin,0.f,25.f)).IsInsideOrOn(Local);
}
AFrontlineTank* UFrontlineVehicleSubsystem::AcquireGrenadeTarget(const AWarfareFrontlineSoldier* Shooter,float MaxRange,FVector& OutAimPoint)const
{
    OutAimPoint=FVector::ZeroVector;
    const auto* B=Battle();if(!B||!Shooter||!Shooter->IsActiveCombatant()||!FMath::IsFinite(MaxRange)||MaxRange<=0)return nullptr;
    AFrontlineTank* Best=nullptr;float BestDistance=MaxRange;
    for(AFrontlineTank* V:Vehicles)if(IsValid(V)&&V->Team!=Shooter->Team&&V->IsOperational()&&V->HasEnteredField())
    {
        const FVector Point=V->GrenadeAimPointFor(Shooter->Feet());const float Distance=FVector::Dist2D(Shooter->Feet(),Point);
        if(Distance>BestDistance)continue;
        const bool National=WarfareFRRUCampaignV82::Active(B);
        const bool Observed=National?ClearSight(GetWorld(),Shooter,V,Shooter->GetMuzzleLocation(),Point+FVector(0,0,65))
            :ClearSight(GetWorld(),Shooter,V,Shooter->Feet()+FVector(0,0,145),V->ObservedArmourAimPointV837(Shooter->Feet()));
        const float Flight=FrontlineGrenade::FlightSeconds(Distance);
        if(Distance<=BestDistance&&B->DirectFireMultiplier(Shooter->Feet(),Point)>=.9f&&Observed
            &&(National||ClearBombArcV833(Shooter,V,Point,Flight)))
        {Best=V;BestDistance=Distance;OutAimPoint=Point;}
    }
    return Best;
}
void UFrontlineVehicleSubsystem::ApplyAntiTankRifleGrenadeV836(const FVector& Position,float Radius,int32 AttackingTeam,const FFrontlineKillSourceV65& SourceV65)
{
    auto* B=Battle();
    if(!B||WarfareFRRUCampaignV82::Active(B)||Position.ContainsNaN()||!FMath::IsFinite(Radius)
        ||Radius<=0||AttackingTeam<0||AttackingTeam>1)return;
    auto Credit=SourceV65;Credit.Team=AttackingTeam;
    // The early HE rifle charge has to reach real plate or running gear. A
    // committed near miss loses most of its force; it never tracks the hull.
    // Three perfect contacts are required against a standard A7V, four against
    // its overhauled hull; nearby friendly vehicles share contact/earth screening.
    for(AFrontlineTank* V:Vehicles)if(IsValid(V)&&V->IsOperational()&&V->HasEnteredField()
        &&V->Body&&V->Body->GetStaticMesh()&&!WarfareFRRUArmourV82::National(V))
    {
        const FTransform Hull=V->Body->GetComponentTransform();
        const FBox Bounds=V->Body->GetStaticMesh()->GetBoundingBox();if(!Bounds.IsValid)continue;
        const FVector Local=Hull.InverseTransformPosition(Position);
        const FVector Near(FMath::Clamp(Local.X,Bounds.Min.X,Bounds.Max.X),FMath::Clamp(Local.Y,Bounds.Min.Y,Bounds.Max.Y),FMath::Clamp(Local.Z,Bounds.Min.Z,Bounds.Max.Z));
        const FVector ContactPoint=Hull.TransformPosition(Near);const float Gap=FVector::Distance(Position,ContactPoint);
        const float Reach=FMath::Min(90.f,Radius);if(Gap>Reach)continue;
        if(Gap>1.f&&B->GetWorld()->LineTraceTestByChannel(Position+FVector(0,0,3),ContactPoint,ECC_Visibility,SightParams(GetWorld(),nullptr,V)))continue;
        const float Contact=1.f-.70f*Gap/Reach;
        const float HullBefore=V->GetHullIntegrity(),TracksBefore=V->GetTrackIntegrity(0)+V->GetTrackIntegrity(1);
        V->ReceiveRailContactV837(ContactPoint,110.f*Contact,Credit);
        V->ReceiveModuleHit(EFrontlineTankModule::Hull,150.f*Contact,0,Credit);
        if(V->IsOperational())V->ReceiveModuleHit(Local.Y<0?EFrontlineTankModule::LeftTrack:EFrontlineTankModule::RightTrack,70.f*Contact,0,Credit);
        if(HullBefore>V->GetHullIntegrity())if(auto* Fire=AFrontlineFuelFireV33::Get(GetWorld()))
        {
            FVector FlashLocal=Near,NormalLocal=FVector::UpVector;
            if(Bounds.IsInsideOrOn(Local))
            {
                double Closest=DBL_MAX;
                for(int32 Axis=0;Axis<3;++Axis)for(float Side:{-1.f,1.f})
                {const double Face=Side<0?Bounds.Min[Axis]:Bounds.Max[Axis],Distance=FMath::Abs(Face-Local[Axis]);if(Distance<Closest){Closest=Distance;FlashLocal=Local;FlashLocal[Axis]=Face;NormalLocal=FVector::ZeroVector;NormalLocal[Axis]=Side;}}
            }
            else NormalLocal=(Local-Near).GetSafeNormal();
            Fire->EmitArmourContactV836(Hull.TransformPosition(FlashLocal),Hull.TransformVectorNoScale(NormalLocal),Contact,true,EFrontlineArmourContactSoundV839::Grenade);
        }
        FrontlineSpecialistsV41::NoteGrenadeArmourV833(B,Credit,V->Team,HullBefore-V->GetHullIntegrity(),TracksBefore-V->GetTrackIntegrity(0)-V->GetTrackIntegrity(1));
    }
}
void UFrontlineVehicleSubsystem::ApplyGrenadeBlast(const FVector& Position,float Radius,int32 AttackingTeam,const FFrontlineKillSourceV65& SourceV65)
{
    auto Credit=SourceV65;Credit.Team=AttackingTeam;
    if(Position.ContainsNaN()||!FMath::IsFinite(Radius)||Radius<=0||AttackingTeam<0||AttackingTeam>1)return;
    for(AFrontlineTank* V:Vehicles)if(IsValid(V)&&V->IsOperational()&&V->HasEnteredField())
    {
        const FVector Local=V->GetActorTransform().InverseTransformPosition(Position);
        const auto* National=WarfareFRRUArmourV82::Definition(V);
        const FVector Near=National?FVector(
            FMath::Clamp(Local.X,National->Minimum.X,National->Maximum.X),
            FMath::Clamp(Local.Y,National->Minimum.Y,National->Maximum.Y),35):
            FVector(FMath::Clamp(Local.X,-double(V->FootprintHalfLengthV37()),double(V->FootprintHalfLengthV37())),FMath::Clamp(Local.Y,-190.,190.),35);
        const float Gap=FVector::Dist2D(Local,Near),Reach=FMath::Min(170.f,Radius);
        if(Gap>Reach||Local.Z>130.f||Local.Z< -130.f)continue;
        const float HullBefore=V->GetHullIntegrity(),TracksBefore=V->GetTrackIntegrity(0)+V->GetTrackIntegrity(1);
        const float Contact=FMath::Clamp(1.f-Gap/Reach,0.f,1.f);
        V->ReceiveRailContactV837(V->GetActorTransform().TransformPosition(Near),42.f*Contact,Credit);
        // Fragmentation cannot duel a tank's armour. A bomb reaching the running
        // gear now produces a bounded, visible track hit instead of requiring
        // a second very unlikely roll after the already scattered throw.
        if(Gap<=90.f||V->Random.FRand()<.55f*Contact)
            V->ReceiveModuleHit(Local.Y<0?EFrontlineTankModule::LeftTrack:EFrontlineTankModule::RightTrack,42.f*FMath::Lerp(.45f,1.f,Contact),0,Credit);
        // A charge against the plating also shakes the hull. Keep this small,
        // deterministic and restricted to contact, separate from track damage.
        if(Gap<=70.f)V->ReceiveModuleHit(EFrontlineTankModule::Hull,12.f*(1.f-.5f*Gap/70.f),0,Credit);
        if(!National&&(HullBefore>V->GetHullIntegrity()||TracksBefore>V->GetTrackIntegrity(0)+V->GetTrackIntegrity(1))
            &&!GetWorld()->LineTraceTestByChannel(Position+FVector(0,0,3),V->GetActorTransform().TransformPosition(Near)+FVector(0,0,1),ECC_Visibility,SightParams(GetWorld(),nullptr,V)))
            if(auto* Fire=AFrontlineFuelFireV33::Get(GetWorld()))
                Fire->EmitArmourContactV836(V->GetActorTransform().TransformPosition(Near),(Position-V->AimPoint()).GetSafeNormal(),.35f,true,EFrontlineArmourContactSoundV839::Grenade);
        FrontlineSpecialistsV41::NoteGrenadeArmourV833(Battle(),Credit,V->Team,HullBefore-V->GetHullIntegrity(),TracksBefore-V->GetTrackIntegrity(0)-V->GetTrackIntegrity(1));
    }
}
AFrontlineTank* UFrontlineVehicleSubsystem::AcquireInfantryTarget(const AWarfareFrontlineSoldier* Shooter)const
{
    const auto* B=Battle();if(!B||!Shooter||!Shooter->IsActiveCombatant())return nullptr;
    const bool AntiTank=Shooter->Role==EFrontlineRole::AntiTankRifle&&Shooter->MemberIndex==0;
    const AWarfareFrontlineSoldier* No44Lead=nullptr;
    if(!WarfareFRRUCampaignV82::Active(B)&&Shooter->Role==EFrontlineRole::RifleGrenadier)
        if(const auto* Q=B->FindSquad(Shooter->SquadId))for(const auto& Member:Q->Members)
            if(const auto* U=Member.Get();U&&U->Team==Shooter->Team&&U->Battle==Shooter->Battle
                &&U->bFieldEntered&&U->IsActiveCombatant()&&FrontlineSpecialistsV41::HasAntiTankRifleGrenadeV836(U))
            {No44Lead=U;break;}
    // Crew observe the armour that their living, issued No.44 operator can
    // engage. Their own SMLE discharge keeps its independent weapon range.
    const bool No44Crew=No44Lead!=nullptr;
    const float ProfileRange=(No44Crew||FrontlineSpecialistsV41::HasAntiTankRifleGrenadeV836(Shooter))?FrontlineSpecialistsV41::AntiTankRifleGrenadeRangeCmV836
        :FrontlineWeapons::ProfileFor(Shooter).FireRangeCm;
    const bool HostSpecialist=!WarfareFRRUCampaignV82::Active(B)
        &&(AntiTank||No44Crew||(Shooter->Role==EFrontlineRole::RifleGrenadier&&Shooter->MemberIndex==0));
    AFrontlineTank* Best=nullptr;float Distance=(HostSpecialist?ProfileRange:FMath::Min(AntiTank?5400.f:3200.f,ProfileRange))*B->EncounterVisibilityScale();
    for(AFrontlineTank* V:Vehicles)if(IsValid(V)&&V->Team!=Shooter->Team&&V->IsOperational()&&V->HasEnteredField())
    {const FVector Point=V->ObservedArmourAimPointV837(Shooter->Feet());const float D=FVector::Dist2D(Shooter->Feet(),Point);if(D<Distance&&B->DirectFireMultiplier(Shooter->Feet(),Point)>=.9f&&ClearSight(GetWorld(),Shooter,V,Shooter->Feet()+FVector(0,0,145),Point)){Distance=D;Best=V;}}
    return Best;
}
bool UFrontlineVehicleSubsystem::HasInfantryFiringLineV828(const AWarfareFrontlineSoldier* Shooter,const AFrontlineTank* Target)const
{
    const auto* B=Battle();if(!B||!Shooter||!Target||!Shooter->IsActiveCombatant()||!Target->IsOperational()
        ||Target->Team==Shooter->Team||!Target->HasEnteredField())return false;
    const FVector ArmourPoint=Target->ObservedArmourAimPointV837(Shooter->Feet());
    if(B->DirectFireMultiplier(Shooter->GetMuzzleLocation(),ArmourPoint)<.9f)return false;
    if(!WarfareFRRUCampaignV82::Active(B)&&Shooter->Role==EFrontlineRole::RifleGrenadier&&Shooter->MemberIndex==0)
    {
        // No discharge can occur during feeding/recoil. Keep stable observed
        // armour contact without tracing a segmented arc every reload frame;
        // the Ready frame rechecks the complete ballistic path before firing.
        if(Shooter->WeaponPhase!=EFrontlineWeaponPhase::Ready)return true;
        const bool AntiArmour=FrontlineSpecialistsV41::HasAntiTankRifleGrenadeV836(Shooter);
        const FVector Aim=AntiArmour?ArmourPoint:Target->GrenadeAimPointFor(Shooter->Feet());
        const float Range=FVector::Dist2D(Shooter->Feet(),Aim);
        if(AntiArmour&&Range>FrontlineSpecialistsV41::AntiTankRifleGrenadeRangeCmV836*B->EncounterVisibilityScale())return false;
        return ClearBombArcV833(Shooter,Target,Aim,FMath::Clamp(.65f+Range/2100.f,1.05f,2.1f));
    }
    return ClearSight(GetWorld(),Shooter,Target,Shooter->GetMuzzleLocation(),ArmourPoint);
}
void UFrontlineVehicleSubsystem::ApplySmallArmsHit(AFrontlineTank* Target,const FVector& Impact,bool bAutomatic,int32 AttackingTeam)
{
    if(!IsValid(Target)||!Target->IsOperational()||Target->Team==AttackingTeam||Impact.ContainsNaN())return;
    // Ordinary service bullets do not erode armour HP. A rare exposed gun-port hit disables that mount.
    const FVector Local=Target->GetActorTransform().InverseTransformPosition(Impact);
    if(Local.Z<100)return;
    int32 Mount=INDEX_NONE;float Nearest=65.f;
    for(int32 I=0;I<Target->GunMounts.Num();++I)
    {const float Distance=FVector::Dist(Local,Target->GunMounts[I]);if(Distance<Nearest){Nearest=Distance;Mount=I;}}
    if(Mount==INDEX_NONE)return; // A miss beside the hull cannot disable an arbitrary opposite gun.
    FRandomStream R(GetTypeHash(Impact)^int32(GFrameCounter));
    if(R.FRand()<FrontlineWeapons::FirearmHitChance(bAutomatic?.055f:.025f,bAutomatic))Target->ReceiveModuleHit(EFrontlineTankModule::Machinegun,22.f,Mount);
}

AFrontlineTank::AFrontlineTank()
{
    PrimaryActorTick.bCanEverTick=false;
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArmouredHull"));SetRootComponent(Body);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Body->SetCollisionResponseToAllChannels(ECR_Ignore);Body->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Body->SetReceivesDecals(false);Body->SetCastShadow(true);
    TrackLinks=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ArticulatedTrackShoes"));TrackLinks->SetupAttachment(Body);TrackLinks->SetCollisionEnabled(ECollisionEnabled::NoCollision);TrackLinks->SetCastShadow(true);
}
bool AFrontlineTank::Configure(AWarfareFrontlineGameMode* InBattle,EFrontlineVehicle InType,int32 InTeam,int32 InSerial)
{
    Battle=InBattle;Type=InType;Team=InTeam;Serial=InSerial;Random.Initialize(281917+InSerial*73+InTeam*913);
    HeadingYawV37=InBattle&&InBattle->GetTeamDirection(Team)<0?180.f:0.f;PreferredLaneV37=DesiredLaneV37=GetActorLocation().Y;
    FailedGermanLaneV82=0;FailedGermanLaneUntilHostV82=-1;GermanCannonReadbackV82.Reset();
    GermanRecoveryPhaseV826=0;GermanTerrainStallSecondsV826=GermanRecoveryRemainingCmV826=0;
    GermanRecoveryLaneV826=GermanRecoveryBlockedSecondsV826=0;GermanRecoveryReadyAtV826=-1;
    GermanAcceptedRouteV826.Reset();
    const auto& D=UFrontlineVehicleSubsystem::Definition(Type);HullIntegrity=MaximumHullIntegrity=D.Hull;
    bNationalGermanGroundV82=FRRUGermanStaticContactV82::Eligible(Battle,Team,Type);
    if(!bNationalGermanGroundV82)HullIntegrity=MaximumHullIntegrity=D.Hull*FrontlineArmourV63::HullScaleV8218(Battle,Team);
    // Never make the old smoothing path available to any national vehicle.
    if(WarfareFRRUCampaignV82::Active(Battle)&&!bNationalGermanGroundV82)return false;
    TrackIntegrity[0]=TrackIntegrity[1]=(IsWheeledV8218()?95.f:180.f)*(bNationalGermanGroundV82?1.f:FrontlineArmourV63::TrackScale(Battle,Team));
    if(bNationalGermanGroundV82)
    {
        FString Error;if(!FRRUGermanStaticContactV82::AssetsReady(Type,Error))return false;
        Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FRRUGermanStaticContactV82::URI(TEXT("SM_")+FString(D.AssetKey))));
        TrackLinks->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FRRUGermanStaticContactV82::URI(TEXT("SM_TrackShoe"))));
        Body->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
    }
    else
    {
        Body->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Ignore);
        Body->SetStaticMesh(Asset<UStaticMesh>(TEXT("SM_")+FString(D.AssetKey)));
        TrackLinks->SetStaticMesh(Asset<UStaticMesh>(TEXT("SM_TrackShoe")));
    }
    if(!Body->GetStaticMesh()||!TrackLinks->GetStaticMesh())return false;
    ApplySurfaceV37(TrackLinks,.86f,.38f);
    ApplySurfaceV37(Body,.48f+float((Serial*17)%41)*.01f,.22f+float((Serial*13)%35)*.01f);
    if(IsWheeledV8218())
    {
        for(int32 Side=0;Side<2;++Side)for(int32 Axle=0;Axle<2;++Axle)
        {
            auto* W=NewObject<UStaticMeshComponent>(this);W->SetupAttachment(Body);W->SetStaticMesh(Asset<UStaticMesh>(TEXT("SM_AustinWheel")));
            W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetRelativeLocation(FVector(Axle?158.f:-158.f,Side?86.f:-86.f,48.f));
            W->RegisterComponent();W->SetCastShadow(true);ApplySurfaceV37(W,.48f,.08f);RoadWheelsV8218.Add(W);
        }
    }
    else if(Heavy(Type))TrackPath={{-390,55},{-365,230},{230,278},{405,115},{330,18},{-290,18}};
    else if(A7VFamily(Type))TrackPath={{-310,47},{-285,100},{280,100},{310,47},{275,20},{-275,20}};
    else TrackPath={{-300,50},{-260,112},{250,112},{300,50},{255,20},{-255,20}};
    if(Type==EFrontlineVehicle::MarkVStar)for(auto& P:TrackPath)P.X+=P.X>=-165.f?91.f:-91.f;
    TrackPath=RoundTrackCornersV33(TrackPath,Heavy(Type)?27.f:A7VFamily(Type)?21.f:22.f,Heavy(Type)?6:5);
    PathLength=0;for(int32 I=0;I<TrackPath.Num();++I){const float L=(TrackPath[(I+1)%TrackPath.Num()]-TrackPath[I]).Size();TrackSegments.Add(L);PathLength+=L;}
    const int32 Links=FMath::RoundToInt(PathLength/19.f);for(int32 Side=0;Side<2;++Side)for(int32 I=0;I<Links;++I)TrackLinks->AddInstance(FTransform::Identity);
    TrackTransformsV33.SetNum(Links*2);
    auto AddMesh=[&](const TCHAR* Label,const TCHAR* Name,const FVector& P,float Yaw)
    {
        auto* C=NewObject<UStaticMeshComponent>(this,*FString::Printf(TEXT("%s%d"),Label,Guns.Num()+Cannons.Num()));C->SetupAttachment(Body);C->SetStaticMesh(Asset<UStaticMesh>(Name));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetRelativeLocation(P);C->SetRelativeRotation(FRotator(0,Yaw,0));C->RegisterComponent();ApplySurfaceV37(C,.18f,.22f);return C;
    };
    if(IsWheeledV8218())
    {
        // The British 1918 pattern uses two Vickers turrets, not a Putilov hull.
        GunMounts={{-45,-57,245},{-45,57,245}};GunBaseYaw={-18,18};
        for(int32 I=0;I<2;++I)
        {
            auto* T=AddMesh(TEXT("RoadTurret"),TEXT("SM_AustinTurret"),FVector(-45,I?57.f:-57.f,210),GunBaseYaw[I]);
            RoadTurretsV8218.Add(T);
        }
    }
    else if(Female(Type))
    {
        // Four sponson Lewis guns and one hull gun; no cannon substitution.
        // The forward corner mounts overlap their arcs ahead of the tracks.
        GunMounts={{250,0,177},{92,-191,183},{92,191,183},{-105,-191,183},{-105,191,183}};
        GunBaseYaw={0,-38,38,-138,138};
    }
    else if(Heavy(Type))
    {
        CannonMounts={{65,-209,157},{65,209,157}};CannonBaseYaw={-28,28};
        GunMounts={{250,0,177},{-120,-177,170},{-120,177,170},{-260,0,167}};GunBaseYaw={0,-100,100,180};
        if(Type==EFrontlineVehicle::MarkVMale||Type==EFrontlineVehicle::MarkVStar)GunMounts[3]=FVector(-259,0,254);
        if(Type==EFrontlineVehicle::MarkVStar)
        {
            for(auto& P:CannonMounts)P.X+=P.X>=-165.f?91.f:-91.f;
            for(auto& P:GunMounts)P.X+=P.X>=-165.f?91.f:-91.f;
            // Additional side ball ports are part of the hull, not two more
            // carried guns. The male V* retained four Hotchkiss machine guns.
        }
    }
    else if(A7VFamily(Type))
    {
        if(Type!=EFrontlineVehicle::A7VGretchen){CannonMounts={{342,0,125}};CannonBaseYaw={0};}
        GunMounts={{170,-151,175},{-145,-151,175},{170,151,175},{-145,151,175},{-340,-75,172},{-340,75,172}};GunBaseYaw={-65,-120,65,120,-165,165};
        if(Type==EFrontlineVehicle::A7VGretchen){GunMounts.Add(FVector(349,-53,163));GunMounts.Add(FVector(349,53,163));GunBaseYaw.Add(-12);GunBaseYaw.Add(12);}
    }
    else
    {GunMounts={{-60,0,215},{-150,-92,212},{-150,92,212},{-250,0,211}};GunBaseYaw={0,-75,75,180};}
    for(int32 I=0;I<CannonMounts.Num();++I)Cannons.Add(AddMesh(TEXT("Cannon"),TEXT("SM_SixPounder"),CannonMounts[I],CannonBaseYaw[I]));
    for(int32 I=0;I<GunMounts.Num();++I)
    {
        Guns.Add(AddMesh(TEXT("Machinegun"),IsWheeledV8218()?TEXT("SM_AustinVickers"):Type==EFrontlineVehicle::Whippet||Type==EFrontlineVehicle::MarkVMale||Type==EFrontlineVehicle::MarkVStar?TEXT("SM_HotchkissTankMG"):TEXT("SM_TankMG"),GunMounts[I],GunBaseYaw[I]));
        GunIntegrity.Add(60.f*(bNationalGermanGroundV82?1.f:FrontlineArmourV63::GunIntegrityScale(Battle,Team)));GunClock.Add(Random.FRandRange(1,3));BurstRemaining.Add(0);
    }
    auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    auto* FlashBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Characters/V13/Materials/M_MuzzleFlash_V13.M_MuzzleFlash_V13"));
    for(int32 I=0;I<Guns.Num()+Cannons.Num();++I)
    {
        auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(Body);C->SetStaticMesh(Plane);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();C->SetVisibility(false);Flashes.Add(C);FlashAge.Add(10);
        auto* M=UMaterialInstanceDynamic::Create(FlashBase,this);C->SetMaterial(0,M);FlashMaterials.Add(M);
    }
    for(int32 I=0;I<6;++I)
    {
        auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(Body);C->SetStaticMesh(Plane);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();C->SetVisibility(false);Smoke.Add(C);
        auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V23/Materials/M_BlastDust_2_V23.M_BlastDust_2_V23"));
        auto* M=UMaterialInstanceDynamic::Create(Base,this);if(M){M->SetScalarParameterValue(TEXT("Density"),0);M->SetScalarParameterValue(TEXT("Seed"),I*.87f);}C->SetMaterial(0,M);SmokeMaterials.Add(M);
    }
    // Wreck fires share the V33 bounded fuel/smoke service; no opaque old flame card.
    AddMarkingsV37();UpdateTracks(0);return AreAssetsReady();
}
void AFrontlineTank::ApplySurfaceV37(UStaticMeshComponent* Component,float Dust,float Rust)
{
    if(!Component||!Component->GetStaticMesh())return;
    const auto& Slots=Component->GetStaticMesh()->GetStaticMaterials();
    for(int32 I=0;I<Component->GetNumMaterials();++I)
    {
        FString Key=Slots.IsValidIndex(I)?Slots[I].MaterialSlotName.ToString():FString();
        // The legacy Whippet door had an empty white plaque. Its actual
        // serial is now painted beside the door, so coat that blank patch.
        if(Type==EFrontlineVehicle::Whippet&&Component==Body&&Key==TEXT("White"))Key=TEXT("British");
        if(bRailConvoyV32&&Key==TEXT("German"))Key=TEXT("RailPaint");
        if(bRailConvoyV32&&Key==TEXT("Steel"))Key=TEXT("RailSteel");
        FString Model;
        if(!bNationalGermanGroundV82&&!bRailConvoyV32)
        {
            if(Component==Body&&Component->GetStaticMesh()->GetName()==FString(TEXT("SM_"))+UFrontlineVehicleSubsystem::Definition(Type).AssetKey+TEXT("_V838"))Model=UFrontlineVehicleSubsystem::Definition(Type).AssetKey;
            else if(Component->GetStaticMesh()->GetName()==TEXT("SM_AustinWheel_V838"))Model=TEXT("AustinWheel");
            else if(Component->GetStaticMesh()->GetName()==TEXT("SM_AustinTurret_V838"))Model=TEXT("AustinTurret");
        }
        UMaterialInterface* Bespoke=FrontlineVehicleArtV836::ResolveModelMaterialV838(Model,Key);
        UMaterialInterface* Source=Bespoke?Bespoke:Key.IsEmpty()?Component->GetMaterial(I):
            (bNationalGermanGroundV82?nullptr:FrontlineVehicleArtV836::ResolveMaterial(Key));
        if(!Source&&!Key.IsEmpty())Source=Asset<UMaterialInterface>(TEXT("M_Vehicle_")+Key,!bNationalGermanGroundV82);
        if(!Source)Source=Component->GetMaterial(I);
        if(auto* M=Component->CreateDynamicMaterialInstance(I,Source))
        {
            M->SetScalarParameterValue(TEXT("DeploymentSeed"),float((Serial*37+Team*1917)%997));
            if(!Bespoke)
            {
                M->SetScalarParameterValue(TEXT("FieldDust"),Dust);M->SetScalarParameterValue(TEXT("FieldRust"),Rust);
                M->SetScalarParameterValue(TEXT("CamouflageStrength"),bRailConvoyV32?0.f:A7VFamily(Type)?.32f:GermanVehicle(Type)?.64f:0.f);
                M->SetScalarParameterValue(TEXT("RecognitionPaint"),Component==Body&&!bRailConvoyV32?1.f:0.f);
                M->SetScalarParameterValue(TEXT("RecognitionType"),A7VFamily(Type)?1.f:0.f);
            }
            FrontlineVehicleArtV836::HoldMaterialDetailV837(M);
            BodyMaterials.Add(M);
        }
    }
}
void AFrontlineTank::AddMarkingsV37()
{
    if(bRailConvoyV32||Type==EFrontlineVehicle::MarkVStar||IsWheeledV8218()||GermanVehicle(Type))return;
    const int32 Variant=((Serial+1)/2+Team)%4;
    // Unchanged side frames accept the existing period stencil overlays.
    const auto MarkingType=A7VFamily(Type)?EFrontlineVehicle::A7V:
        Heavy(Type)?(GermanVehicle(Type)?EFrontlineVehicle::BeuteMarkIV:EFrontlineVehicle::MarkIVMale):Type;
    const FString Name=FString::Printf(TEXT("SM_Markings_%s_%d_V37"),UFrontlineVehicleSubsystem::Definition(MarkingType).AssetKey,Variant);
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Warfare/Vehicles/V37/")+Name+TEXT(".")+Name));if(!Mesh)return;
    auto* C=NewObject<UStaticMeshComponent>(this,TEXT("PaintedCrewIdentity"));C->SetupAttachment(Body);C->SetStaticMesh(Mesh);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();MarkingsV37.Add(C);
    ApplySurfaceV37(C,.55f,.22f);
}
bool AFrontlineTank::AreAssetsReady()const
{
    if(WarfareFRRUArmourV82::National(this)){FString Error;return WarfareFRRUArmourV82::AssetsReady(WarfareFRRUArmourV82::Id(this),Error);}
    if(bNationalGermanGroundV82)
    {
        FString Error;if(!Body||!TrackLinks||!Body->GetStaticMesh()||!TrackLinks->GetStaticMesh()
            ||!FRRUGermanStaticContactV82::Eligible(Battle,Team,Type)||!FRRUGermanStaticContactV82::AssetsReady(Type,Error)
            ||Body->GetStaticMesh()->GetPathName()!=FRRUGermanStaticContactV82::URI(TEXT("SM_")+FString(UFrontlineVehicleSubsystem::Definition(Type).AssetKey))
            ||TrackLinks->GetStaticMesh()->GetPathName()!=FRRUGermanStaticContactV82::URI(TEXT("SM_TrackShoe")))return false;
    }
    if(!Body->GetStaticMesh()||!TrackLinks->GetStaticMesh())return false;
    // A rail conversion deliberately replaces the donor A7V hull. Validate the
    // train's own exact assets before the ordinary donor-model path checks.
    if(bRailConvoyV32)
    {
        const bool NewBody=Body->GetStaticMesh()->GetName()==TEXT("SM_RailConvoy_V837");
        return (Body->GetStaticMesh()==FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_RailConvoy"))||Body->GetStaticMesh()->GetName()==TEXT("SM_RailConvoy_V75"))
            &&Guns.Num()==6&&Cannons.Num()==2&&RailWheelsV33.Num()==6
            &&(!NewBody||(RailBogieWheelsV837.Num()==20&&RailDoorsV837.Num()==4&&RailCupolasV837.Num()==2
                &&Cannons[0]->GetStaticMesh()==FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_RailCannon"))
                &&Cannons[1]->GetStaticMesh()==FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_RailCannon"))));
    }
    if(!bNationalGermanGroundV82&&(Type==EFrontlineVehicle::MarkVStar||IsWheeledV8218()||GermanVehicle(Type)))
    {
        const FString Base=TEXT("SM_")+FString(UFrontlineVehicleSubsystem::Definition(Type).AssetKey);
        const bool CurrentMesh=Body->GetStaticMesh()==FrontlineVehicleArtV836::ResolveMesh(Base);
        const FString Key=Base+TEXT("_V8218");
        const FString Path=TEXT("/Game/Warfare/Vehicles/V8218/")+Key+TEXT(".")+Key;
        if(!CurrentMesh&&Body->GetStaticMesh()->GetPathName()!=Path)return false;
        const auto Extent=Body->GetStaticMesh()->GetBoundingBox().GetExtent();
        if(Type==EFrontlineVehicle::MarkVStar&&(Extent.X<470.f||Extent.X>530.f||TrackLinks->GetInstanceCount()<200))return false;
        if(IsWheeledV8218())
        {
            if(Extent.X<220.f||Extent.X>270.f||TrackLinks->GetInstanceCount()!=0||RoadWheelsV8218.Num()!=4||RoadTurretsV8218.Num()!=2)return false;
            for(const auto& W:RoadWheelsV8218)if(!W||!W->GetStaticMesh()||
                (W->GetStaticMesh()!=FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_AustinWheel"))&&!W->GetStaticMesh()->GetName().Equals(TEXT("SM_AustinWheel_V8218"))))return false;
            for(const auto& T:RoadTurretsV8218)if(!T||!T->GetStaticMesh()||
                (T->GetStaticMesh()!=FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_AustinTurret"))&&!T->GetStaticMesh()->GetName().Equals(TEXT("SM_AustinTurret_V8218"))))return false;
        }
    }
    if(Type==EFrontlineVehicle::MarkVMale)
    {
        // Validate the actual imported hull, not merely a non-null fallback.
        // Bounds are measured from the reviewed centimetre-scale V63 sources.
        const FString Key=FString(TEXT("SM_"))+UFrontlineVehicleSubsystem::Definition(Type).AssetKey;
        const FString Path=TEXT("/Game/Warfare/Vehicles/Production/V63/")+Key+TEXT(".")+Key;
        const FBox Bounds=Body->GetStaticMesh()->GetBoundingBox();
        const bool MarkV=Type==EFrontlineVehicle::MarkVMale;
        const FVector Minimum=MarkV?FVector(-387.382,-216.850,17.047):FVector(-364.963,-159.985,18.8);
        const FVector Maximum=MarkV?FVector(397.132,216.850,303.040):FVector(366.965,159.985,324.0);
        if((Body->GetStaticMesh()!=FrontlineVehicleArtV836::ResolveMesh(Key)&&Body->GetStaticMesh()->GetPathName()!=Path)||!Bounds.IsValid||Bounds.Min.ContainsNaN()||Bounds.Max.ContainsNaN()
            ||!Bounds.Min.Equals(Minimum,1.f)||!Bounds.Max.Equals(Maximum,1.f))return false;
    }
    for(const UStaticMeshComponent* C:Guns)if(!C||!C->GetStaticMesh())return false;
    for(const UStaticMeshComponent* C:Cannons)if(!C||!C->GetStaticMesh())return false;
    return Guns.Num()==UFrontlineVehicleSubsystem::Definition(Type).MachinegunCount
        &&Cannons.Num()==UFrontlineVehicleSubsystem::Definition(Type).CannonCount;
}
void AFrontlineTank::UpdateTracks(float Distance,float TurnRadians)
{
    if(IsWheeledV8218())
    {
        TrackOffset=FMath::Fmod(TrackOffset+Distance,2.f*PI*48.f);
        // A car steers its front axle while rolling. It cannot spin its hull
        // on stationary tyres. Both wheel pairs retain their measured phases.
        const float Steer=FMath::Abs(Distance)>.01f?FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan(316.f*TurnRadians/Distance)),-30.f,30.f):0.f;
        WheelSteerV8311=Steer;
        for(int32 Side=0;Side<2;++Side)
        {
            TrackSideOffsetV8311[Side]=FMath::Fmod(TrackSideOffsetV8311[Side]+Distance+(Side?-86.f:86.f)*TurnRadians,2.f*PI*48.f);
            const float Roll=FMath::RadiansToDegrees(TrackSideOffsetV8311[Side]/48.f);
            const float Curvature=FMath::Abs(Distance)>.01f?TurnRadians/Distance:0.f;
            const float FrontSteer=FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(316.f*Curvature,1.f-(Side?86.f:-86.f)*Curvature)),-34.f,34.f);
            for(int32 Axle=0;Axle<2;++Axle)if(RoadWheelsV8218.IsValidIndex(Side*2+Axle)&&RoadWheelsV8218[Side*2+Axle])
                RoadWheelsV8218[Side*2+Axle]->SetRelativeRotation(FRotator(Roll,Axle?FrontSteer:0.f,0));
        }
        return;
    }
    if(PathLength<=0)return;TrackOffset=FMath::Fmod(TrackOffset+Distance,PathLength);if(TrackOffset<0.f)TrackOffset+=PathLength;
    const int32 Count=TrackLinks->GetInstanceCount()/2;
    const float Y=Heavy(Type)?147.f:A7VFamily(Type)?123.f:109.f;
    for(int32 Side=0;Side<2;++Side)
    {
        TrackSideOffsetV8311[Side]=FMath::Fmod(TrackSideOffsetV8311[Side]+Distance+(Side?-Y:Y)*TurnRadians,PathLength);
        if(TrackSideOffsetV8311[Side]<0.f)TrackSideOffsetV8311[Side]+=PathLength;
    }
    for(int32 Side=0;Side<2;++Side)for(int32 I=0;I<Count;++I)
    {
        float D=FMath::Fmod(float(I)*PathLength/Count+TrackSideOffsetV8311[Side],PathLength);int32 Segment=0;
        while(Segment<TrackSegments.Num()-1&&D>TrackSegments[Segment]){D-=TrackSegments[Segment];++Segment;}
        const FVector2D Delta=TrackPath[(Segment+1)%TrackPath.Num()]-TrackPath[Segment];const FVector2D P=TrackPath[Segment]+Delta*(D/TrackSegments[Segment]);
        const float Pitch=-FMath::RadiansToDegrees(FMath::Atan2(Delta.Y,Delta.X));
        TrackTransformsV33[Side*Count+I]=FTransform(FRotator(Pitch,0,0),FVector(P.X,(Side?1:-1)*Y,P.Y),FVector(1,Type==EFrontlineVehicle::MarkVStar?1.50f:Heavy(Type)?1.15f:1,1));
    }
    TrackLinks->BatchUpdateInstancesTransforms(0,TrackTransformsV33,false,true,true);
}
FVector AFrontlineTank::WeaponMuzzle(int32 Index,bool bCannon)const
{
    if(WarfareFRRUArmourV82::National(this))return WarfareFRRUArmourV82::Muzzle(this,Index,bCannon);
    const auto& C=bCannon?Cannons:Guns;
    if(!C.IsValidIndex(Index))return AimPoint();
    const bool LongRailBarrel=bCannon&&bRailConvoyV32&&C[Index]->GetStaticMesh()
        &&C[Index]->GetStaticMesh()->GetName()==TEXT("SM_RailCannon_V837");
    return C[Index]->GetComponentTransform().TransformPosition(FVector(bCannon?(LongRailBarrel?217.f:115.f):IsWheeledV8218()?81.5f:72.f,0,0));
}
FTransform AFrontlineTank::GroundPoseForContactV8218(const FVector& Position,float Yaw,float MotionSeconds,const FTransform* FromPose)const
{
    if(!Battle||Position.ContainsNaN()||!FMath::IsFinite(Yaw))return GetActorTransform();
    if(bRailConvoyV32)return FTransform(FRotator(0,Battle->GetTeamDirection(Team)>0?0.f:180.f,0),FVector(Position.X,-1130,28),FVector::OneVector);
    if(bNationalGermanGroundV82)
    {
        // The boxed national contact adapter keeps its accepted pitch/roll and
        // owns its own source/receiver contract; shared campaigns do not rerun it.
        FTransform Pose=GetActorTransform();FRotator Rotation=Pose.Rotator();Rotation.Yaw=Yaw;
        Pose.SetLocation(Position);Pose.SetRotation(Rotation.Quaternion());return Pose;
    }
    const bool LongHull=Heavy(Type);
    const float Rear=IsWheeledV8218()?158.f:Type==EFrontlineVehicle::MarkVStar?381.f:LongHull?290.f:A7VFamily(Type)?275.f:255.f;
    const float Front=IsWheeledV8218()?158.f:Type==EFrontlineVehicle::MarkVStar?421.f:LongHull?330.f:Rear;
    const float HalfWidth=IsWheeledV8218()?86.f:(LongHull?147.f:A7VFamily(Type)?123.f:109.f)+TrackWidth*(Type==EFrontlineVehicle::MarkVStar?.75f:.5f);
    const float Sole=IsWheeledV8218()?0.f:LongHull?12.f:14.f;
    return AWarfareFrontlineGround::TrackSupportAtV35(Position,FromPose?FromPose->Rotator():GetActorRotation(),Yaw,Rear,Front,HalfWidth,Sole,MotionSeconds);
}
void AFrontlineTank::FitGroundV35(const FVector& Position,float Seconds)
{
    if(!Battle)return;
    if(bNationalGermanGroundV82){CommitNationalGroundV82(Position,HeadingYawV37,0);return;}
    const float Yaw=bRailConvoyV32?(Battle->GetTeamDirection(Team)>0?0.f:180.f):HeadingYawV37;
    if(bRailConvoyV32){SetActorTransform(GroundPoseForContactV8218(Position,Yaw));return;}
    // Even a blocked XY step can continue damping pitch/roll at a trench.
    // Commit that support change through the same live-body gate as travel.
    if(Seconds>0.f&&!CanOccupyV48(Position,Yaw,Seconds))
    {HeadingYawV37=float(GetActorRotation().Yaw);return;}
    SetActorTransform(GroundPoseForContactV8218(Position,Yaw,Seconds));
    RefreshInfantryPassCertificateV86();
}
bool AFrontlineTank::CommitNationalGroundV82(const FVector& Position,float Heading,float Distance,bool Initial,bool Reverse)
{
    using namespace FRRUGermanStaticContactV82;
    if(!bNationalGermanGroundV82||!Eligible(Battle,Team,Type)||!Body||!TrackLinks||Position.ContainsNaN()
        ||!FMath::IsFinite(Heading)||!FMath::IsFinite(Distance)||Distance<0||PathLength<=0
        ||UGameplayStatics::IsGamePaused(GetWorld()))return false;
    const FTransform Before=GetActorTransform();const float PriorHeading=HeadingYawV37,PriorOffset=TrackOffset;
    const double Travel=FVector::Dist2D(Before.GetLocation(),Position);
    // Caller supplies actual travelled distance, never a phase-only advance.
    if(!Initial&&FMath::Abs(Travel-Distance)>.01)return false;
    const double Turn=FMath::FindDeltaAngleDegrees(PriorHeading,Heading);
    const int32 Steps=FMath::Max(1,FMath::Max(FMath::CeilToInt(Travel/10.),FMath::CeilToInt(FMath::Abs(Turn)/3.)));
    if(Steps>MaxMoveSubsteps)return false;
    FBox RigidEnvelope=Body->GetStaticMesh()?Body->GetStaticMesh()->GetBoundingBox():FBox(ForceInit);
    if(!RigidEnvelope.IsValid)return false;
    // Include every actual rigid weapon throughout its authored traverse.
    // Flashes, smoke and markings never inflate the collision envelope.
    for(const auto* Group:{&Guns,&Cannons})for(const auto& Component:*Group)
    {
        if(!Component||!Component->GetStaticMesh()||!Component->GetRelativeScale3D().Equals(FVector::OneVector,1.e-6))return false;
        const FBox Bounds=Component->GetStaticMesh()->GetBoundingBox();if(!Bounds.IsValid||Bounds.Min.ContainsNaN()||Bounds.Max.ContainsNaN())return false;
        double Radius=0;
        for(int32 X=0;X<2;++X)for(int32 Y=0;Y<2;++Y)for(int32 Z=0;Z<2;++Z)
            Radius=FMath::Max(Radius,FVector(X?Bounds.Max.X:Bounds.Min.X,Y?Bounds.Max.Y:Bounds.Min.Y,Z?Bounds.Max.Z:Bounds.Min.Z).Size());
        const FVector Centre=Component->GetRelativeLocation();RigidEnvelope+=Centre-FVector(Radius);RigidEnvelope+=Centre+FVector(Radius);
    }
    FTransform Previous=Before;FPlan Plan;
    const bool RecordRoute=bNationalGermanGroundV82&&!Reverse
        &&(GermanRecoveryPhaseV826==0||(GermanRecoveryPhaseV826==3&&Travel>.01));
    TArray<FGermanAcceptedPoseV826,TInlineAllocator<32>> AcceptedSteps;
    for(int32 I=1;I<=Steps;++I)
    {
        const double Fraction=double(I)/Steps;
        const FVector Desired=FMath::Lerp(Before.GetLocation(),Position,Fraction);
        const float NextHeading=PriorHeading+float(Turn*Fraction);
        const float SignedDistance=Reverse?-Distance:Distance;
        const float RawPhase=FMath::Fmod(PriorOffset+SignedDistance*float(Fraction),PathLength);
        const float Phase=RawPhase<0?RawPhase+PathLength:RawPhase;
        FPlan Candidate;
        if(!FRRUGermanStaticContactV82::Plan(this,Battle,Body,TrackLinks,TrackPath,TrackSegments,PathLength,Phase,Previous,
            Desired,NextHeading,Heavy(Type),Heavy(Type)?147.f:123.f,RigidEnvelope,Candidate,I>1?&Plan:nullptr))return false;
        Previous=Candidate.Actor;Plan=MoveTemp(Candidate);
        if(RecordRoute)AcceptedSteps.Add({Previous.GetLocation(),NextHeading});
    }
    const TArray<FTransform> PriorTracks=TrackTransformsV33;
    // All finite/native/support/complete-volume gates precede the first live
    // edit. No distance, heading, track phase, cooldown or warm state is spent
    // by a failed proposal. This function is called on the game thread.
    if(!SetActorTransform(Plan.Actor,false,nullptr,ETeleportType::None))return false;
    if(!TrackLinks->BatchUpdateInstancesTransforms(0,Plan.Tracks,false,true,true))
    {
        SetActorTransform(Before,false,nullptr,ETeleportType::None);
        TrackLinks->BatchUpdateInstancesTransforms(0,PriorTracks,false,true,true);return false;
    }
    TrackTransformsV33=Plan.Tracks;
    const float RawPhase=FMath::Fmod(PriorOffset+(Reverse?-Distance:Distance),PathLength);
    TrackOffset=RawPhase<0?RawPhase+PathLength:RawPhase;
    HeadingYawV37=Heading;Committed(this,Plan);
    if(RecordRoute)
    {
        if(GermanRecoveryPhaseV826==3)GermanAcceptedRouteV826.Reset();
        if(GermanAcceptedRouteV826.IsEmpty())GermanAcceptedRouteV826.Add({Before.GetLocation(),PriorHeading});
        for(const auto& Step:AcceptedSteps)
        {
            const auto& Last=GermanAcceptedRouteV826.Last();
            if(FVector::Dist2D(Last.Position,Step.Position)>.001
                ||FMath::Abs(FMath::FindDeltaAngleDegrees(Last.Heading,Step.Heading))>.001f)
                GermanAcceptedRouteV826.Add(Step);
        }
        // Bound both retained travel and frame-dependent sampling density.
        double RetainedDistance=0;int32 First=GermanAcceptedRouteV826.Num()-1;
        while(First>0&&RetainedDistance<500.&&GermanAcceptedRouteV826.Num()-First<1024)
        {RetainedDistance+=FVector::Dist2D(GermanAcceptedRouteV826[First].Position,GermanAcceptedRouteV826[First-1].Position);--First;}
        if(First>0)GermanAcceptedRouteV826.RemoveAt(0,First,EAllowShrinking::No);
    }
    return true;
}
bool AFrontlineTank::OccupiesNationalNodeV82(const FVector& Centre,float Radius)const
{
    if(!bNationalGermanGroundV82||!FRRUGermanStaticContactV82::Eligible(Battle,Team,Type)||!IsOperational()||!HasEnteredField()
        ||Centre.ContainsNaN()||!FMath::IsFinite(Radius)||Radius<0||!Body||!Body->GetStaticMesh()
        ||!GetActorScale3D().Equals(FVector::OneVector,1.e-6)||Body->GetComponentTransform().ContainsNaN())return false;
    const FBox Hull=Body->GetStaticMesh()->GetBoundingBox();if(!Hull.IsValid||Hull.Min.ContainsNaN()||Hull.Max.ContainsNaN())return false;
    const FVector P=Body->GetComponentTransform().InverseTransformPosition(Centre);
    const FVector Nearest(FMath::Clamp(P.X,Hull.Min.X,Hull.Max.X),FMath::Clamp(P.Y,Hull.Min.Y,Hull.Max.Y),P.Z);
    return FVector::DistSquared2D(P,Nearest)<=FMath::Square(double(Radius));
}
TSharedPtr<FJsonObject> AFrontlineTank::DescribeNationalGroundContactV82()const
{
    if(!bNationalGermanGroundV82)return nullptr;
    const auto Contact=FRRUGermanStaticContactV82::Diagnosis(this);if(!Contact)return nullptr;
    // A fresh top-level snapshot keeps later weapon decisions from rewriting
    // contact objects already retained by earlier review samples.
    auto Snapshot=MakeShared<FJsonObject>();Snapshot->Values=Contact->Values;
    if(GermanCannonReadbackV82)Snapshot->SetObjectField(TEXT("cannonReadback"),GermanCannonReadbackV82);
    Snapshot->SetNumberField(TEXT("recoveryPhase"),GermanRecoveryPhaseV826);
    Snapshot->SetNumberField(TEXT("recoveryLaneCm"),GermanRecoveryLaneV826);
    Snapshot->SetNumberField(TEXT("recoveryRemainingCm"),GermanRecoveryRemainingCmV826);
    Snapshot->SetNumberField(TEXT("acceptedRoutePoses"),GermanAcceptedRouteV826.Num());
    return Snapshot;
}
float AFrontlineTank::FootprintHalfLengthV37()const
{
    if(WarfareFRRUArmourV82::National(this))return WarfareFRRUArmourV82::HalfLength(this);
    if(bRailConvoyV32&&Body&&Body->GetStaticMesh())
    {
        const FBox Box=Body->GetStaticMesh()->GetBoundingBox();
        return FMath::Max(FMath::Abs(float(Box.Min.X)),FMath::Abs(float(Box.Max.X)));
    }
    return IsWheeledV8218()?245.f:Type==EFrontlineVehicle::MarkVStar?505.f:Heavy(Type)?410.f:A7VFamily(Type)?370.f:310.f;
}
float AFrontlineTank::FootprintHalfWidthV8311()const
{
    const float RunningGear=IsWheeledV8218()?108.f:bRailConvoyV32?155.f:Heavy(Type)?177.f:A7VFamily(Type)?145.f:131.f;
    if(!Body||!Body->GetStaticMesh())return RunningGear;
    const FBox Box=Body->GetStaticMesh()->GetBoundingBox();
    return Box.IsValid?FMath::Max(RunningGear,FMath::Max(FMath::Abs(float(Box.Min.Y)),FMath::Abs(float(Box.Max.Y)))):RunningGear;
}
float AFrontlineTank::SteeringRateV8311(float Speed,bool Stationary)const
{
    if(bRailConvoyV32)return 0.f;
    if(IsWheeledV8218())return Stationary?0.f:FMath::Min(28.f,FMath::RadiansToDegrees(FMath::Max(0.f,Speed)/550.f));
    // Game-time angular tuning follows chassis geometry and drivetrain. These
    // are presentation limits, not claimed period test measurements.
    if(Type==EFrontlineVehicle::Whippet)return Stationary?8.f:25.f;
    if(Type==EFrontlineVehicle::MarkVMale)return Stationary?12.f:21.f;
    if(Type==EFrontlineVehicle::MarkVStar)return Stationary?8.f:15.f;
    if(A7VFamily(Type))return Stationary?10.f:18.f;
    return Stationary?8.f:16.f;
}
float AFrontlineTank::ArmourOverlapV8311(const FVector& Position,float Yaw,const AFrontlineTank* Other,float Margin)const
{
    if(!IsValid(Other)||Other==this||Other->IsHidden())return 0.f;
    const FVector2D Delta(Other->GetActorLocation().X-Position.X,Other->GetActorLocation().Y-Position.Y);
    const float ALength=FootprintHalfLengthV37(),BLength=Other->FootprintHalfLengthV37();
    if(Delta.SizeSquared()>FMath::Square(ALength+BLength+600.f+Margin))return 0.f;
    const float AAngle=FMath::DegreesToRadians(Yaw),BAngle=FMath::DegreesToRadians(float(Other->GetActorRotation().Yaw));
    const FVector2D Ax(FMath::Cos(AAngle),FMath::Sin(AAngle)),Ay(-Ax.Y,Ax.X),Bx(FMath::Cos(BAngle),FMath::Sin(BAngle)),By(-Bx.Y,Bx.X);
    const float AWidth=FootprintHalfWidthV8311(),BWidth=Other->FootprintHalfWidthV8311();float Penetration=MAX_flt;
    for(const FVector2D& Axis:{Ax,Ay,Bx,By})
    {
        const float A=ALength*FMath::Abs(FVector2D::DotProduct(Axis,Ax))+AWidth*FMath::Abs(FVector2D::DotProduct(Axis,Ay));
        const float C=BLength*FMath::Abs(FVector2D::DotProduct(Axis,Bx))+BWidth*FMath::Abs(FVector2D::DotProduct(Axis,By));
        const float Depth=A+C+Margin-FMath::Abs(FVector2D::DotProduct(Delta,Axis));if(Depth<=0.f)return 0.f;
        Penetration=FMath::Min(Penetration,Depth);
    }
    return Penetration;
}
float AFrontlineTank::RearProgressV49()const
{
    if(!Battle||GetActorTransform().ContainsNaN())return -MAX_flt;
    const float HalfX=FMath::Abs(GetActorForwardVector().X)*FootprintHalfLengthV37()
        +FMath::Abs(GetActorRightVector().X)*(bRailConvoyV32?155.f:225.f);
    const int32 Dir=Battle->GetTeamDirection(Team);
    float Rear=GetActorLocation().X*Dir-HalfX;
    // The chassis footprint alone omits the rear upper hull on a pitched
    // vehicle. Use only the actual body bounds, never smoke or shell bounds.
    if(Body&&Body->GetStaticMesh())
    {
        const FBox Hull=Body->Bounds.GetBox();
        if(Hull.IsValid)Rear=FMath::Min(Rear,float(Dir>0?Hull.Min.X:-Hull.Max.X));
    }
    return Rear;
}
bool AFrontlineTank::HasEnteredField()const
{
    if(WarfareFRRUArmourV82::National(this))return bEntered||WarfareFRRUArmourV82::Entered(this);
    if(bEntered)return true;
    if(!Battle)return false;
    const int32 Dir=Battle->GetTeamDirection(Team);
    const float Nose=GetActorLocation().X+Dir*(FMath::Abs(GetActorForwardVector().X)*FootprintHalfLengthV37()
        +FMath::Abs(GetActorRightVector().X)*225.f);
    return FrontlinePacingV24::HasCrossedEntry(Nose,Dir);
}
bool AFrontlineTank::HasClearedEnemyEndV49()const
{
    return Battle&&Battle->Phase==EFrontlinePhase::Fighting&&IsOperational()&&bEntered
        &&FMath::Abs(GetActorLocation().Y)<=FrontlineBattlefield::HalfFrontage+200.f
        &&RearProgressV49()>FrontlineGameplayV49::BreakthroughLine()
        &&!FrontlineGameplayV49::EnemyEndDefended(Battle,Team);
}
TSharedRef<FJsonObject> AFrontlineTank::DescribeMobilityV839()const
{
    auto J=MakeShared<FJsonObject>();
    J->SetNumberField(TEXT("model"),int32(Type));J->SetNumberField(TEXT("team"),Team);J->SetNumberField(TEXT("serial"),Serial);
    J->SetNumberField(TEXT("host_seconds"),MobilityHostTimeV839);J->SetNumberField(TEXT("step_seconds"),MobilityStepV839);
    J->SetNumberField(TEXT("requested_cm"),MobilityRequestedV839);J->SetNumberField(TEXT("moved_cm"),MobilityMovedV839);
    J->SetStringField(TEXT("stop_gate"),MobilityGateV839);J->SetStringField(TEXT("position"),GetActorLocation().ToString());
    J->SetStringField(TEXT("rotation"),GetActorRotation().ToString());J->SetNumberField(TEXT("heading_deg"),HeadingYawV37);
    J->SetNumberField(TEXT("desired_lane_cm"),DesiredLaneV37);J->SetNumberField(TEXT("lane_recovery_seconds"),RouteStallSecondsV837);
    J->SetNumberField(TEXT("static_body_detour_remaining_poses"),FMath::Max(0,StaticBodyDetourV85.Num()-StaticBodyDetourIndexV85));
    J->SetNumberField(TEXT("static_body_detour_blocked_seconds"),StaticBodyDetourBlockedV85);
    J->SetNumberField(TEXT("bypass_phase"),BypassPhaseV839);J->SetBoolField(TEXT("actual_backing_step"),bBackingStepV839);
    J->SetBoolField(TEXT("certified_infantry_passing_route"),bBypassInfantryPassingV84412&&IsOperational()&&!IsImmobilized()&&!IsHeldForFieldRepairV63());
    J->SetNumberField(TEXT("bypass_backing_cm"),BypassBackingTotalV839);J->SetNumberField(TEXT("bypass_backing_limit_cm"),BypassBackingLimitV839);
    J->SetBoolField(TEXT("infantry_yield_recovery"),HasInfantryYieldRecoveryV84411(BypassInfantryV84411.Get()));
    if(const auto* U=BypassInfantryV84411.Get())
    {J->SetNumberField(TEXT("recovery_squad"),U->SquadId);J->SetNumberField(TEXT("recovery_member"),U->MemberIndex);
        J->SetStringField(TEXT("recovery_start_feet"),BypassInfantryStartV84411.ToString());
        FString ReviewUserV86;
        const bool PrivateReviewV86=Battle&&Battle->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrench"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FrontlineUltimateArchiveV8215::ReviewUserDir(ReviewUserV86);
        if(PrivateReviewV86)
        {
            FVector End=FVector::ZeroVector;J->SetBoolField(TEXT("passing_route_finishes_within_field"),StraightInfantryPassWithinFieldV86(U,&End));
            J->SetStringField(TEXT("passing_route_terminal"),End.ToString());
            J->SetStringField(TEXT("passing_owner_actual_feet"),U->Feet().ToString());
            J->SetStringField(TEXT("passing_owner_certified_feet"),BypassPassFeetV86.ToString());
            J->SetNumberField(TEXT("passing_certified_heading_deg"),BypassPassHeadingV86);
            J->SetNumberField(TEXT("passing_actual_rear_progress_cm"),RearProgressV49());
            TArray<TSharedPtr<FJsonValue>> Attempts;
            for(const auto& Item:BypassInfantryAttemptsV84411)if(const auto* Person=Item.Key.Get())
            {
                auto A=MakeShared<FJsonObject>();A->SetNumberField(TEXT("squad"),Person->SquadId);A->SetNumberField(TEXT("member"),Person->MemberIndex);
                A->SetStringField(TEXT("attempt_feet"),Item.Value.ToString());A->SetStringField(TEXT("actual_feet"),Person->Feet().ToString());
                A->SetNumberField(TEXT("distance_from_attempt_cm"),FVector::Dist2D(Person->Feet(),Item.Value));Attempts.Add(MakeShared<FJsonValueObject>(A));
            }
            J->SetArrayField(TEXT("attempted_yield_bodies"),Attempts);
        }
        if(Battle&&Battle->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"))
            &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
        {
            J->SetStringField(TEXT("recovery_actual_feet"),U->Feet().ToString());
            J->SetNumberField(TEXT("recovery_remaining_cm"),BypassRemainingV839);
            J->SetNumberField(TEXT("recovery_blocked_seconds"),BypassBlockedSecondsV839);
            J->SetNumberField(TEXT("recovery_attempted_bodies"),BypassInfantryAttemptsV84411.Num());
            if(auto Step=FrontlineReadArmourStepV84411(U))J->SetObjectField(TEXT("recovery_actual_step_receipt"),Step);
            FVector Q;const bool Lease=FrontlineReadArmourYieldV84411(U,Q);J->SetBoolField(TEXT("recovery_has_side_lease"),Lease);
            if(Lease)
            {J->SetStringField(TEXT("recovery_side_station"),Q.ToString());J->SetNumberField(TEXT("recovery_to_station_cm"),FVector::Dist2D(U->Feet(),Q));
                J->SetBoolField(TEXT("recovery_direct_route_clear"),FrontlineArmourYieldDirectPathClearV84411(U,Q));}
            const float DT=FMath::Max(1.f/120.f,MobilityStepV839);
            J->SetBoolField(TEXT("recovery_next_forward_blocks_owner"),BlocksInfantryAtV8218(U->Feet(),GetActorLocation()
                +FRotator(0,HeadingYawV37,0).Vector()*MobilityRequestedV839,HeadingYawV37,34.f,DT,U));
        }}
    FString DetourReviewUserV86;
    if(StaticBodyDetourAttemptV86&&Battle&&Battle->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))&&FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrench"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(DetourReviewUserV86))
    {
        J->SetObjectField(TEXT("last_static_body_detour_attempt"),StaticBodyDetourAttemptV86);
        if(StaticBodyOpeningTrenchV86!=INDEX_NONE)
            if(const auto* Opening=CooperativeOpeningsV86.Find(StaticBodyDetourOwnerV85))
                if(const auto* Unit=StaticBodyDetourOwnerV85.Get())
                {
                    auto Receipt=MakeShared<FJsonObject>();
                    Receipt->SetNumberField(TEXT("squad"),Unit->SquadId);Receipt->SetNumberField(TEXT("member"),Unit->MemberIndex);
                    Receipt->SetStringField(TEXT("actual_feet"),Unit->Feet().ToString());Receipt->SetStringField(TEXT("leg"),Opening->Leg.ToString());
                    Receipt->SetStringField(TEXT("original_goal"),Opening->Goal.ToString());Receipt->SetStringField(TEXT("agreed_from"),Opening->From.ToString());
                    Receipt->SetNumberField(TEXT("leg_distance_cm"),FVector::Dist2D(Unit->Feet(),Opening->Leg));
                    Receipt->SetNumberField(TEXT("goal_distance_cm"),FVector::Dist2D(Unit->Feet(),Opening->Goal));
                    Receipt->SetNumberField(TEXT("motion_speed_cm_s"),Unit->GetMotionSpeed());Receipt->SetBoolField(TEXT("acknowledged"),Opening->Acknowledged);
                    FVector CurrentGoal=FVector::ZeroVector,CurrentLeg=FVector::ZeroVector,Guide=FVector::ZeroVector;
                    Receipt->SetBoolField(TEXT("held_assignment_valid"),FrontlineReadHeldArmourLegV86(Unit,this,CurrentGoal,CurrentLeg));
                    Receipt->SetStringField(TEXT("actual_goal"),CurrentGoal.ToString());Receipt->SetStringField(TEXT("actual_router_leg"),CurrentLeg.ToString());
                    Receipt->SetBoolField(TEXT("read_guide"),ReadCooperativeOpeningGuideV86(Unit,Opening->Goal,Guide));Receipt->SetStringField(TEXT("read_guide_point"),Guide.ToString());
                    Receipt->SetBoolField(TEXT("leg_current_clear"),FrontlineArmourYieldDirectPathClearV84411(Unit,Opening->Leg));
                    Receipt->SetBoolField(TEXT("remaining_route_current_clear"),FrontlineCooperativeFloorRouteClearV86(Unit,this,Opening->FloorPath));
                    TArray<TSharedPtr<FJsonValue>> Remaining;
                    for(const FVector& Point:Opening->FloorPath)Remaining.Add(MakeShared<FJsonValueString>(Point.ToString()));
                    Receipt->SetArrayField(TEXT("remaining_floor_path"),Remaining);
                    if(auto Step=FrontlineReadArmourStepV84411(Unit))
                    {
                        auto Summary=MakeShared<FJsonObject>();
                        for(const TCHAR* Key:{TEXT("at_host_seconds"),TEXT("from"),TEXT("destination"),TEXT("guided"),TEXT("detour_active"),
                            TEXT("actor_yaw"),TEXT("result_gate"),TEXT("proposed_to"),TEXT("proposed_cm"),TEXT("actual_candidates"),
                            TEXT("first_route_body_reject"),TEXT("first_route_bank_reject"),TEXT("first_route_hull_reject")})
                            if(const auto* Value=Step->Values.Find(Key))Summary->SetField(Key,*Value);
                        Receipt->SetObjectField(TEXT("last_actual_step"),Summary);
                    }
                    J->SetObjectField(TEXT("cooperative_opening_state"),Receipt);
                }
    }
    J->SetNumberField(TEXT("hull_integrity"),HullIntegrity);J->SetNumberField(TEXT("left_track"),TrackIntegrity[0]);J->SetNumberField(TEXT("right_track"),TrackIntegrity[1]);
    J->SetBoolField(TEXT("entered"),HasEnteredField());J->SetBoolField(TEXT("national_adapter"),bNationalGermanGroundV82);
    J->SetBoolField(TEXT("physical_occupancy_clear"),CanOccupyV48(GetActorLocation(),HeadingYawV37));
    J->SetNumberField(TEXT("mg_shots"),MachinegunShots);J->SetNumberField(TEXT("cannon_shots"),CannonShots);
    J->SetBoolField(TEXT("mounted_firing_ready"),bMobilityFiringReadyV8448);
    J->SetBoolField(TEXT("rolling_acquisition"),bRollingAcquisitionV8448);
    J->SetBoolField(TEXT("feasible_mounted_traverse"),bMobilityTraversingV8448);
    J->SetNumberField(TEXT("left_running_gear_phase"),TrackSideOffsetV8311[0]);J->SetNumberField(TEXT("right_running_gear_phase"),TrackSideOffsetV8311[1]);
    J->SetNumberField(TEXT("front_wheel_steer_deg"),WheelSteerV8311);J->SetNumberField(TEXT("moving_steering_cap_deg_s"),SteeringRateV8311(UFrontlineVehicleSubsystem::Definition(Type).SpeedCm,false));
    if(Body&&Body->GetStaticMesh())
    {J->SetStringField(TEXT("actual_body"),Body->GetStaticMesh()->GetPathName());J->SetStringField(TEXT("actual_body_bounds"),Body->Bounds.GetBox().ToString());}
    FString ArcReviewUserV86;
    const bool PrivateArcV86=FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrench"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(ArcReviewUserV86);
    if(Battle&&Battle->IsProgressionVolatile()&&(FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"))||PrivateArcV86)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&!bNationalGermanGroundV82)
    {
        // Read the same three finite steering/forward arcs submitted by
        // Advance. These receipts cannot grant a route or alter a vehicle.
        const FVector Before=GetActorLocation();
        const float DT=FMath::Max(1.f/120.f,MobilityStepV839),Distance=FMath::Max(0.f,MobilityRequestedV839);
        const float MaxTurn=SteeringRateV8311(Distance/DT,false)*DT,Dir=Battle->GetTeamDirection(Team);
        TArray<TSharedPtr<FJsonValue>> Arcs;
        for(float Offset:{0.f,-MaxTurn,MaxTurn})
        {
            const float Yaw=HeadingYawV37+Offset;
            const FVector Facing=FRotator(0,Yaw,0).Vector(),Candidate=Before+Facing*Distance;
            const bool StaticClear=CanOccupyV48(Before,Yaw,DT);
            bool Clear=StaticClear&&Facing.X*Dir>=.10f&&FMath::Abs(Candidate.Y)<=980.f;
            const int32 Samples=FMath::Max(1,FMath::CeilToInt(Distance/20.f));
            for(int32 I=1;I<=Samples&&Clear;++I)
                Clear=CanOccupyV48(FMath::Lerp(Before,Candidate,double(I)/Samples),Yaw,DT*I/Samples);
            auto Arc=MakeShared<FJsonObject>();Arc->SetNumberField(TEXT("yaw_deg"),Yaw);
            Arc->SetBoolField(TEXT("static_occupancy_clear"),StaticClear);Arc->SetBoolField(TEXT("forward_arc_clear"),Clear);
            Arc->SetStringField(TEXT("candidate"),Candidate.ToString());Arcs.Add(MakeShared<FJsonValueObject>(Arc));
        }
        J->SetArrayField(TEXT("actual_bounded_forward_arcs"),Arcs);
    }
    return J;
}
bool AFrontlineTank::CanOccupyV48(const FVector& Position,float Yaw,float MotionSeconds,const FTransform* FromPose,const FTransform* SupportedPose)const
{
    const auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld());if(!Fleet)return false;
    if(Battle)for(const AWarfareFrontlineSoldier* U:Battle->Units)
        if(IsValid(U)&&U->IsAlive()&&!U->IsHidden()
            &&FVector::DistSquared2D(U->Feet(),Position)<FMath::Square(FootprintHalfLengthV37()+350.f)
            &&BlocksInfantryAtV8218(U->Feet(),Position,Yaw,34.f,MotionSeconds,U,FromPose,SupportedPose)
            &&(bNationalGermanGroundV82||!RelievesExistingInfantryContactV842(U->Feet(),Position,Yaw,34.f,MotionSeconds,U,FromPose,SupportedPose)))return false;
    // UK/DE vehicle hulls are soft routing obstacles. Infantry contact above
    // remains authoritative; a dense armour queue must not become a hard wall.
    // Preserve the separately boxed national-ground contract unchanged.
    if(!bNationalGermanGroundV82)return true;
    const FVector2D Ax(FMath::Cos(FMath::DegreesToRadians(Yaw)),FMath::Sin(FMath::DegreesToRadians(Yaw))),Ay(-Ax.Y,Ax.X);
    for(const AFrontlineTank* Other:Fleet->GetVehicles())if(IsValid(Other)&&Other!=this&&Other->IsOperational())
    {
        const FVector2D Delta(Other->GetActorLocation().X-Position.X,Other->GetActorLocation().Y-Position.Y);
        if(Delta.SizeSquared()>FMath::Square(FootprintHalfLengthV37()+Other->FootprintHalfLengthV37()+600.f))continue;
        const float Angle=FMath::DegreesToRadians(float(Other->GetActorRotation().Yaw));const FVector2D Bx(FMath::Cos(Angle),FMath::Sin(Angle)),By(-Bx.Y,Bx.X);
        bool Separated=false;
        for(const FVector2D& Axis:{Ax,Ay,Bx,By})
        {
            const float A=FootprintHalfLengthV37()*FMath::Abs(FVector2D::DotProduct(Axis,Ax))+(bRailConvoyV32?155.f:225.f)*FMath::Abs(FVector2D::DotProduct(Axis,Ay));
            const float B=Other->FootprintHalfLengthV37()*FMath::Abs(FVector2D::DotProduct(Axis,Bx))+(Other->IsRailConvoyV32()?155.f:225.f)*FMath::Abs(FVector2D::DotProduct(Axis,By));
            if(FMath::Abs(FVector2D::DotProduct(Delta,Axis))>=A+B+12.f){Separated=true;break;}
        }
        if(!Separated)return false;
    }
    return true;
}
bool AFrontlineTank::ArmourSightV48(const AFrontlineTank* Target,FVector& OutPoint)const
{
    if(!Battle||!IsValid(Target))return false;
    // Test stable physical gun pivots, not an animated muzzle or a global-X
    // facing test. A pitched hull's centre alone can be hidden by a trench lip
    // while a real upper hull panel is exposed to one of the sponsons.
    const float Heights[]={160.f,200.f,120.f};
    const auto Sight=SightParams(GetWorld(),this,Target);
    const TArray<FVector>& Mounts=CannonMounts.IsEmpty()?GunMounts:CannonMounts;
    for(float Height:Heights)
    {
        const FVector Point=Target->GetActorTransform().TransformPosition(FVector(0,0,Height));
        for(int32 I=0;I<FMath::Max(1,Mounts.Num());++I)
        {
            const FVector From=Mounts.IsValidIndex(I)?GetActorTransform().TransformPosition(Mounts[I]):AimPoint();
            if(Battle->DirectFireMultiplier(From,Point)>=.9f&&ClearSight(GetWorld(),this,Target,From,Point,&Sight))
            {OutPoint=Point;return true;}
        }
    }
    return false;
}
bool AFrontlineTank::ArmourFiringSolutionV8219(const AFrontlineTank* Target,int32 Mount,FVector& OutPoint,bool ForAiming,float* OutMuzzleDistance)const
{
    if(!Battle||!IsValid(Target)||Target==this||Target->Team==Team||!Target->IsOperational()
        ||!Target->HasEnteredField()||!CannonMounts.IsValidIndex(Mount)||!RailCannonOperationalV837(Mount))return false;
    const float Range=FMath::Min(6500.f,UFrontlineVehicleSubsystem::Definition(Type).RangeCm*Battle->EncounterVisibilityScale());
    TOptional<FCollisionQueryParams> Sight;
    // Observation and discharge share the target's real exposed hull panels.
    // A lip hiding its centre must not discard an upper panel that made this
    // mount hold its firing position. Each panel keeps the actual muzzle gates.
    for(const float Height:{160.f,200.f,120.f})
    {
        const FVector Point=Target->GetActorTransform().TransformPosition(FVector(0,0,Height));
        const FRotator Bearing=(GetActorTransform().InverseTransformPosition(Point)-CannonMounts[Mount]).Rotation();
        const bool LongRail=bRailConvoyV32&&Cannons.IsValidIndex(Mount)&&Cannons[Mount]->GetStaticMesh()
            &&Cannons[Mount]->GetStaticMesh()->GetName()==TEXT("SM_RailCannon_V837");
        const FVector Muzzle=ForAiming?GetActorTransform().TransformPosition(CannonMounts[Mount]+Bearing.Vector()*(LongRail?217.f:115.f)):WeaponMuzzle(Mount,true);
        if(FVector::Dist2D(Muzzle,Point)<Range
            &&FMath::Abs(FMath::FindDeltaAngleDegrees(CannonBaseYaw[Mount],float(Bearing.Yaw)))<=CannonTraverseV837(this)
            &&Bearing.Pitch>=-12.f&&Bearing.Pitch<=18.f
            &&Battle->DirectFireMultiplier(Muzzle,Point)>=.9f)
        {
            if(!Sight.IsSet())Sight.Emplace(SightParams(GetWorld(),this,Target));
            if(ClearSight(GetWorld(),this,Target,Muzzle,Point,&Sight.GetValue()))
            {OutPoint=Point;if(OutMuzzleDistance)*OutMuzzleDistance=float(FVector::Dist2D(Muzzle,Point));return true;}
        }
    }
    return false;
}
AFrontlineTank* AFrontlineTank::SelectArmourContactV84414(float& OutDistance,FVector& OutPoint)const
{
    if(!Battle||!bEntered)return nullptr;
    const auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld());if(!Fleet)return nullptr;
    const float Range=UFrontlineVehicleSubsystem::Definition(Type).RangeCm*Battle->EncounterVisibilityScale();
    OutDistance=Range;OutPoint=FVector::ZeroVector;
    AFrontlineTank* Selected=nullptr;int32 BestReadiness=-1;float BestScore=MAX_flt;
    for(AFrontlineTank* Other:Fleet->GetVehicles())
    {
        if(!IsValid(Other)||Other==this||Other->IsHidden()||!Other->IsOperational()
            ||!Other->HasEnteredField()||Other->Team==Team)continue;
        float Distance=bRailConvoyV32?ObservedVehicleDistanceV837(this,Other->AimPoint(),true):float(FVector::Dist2D(AimPoint(),Other->AimPoint()));
        if(!bRailConvoyV32&&Distance>Range+FootprintHalfLengthV37()+Other->FootprintHalfLengthV37()+200.f)continue;
        FVector Point=FVector::ZeroVector;
        if(bNationalGermanGroundV82||LegacyContactChoiceV84414)
        {
            if(Distance<OutDistance&&ArmourSightV48(Other,Point))
            {Selected=Other;OutDistance=Distance;OutPoint=Point;}
            continue;
        }
        // The same real mounted solution owns selection, stopping and fire.
        // A nearer hull-eye observation cannot displace a usable cannon target,
        // and a forward muzzle's valid range cannot be vetoed by root distance.
        bool Mounted=false;float MountedDistance=MAX_flt;FVector MountedPoint=FVector::ZeroVector;
        for(int32 I=0;I<CannonMounts.Num();++I)
        {
            FVector P;float D=MAX_flt;if(!ArmourFiringSolutionV8219(Other,I,P,true,&D))continue;
            if(D<MountedDistance){Mounted=true;MountedDistance=D;MountedPoint=P;}
        }
        int32 Readiness=Mounted?2:0;
        if(Mounted){Point=MountedPoint;Distance=MountedDistance;}
        else
        {
            if(Distance>=Range||!ArmourSightV48(Other,Point))continue;
            float Heading=HeadingYawV37;
            if(!Cannons.IsEmpty()&&MountedHeadingV8448(Point,Heading,true,Other))Readiness=1;
        }
        // Finish a physically feasible clearance task before choosing another
        // equally unaligned rear contact. Otherwise neighbour readiness flips
        // cancel the unwind and restart the blocked pivot every few frames.
        // A usable mounted target (readiness 2) can still supersede this task.
        const bool CommittedRecovery=Readiness==1&&bArmourClearanceV851&&Other==ArmourClearanceTargetV851.Get();
        const float Score=CommittedRecovery?-MAX_flt:Distance-(Other==ArmourContactV48.Get()?140.f:0.f);
        if(Readiness>BestReadiness||(Readiness==BestReadiness&&Score<BestScore))
        {BestReadiness=Readiness;BestScore=Score;Selected=Other;OutDistance=Distance;OutPoint=Point;}
    }
    return Selected;
}
bool AFrontlineTank::MountedHeadingV8448(const FVector& Target,float& OutHeading,bool CannonOnly,const AActor* TargetActor)const
{
    if(!Battle||Target.ContainsNaN())return false;
    float Shortest=MAX_flt;bool Found=false;
    const auto Sight=SightParams(GetWorld(),this,TargetActor);
    FVector Points[3]={Target,Target,Target};int32 PointCount=1;
    if(const auto* Person=Cast<AWarfareFrontlineSoldier>(TargetActor))
    {
        Points[0]=FrontlineMachinegunsV30::SightTarget(Person);Points[1]=Person->Soldier?
            Person->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5):Person->Feet()+FVector(0,0,145);PointCount=2;
    }
    else if(const auto* Armour=Cast<AFrontlineTank>(TargetActor))
    {int32 I=0;for(float H:{160.f,200.f,120.f})Points[I++]=Armour->GetActorTransform().TransformPosition(FVector(0,0,H));PointCount=3;}
    const auto Consider=[&](const FVector& Point,const FVector& Mount,float BaseYaw,float Sector,bool Cannon)
    {
        const auto Usable=[&](const FTransform& Pose)
        {
            const FRotator Bearing=(Pose.InverseTransformPosition(Point)-Mount).Rotation();
            if(FMath::Abs(FMath::FindDeltaAngleDegrees(BaseYaw,float(Bearing.Yaw)))>Sector
                ||Bearing.Pitch<(Cannon?-12.f:-15.f)||Bearing.Pitch>18.f)return false;
            const FVector Muzzle=Pose.TransformPosition(Mount+Bearing.Vector()*(Cannon?115.f:IsWheeledV8218()?81.5f:72.f));
            return FVector::Dist2D(Muzzle,Point)<MountedInfantryRangeV836(Battle,Type,Cannon)
                &&Battle->DirectFireMultiplier(Muzzle,Point)>=.9f
                &&ClearSight(GetWorld(),this,TargetActor,Muzzle,Point,&Sight);
        };
        // Prefer the rendered support pose: a gun which can aim from here
        // must not make its hull oscillate between competing future fits.
        if(Usable(GetActorTransform())){Found=true;Shortest=0.f;OutHeading=HeadingYawV37;return;}
        float Heading=HeadingYawV37;
        for(int32 Iteration=0;Iteration<4;++Iteration)
        {
            const FTransform Pose=GroundPoseForContactV8218(GetActorLocation(),Heading,0.f);
            const float Bearing=float((Pose.InverseTransformPosition(Point)-Mount).Rotation().Yaw);
            const float Delta=FMath::FindDeltaAngleDegrees(BaseYaw,Bearing);
            const float Correction=Delta-FMath::Clamp(Delta,-(Sector-5.f),Sector-5.f);
            Heading+=Correction;if(FMath::Abs(Correction)<.005f)break;
        }
        const float Turn=FMath::FindDeltaAngleDegrees(HeadingYawV37,Heading);
        if(!Usable(GroundPoseForContactV8218(GetActorLocation(),Heading,0.f)))return;
        if(FMath::Abs(Turn)<Shortest){Found=true;Shortest=FMath::Abs(Turn);OutHeading=Heading;}
    };
    for(int32 P=0;P<PointCount;++P)
    {
        for(int32 I=0;I<CannonMounts.Num();++I)
            if(RailCannonOperationalV837(I))Consider(Points[P],CannonMounts[I],CannonBaseYaw[I],CannonTraverseV837(this)-5.f,true);
        if(!CannonOnly)for(int32 I=0;I<GunMounts.Num();++I)
            if(GunIntegrity.IsValidIndex(I)&&GunIntegrity[I]>0.f)Consider(Points[P],GunMounts[I],GunBaseYaw[I],bNationalGermanGroundV82?55.f:60.f,false);
    }
    return Found;
}
bool AFrontlineTank::FaceArmourV48(const FVector& Target,float Seconds,bool CannonOnly,const AActor* TargetActor)
{
    // Retain each mount's authored traverse and seek its nearest firing
    // heading. A needless nose-on sweep can be blocked by a neighbouring hull
    // even when a smaller, physically clear turn would cover the contact.
    float TargetYaw=HeadingYawV37,ShortestTurn=MAX_flt;
    const auto Consider=[&](float BaseYaw,const FVector& Mount,float Sector)
    {
        const FRotator Aim=(GetActorTransform().InverseTransformPosition(Target)-Mount).Rotation();
        const float Delta=FMath::FindDeltaAngleDegrees(BaseYaw,float(Aim.Yaw));
        if(FMath::Abs(Delta)<=Sector)return true;
        const float Turn=Delta-FMath::Clamp(Delta,-(Sector-5.f),Sector-5.f);
        if(FMath::Abs(Turn)<ShortestTurn){ShortestTurn=FMath::Abs(Turn);TargetYaw=HeadingYawV37+Turn;}
        return false;
    };
    if(!bNationalGermanGroundV82&&!LegacyContactTurnV8448)
    {
        const auto* Armour=Cast<AFrontlineTank>(TargetActor);
        if(!CannonOnly||!IsValid(Armour)||ArmourClearanceTargetV851.Get()!=Armour)
        {bArmourClearanceV851=false;ArmourClearanceTargetV851.Reset();bArmourClearanceReverseV851=false;}
        bAcquisitionFeasibleV8448=MountedHeadingV8448(Target,TargetYaw,CannonOnly,TargetActor);
        if(!bAcquisitionFeasibleV8448)
        {bArmourClearanceV851=false;ArmourClearanceTargetV851.Reset();bArmourClearanceReverseV851=false;return false;}
        ShortestTurn=FMath::Abs(FMath::FindDeltaAngleDegrees(HeadingYawV37,TargetYaw));
        if(ShortestTurn<.01f)
        {bArmourClearanceV851=false;ArmourClearanceTargetV851.Reset();bArmourClearanceReverseV851=false;return true;}
        const float ForwardYaw=Battle->GetTeamDirection(Team)>0?0.f:180.f;
        const float ForwardTurn=FMath::Abs(FMath::FindDeltaAngleDegrees(ForwardYaw,TargetYaw));
        if(!LegacyForwardOnlyAcquisitionV85&&CannonOnly&&!IsWheeledV8218()&&!bRailConvoyV32
            &&!IsImmobilized()&&ForwardTurn>60.f)
        {
            const auto TurnClear=[&](float EndYaw)
            {
                const float Delta=FMath::FindDeltaAngleDegrees(HeadingYawV37,EndYaw);
                const int32 Count=FMath::Max(1,FMath::CeilToInt(FMath::Abs(Delta)/3.f));
                for(int32 I=1;I<=Count;++I)
                {
                    const float Yaw=HeadingYawV37+Delta*I/Count;
                    if(!CanOccupyV48(GetActorLocation(),Yaw,Seconds*I/Count)
                        ||!ArmourPivotHullClearV85(this,GroundPoseForContactV8218(GetActorLocation(),Yaw,Seconds*I/Count)))return false;
                }
                return true;
            };
            // A blocked partial pivot previously fell back to the ordinary
            // forward route, which undid the turn on the following frame.
            // Retain the observed contact while making physical turning room.
            // Do not retry the partial pivot until its entire remaining sweep
            // is clear: otherwise a dense formation oscillates forever.
            if(bArmourClearanceV851)
            {
                if(TurnClear(TargetYaw)){bArmourClearanceV851=false;ArmourClearanceTargetV851.Reset();bArmourClearanceReverseV851=false;}
                else{bArmourClearanceStepV851=true;return false;}
            }
            // The rolling planner deliberately stays inside the army's forward
            // hemisphere. A rear contact can be a valid cannon acquisition but
            // can never reach its selected yaw through that clamped route.
            // Tracked running gear may pivot here using its existing stationary
            // steering rate. Every partial turn retains the full physical body
            // and damped support checks; this grants no reverse translation.
            const float NextYaw=FMath::FixedTurn(HeadingYawV37,TargetYaw,SteeringRateV8311(0.f,true)*Seconds);
            const float Turn=FMath::FindDeltaAngleDegrees(HeadingYawV37,NextYaw);
            const int32 Steps=FMath::Max(1,FMath::CeilToInt(FMath::Abs(Turn)/3.f));
            for(int32 I=1;I<=Steps;++I)
            {
                const float Yaw=HeadingYawV37+Turn*I/Steps,Age=Seconds*I/Steps;
                if(!CanOccupyV48(GetActorLocation(),Yaw,Age)
                    ||!ArmourPivotHullClearV85(this,GroundPoseForContactV8218(GetActorLocation(),Yaw,Age)))
                {bArmourClearanceV851=bArmourClearanceStepV851=true;ArmourClearanceTargetV851=const_cast<AFrontlineTank*>(Armour);return false;}
            }
            if(FMath::Abs(Turn)>.01f)bBypassInfantryPassingV84412=false;
            HeadingYawV37=NextYaw;FitGroundV35(GetActorLocation(),Seconds);
            return false;
        }
        // Future support authorizes a checked rolling approach, never a
        // stationary whole-hull pivot without a present usable muzzle.
        bArmourClearanceV851=false;ArmourClearanceTargetV851.Reset();bArmourClearanceReverseV851=false;
        bRollingAcquisitionV8448=true;RollingAcquisitionYawV8448=TargetYaw;
        return false;
    }
    else
    {
        for(int32 I=0;I<CannonMounts.Num();++I)
            if(RailCannonOperationalV837(I)&&Consider(CannonBaseYaw[I],CannonMounts[I],CannonTraverseV837(this)-5.f))return true;
        if(CannonMounts.IsEmpty())for(int32 I=0;I<GunMounts.Num();++I)if(GunIntegrity[I]>0)
            if(Consider(GunBaseYaw[I],GunMounts[I],bNationalGermanGroundV82?55.f:60.f))return true;
    }
    if(IsImmobilized()||bRailConvoyV32)return true;
    if(ShortestTurn==MAX_flt)return false;
    if(!bNationalGermanGroundV82&&IsWheeledV8218())return false;
    if(bNationalGermanGroundV82)
    {
        const float Next=FMath::FixedTurn(HeadingYawV37,TargetYaw,32.f*Seconds);
        return CommitNationalGroundV82(GetActorLocation(),Next,0);
    }
    const float NextYaw=FMath::FixedTurn(HeadingYawV37,TargetYaw,SteeringRateV8311(0.f,true)*Seconds);
    const float Turn=FMath::FindDeltaAngleDegrees(HeadingYawV37,NextYaw);
    // Sweep this frame's bounded turn. A neighbour obstructing a later pose
    // must not veto a presently clear small turn and pin both hulls forever.
    const int32 Steps=FMath::Max(1,FMath::CeilToInt(FMath::Abs(Turn)/3.f));
    for(int32 I=1;I<=Steps;++I)
        if(!CanOccupyV48(GetActorLocation(),HeadingYawV37+Turn*I/Steps,Seconds*I/Steps))return false;
    if(FMath::Abs(Turn)>.01f)bBypassInfantryPassingV84412=false;
    HeadingYawV37=NextYaw;
    FitGroundV35(GetActorLocation(),Seconds);
    return true;
}
float AFrontlineTank::AdvanceArmourClearanceV851(float Distance,float Seconds)
{
    bBackingStepV839=false;
    if(!Battle||!bArmourClearanceV851||Distance<=0.f||Seconds<=0.f||IsImmobilized()
        ||IsHeldForFieldRepairV63()||bNationalGermanGroundV82||bRailConvoyV32||IsWheeledV8218())return 0.f;
    const auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld());if(!Fleet)return 0.f;
    const FVector Before=GetActorLocation();const float ForwardYaw=Battle->GetTeamDirection(Team)>0?0.f:180.f;
    const bool HoldEscapeHeading=bArmourClearanceReverseV851||FMath::Abs(FMath::FindDeltaAngleDegrees(HeadingYawV37,ForwardYaw))>60.f;
    const float NextYaw=FMath::FixedTurn(HeadingYawV37,ForwardYaw,SteeringRateV8311(0.f,true)*Seconds);
    const float Turn=FMath::FindDeltaAngleDegrees(HeadingYawV37,NextYaw);
    const int32 Turns=FMath::Max(1,FMath::CeilToInt(FMath::Abs(Turn)/3.f));
    bool UnwindClear=!HoldEscapeHeading;
    for(int32 I=1;I<=Turns&&UnwindClear;++I)
    {
        const float Yaw=HeadingYawV37+Turn*I/Turns,Age=Seconds*I/Turns;
        if(!CanOccupyV48(Before,Yaw,Age)||!ArmourPivotHullClearV85(this,GroundPoseForContactV8218(Before,Yaw,Age)))
        {UnwindClear=false;break;}
    }
    if(UnwindClear){HeadingYawV37=NextYaw;FitGroundV35(Before,Seconds);}
    const float ForwardError=FMath::Abs(FMath::FindDeltaAngleDegrees(HeadingYawV37,ForwardYaw));
    if(UnwindClear&&ForwardError>1.f)return 0.f;
    // A neighbour may prevent the next small unwind while leaving the current
    // heading's forward path clear. Requiring a completely straight hull first
    // made that legal exit unreachable. Keep the accepted heading and sweep a
    // short step. Do not translate diagonally while the stationary unwind is
    // still feasible: that gradually funnels separate vehicle lanes together.
    Distance=FMath::Min(Distance,12.f);
    struct FClearanceStepV851{int32 Direction=0;float Distance=0.f;FVector Position=FVector::ZeroVector;};
    const auto SafeStep=[&](const AFrontlineTank* V,float Yaw,float Budget)
    {
        FClearanceStepV851 Step;
        const FVector From=V->GetActorLocation(),Axis=FRotator(0,Yaw,0).Vector();
        // Prefer forward running gear. Reverse is a local escape from a real
        // blocked forward path, never an unbounded new advance order.
        for(const int32 Direction:{1,-1})
        {
            // Once a backing escape starts, do not drive into the same blocked
            // pose on the very next frame merely because that short step is now
            // clear. Release the direction only when turning space is regained.
            const float ArmyYaw=Battle->GetTeamDirection(V->Team)>0?0.f:180.f;
            if(Direction>0&&(V->bArmourClearanceReverseV851||FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw,ArmyYaw))>60.f))continue;
            const float Remaining=FMath::Max(0.f,FMath::Max(300.f,V->FootprintHalfLengthV37()*2.f)-V->ArmourClearanceBackingV851);
            const float Length=Direction>0?Budget:FMath::Min(Budget,Remaining);
            if(Length<=.01f)continue;
            const FVector To=From+Axis*(Direction*Length);
            const int32 Count=FMath::Max(1,FMath::CeilToInt(Length/4.f));bool Clear=true;
            for(int32 I=1;I<=Count&&Clear;++I)
            {
                const FVector P=FMath::Lerp(From,To,double(I)/Count);const float Age=Seconds*I/Count;
                Clear=FMath::Abs(P.Y)<=980.f&&V->CanOccupyV48(P,Yaw,Age)
                    &&ArmourPivotHullClearV85(V,V->GroundPoseForContactV8218(P,Yaw,Age));
            }
            if(Clear){Step.Direction=Direction;Step.Distance=Length;Step.Position=To;return Step;}
        }
        return Step;
    };
    const FClearanceStepV851 Own=SafeStep(this,HeadingYawV37,Distance);if(!Own.Direction)return 0.f;
    // A vehicle without an executable path cannot hold its neighbours' lease.
    // Forward exits outrank backing; serial breaks ties between two actually
    // safe steps. Everyone may still unwind a clear stationary partial turn.
    for(const AFrontlineTank* Other:Fleet->GetVehicles())
        if(IsValid(Other)&&Other!=this&&Other->Team==Team&&Other->IsOperational()&&!Other->IsHidden()
            &&Other->bArmourClearanceV851&&!Other->IsImmobilized()&&!Other->IsHeldForFieldRepairV63()
            &&FVector::DistSquared2D(Other->GetActorLocation(),GetActorLocation())<FMath::Square(2200.f))
        {
            const float OtherForward=Battle->GetTeamDirection(Other->Team)>0?0.f:180.f;
            float OtherYaw=Other->HeadingYawV37;
            const float OtherNext=FMath::FixedTurn(OtherYaw,OtherForward,Other->SteeringRateV8311(0.f,true)*Seconds);
            const float OtherTurn=FMath::FindDeltaAngleDegrees(OtherYaw,OtherNext);
            const int32 Count=FMath::Max(1,FMath::CeilToInt(FMath::Abs(OtherTurn)/3.f));
            bool OtherUnwind=!Other->bArmourClearanceReverseV851&&FMath::Abs(FMath::FindDeltaAngleDegrees(OtherYaw,OtherForward))<=60.f;
            for(int32 I=1;I<=Count&&OtherUnwind;++I)
            {
                const float Yaw=OtherYaw+OtherTurn*I/Count,Age=Seconds*I/Count;
                OtherUnwind=Other->CanOccupyV48(Other->GetActorLocation(),Yaw,Age)
                    &&ArmourPivotHullClearV85(Other,Other->GroundPoseForContactV8218(Other->GetActorLocation(),Yaw,Age));
            }
            if(OtherUnwind)OtherYaw=OtherNext;
            const float Error=FMath::Abs(FMath::FindDeltaAngleDegrees(OtherYaw,OtherForward));
            if(OtherUnwind&&Error>1.f)continue;
            const FClearanceStepV851 Eligible=SafeStep(Other,OtherYaw,FMath::Min(Distance,12.f));
            if(Eligible.Direction&&(Eligible.Direction>Own.Direction||(Eligible.Direction==Own.Direction&&Other->Serial>Serial)))
            {ArmourClearanceLeaseOwnerV851=Other->Serial;return 0.f;}
        }
    bBypassInfantryPassingV84412=false;bBackingStepV839=Own.Direction<0;
    bArmourClearanceReverseV851=bBackingStepV839;
    FitGroundV35(Own.Position,Seconds);const float Moved=FVector::Dist2D(Before,GetActorLocation());
    if(bBackingStepV839)ArmourClearanceBackingV851+=Moved;
    return Moved;
}
bool AFrontlineTank::InfantryFiringSolutionV828(const AWarfareFrontlineSoldier* Target,int32 Mount,bool Cannon,FVector& OutPoint,const FCollisionQueryParams* Prepared,bool ForAiming)const
{
    if(!Battle||!Target||!Target->IsActiveCombatant()||Target->Team==Team)return false;
    const auto& Mounts=Cannon?CannonMounts:GunMounts;const auto& BaseYaw=Cannon?CannonBaseYaw:GunBaseYaw;
    if(!Mounts.IsValidIndex(Mount)||(Cannon&&!RailCannonOperationalV837(Mount))||(!Cannon&&(!GunIntegrity.IsValidIndex(Mount)||GunIntegrity[Mount]<=0)))return false;
    const float Range=WarfareFRRUArmourV82::National(this)?WarfareFRRUArmourV82::WeaponRangeV87(this,Cannon):MountedInfantryRangeV836(Battle,Type,Cannon);
    // Supported infantry can expose only a head over the parapet. Search real
    // body aim points once and reuse that exact point for aiming and firing.
    const FVector Points[]={FrontlineMachinegunsV30::SightTarget(Target),Target->Soldier?
        Target->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5):Target->Feet()+FVector(0,0,145)};
    for(const FVector& Point:Points)
    {
        const FRotator Bearing=(GetActorTransform().InverseTransformPosition(Point)-Mounts[Mount]).Rotation();
        const bool LongRail=Cannon&&bRailConvoyV32&&Cannons.IsValidIndex(Mount)&&Cannons[Mount]->GetStaticMesh()
            &&Cannons[Mount]->GetStaticMesh()->GetName()==TEXT("SM_RailCannon_V837");
        // A future physical muzzle authorizes barrel aiming only. Discharge
        // still requires the actual rendered muzzle after alignment.
        const float Barrel=Cannon?(LongRail?217.f:115.f):IsWheeledV8218()?81.5f:72.f;
        const FVector Muzzle=ForAiming?GetActorTransform().TransformPosition(Mounts[Mount]+Bearing.Vector()*Barrel):WeaponMuzzle(Mount,Cannon);
        if(FVector::Dist2D(Muzzle,Point)>Range||(!bRailConvoyV32&&FVector::Dist2D(GetActorLocation(),Point)>Range)
            ||FMath::Abs(FMath::FindDeltaAngleDegrees(BaseYaw[Mount],float(Bearing.Yaw)))>(Cannon?CannonTraverseV837(this):60.f)
            ||Bearing.Pitch<(Cannon?-12.f:-15.f)||Bearing.Pitch>18.f
            ||Battle->DirectFireMultiplier(Muzzle,Point)<.9f||!ClearSight(GetWorld(),this,Target,Muzzle,Point,Prepared))continue;
        OutPoint=Point;return true;
    }
    return false;
}
bool AFrontlineTank::HasInfantryYieldRecoveryV84411(const AWarfareFrontlineSoldier* U) const
{
    return U&&IsValid(U)&&U->IsAlive()&&!U->IsHidden()&&IsOperational()&&!IsHidden()
        &&!IsImmobilized()&&!IsHeldForFieldRepairV63()
        &&Battle&&U->Battle.Get()==Battle.Get()&&U->Team==Team
        &&!bNationalGermanGroundV82&&!bRailConvoyV32&&!WarfareFRRUCampaignV82::Active(Battle)
        &&!FrontlineUseLegacyArmourYieldV84411()&&(BypassPhaseV839==1||BypassPhaseV839==2)
        &&BypassInfantryV84411.Get()==U;
}
bool AFrontlineTank::StraightInfantryPassWithinFieldV86(const AWarfareFrontlineSoldier* Unit,FVector* End)const
{
    if(!Battle||!IsValid(Unit)||Unit->Battle.Get()!=Battle.Get()||Unit->Team!=Team
        ||!Body||!Body->GetStaticMesh()||GetActorTransform().ContainsNaN())return false;
    const int32 Dir=Battle->GetTeamDirection(Team);const FVector Position=GetActorLocation();
    const FVector Forward=FRotator(0,HeadingYawV37,0).Vector();const float Progress=Forward.X*Dir;
    // A straight station is useful only when the entire real rear can pass it
    // in the ordinary forward travel domain. A side station alone proved body
    // clearance, but could lock a long hull onto a route ending beyond its
    // field edge and force several seconds of stationary recovery there.
    if(!FMath::IsFinite(Progress)||Progress<.35f)return false;
    float BodyFront=float(Unit->Feet().X)*Dir+34.f;
    if(Unit->Soldier)
    {
        const FBox Person=Unit->Soldier->Bounds.GetBox();
        if(Person.IsValid)BodyFront=FMath::Max(BodyFront,float(Dir>0?Person.Max.X:-Person.Min.X));
    }
    const float Remaining=FMath::Max(0.f,BodyFront+3.f-RearProgressV49());
    const FVector Terminal=Position+Forward*(Remaining/Progress);
    if(End)*End=Terminal;
    if(Terminal.ContainsNaN()||FMath::Abs(Position.Y)>980.f||FMath::Abs(Terminal.Y)>980.f)return false;
    const float Speed=FMath::Max(1.f,UFrontlineVehicleSubsystem::Definition(Type).SpeedCm);
    const FTransform EndPose=GroundPoseForContactV8218(Terminal,HeadingYawV37,Remaining/Progress/Speed);
    const FTransform BodyLocal=Body->GetComponentTransform().GetRelativeTransform(GetActorTransform());
    const FBox EndBody=Body->GetStaticMesh()->GetBoundingBox().TransformBy(BodyLocal*EndPose);
    const FBox StartBody=Body->Bounds.GetBox();
    return !EndPose.ContainsNaN()&&StartBody.IsValid&&EndBody.IsValid
        &&StartBody.Min.Y>=-FrontlineBattlefield::HalfFrontage&&StartBody.Max.Y<=FrontlineBattlefield::HalfFrontage
        &&EndBody.Min.Y>=-FrontlineBattlefield::HalfFrontage&&EndBody.Max.Y<=FrontlineBattlefield::HalfFrontage;
}
void AFrontlineTank::RefreshInfantryPassCertificateV86()
{
    if(!bBypassInfantryPassingV84412)return;
    const auto* PassingInfantry=BypassInfantryV84411.Get();
    const auto* Held=Battle&&IsValid(PassingInfantry)&&PassingInfantry->Battle.Get()==Battle.Get()?Battle->FindSquad(PassingInfantry->SquadId):nullptr;
    bool Continue=Battle&&IsOperational()&&!IsImmobilized()&&!IsHeldForFieldRepairV63()
        &&IsValid(PassingInfantry)&&PassingInfantry->IsAlive()&&!PassingInfantry->IsHidden()&&PassingInfantry->Team==Team
        &&Held&&!Held->bAdvancing&&Battle->Trenches.IsValidIndex(Held->Trench)
        &&BypassPhaseV839==0&&!bNationalGermanGroundV82&&!bRailConvoyV32
        &&!FrontlineUseLegacyArmourYieldV84411()&&!WarfareFRRUCampaignV82::Active(Battle)
        &&RouteStallSecondsV837<.60f
        &&FVector::DistSquared2D(PassingInfantry->Feet(),BypassPassFeetV86)<=FMath::Square(15.f)
        &&FMath::Abs(FMath::FindDeltaAngleDegrees(BypassPassHeadingV86,HeadingYawV37))<=.5f
        &&StraightInfantryPassWithinFieldV86(PassingInfantry);
    if(Continue)
    {
        const int32 Dir=Battle->GetTeamDirection(Team);
        float BodyFront=float(PassingInfantry->Feet().X)*Dir+34.f;
        if(PassingInfantry->Soldier)
        {
            const FBox ActualBody=PassingInfantry->Soldier->Bounds.GetBox();
            if(ActualBody.IsValid)BodyFront=FMath::Max(BodyFront,float(Dir>0?ActualBody.Max.X:-ActualBody.Min.X));
        }
        Continue=RearProgressV49()<=BodyFront+3.f;
    }
    if(!Continue){bBypassInfantryPassingV84412=false;RouteDecisionAtV37=0;}
}

float AFrontlineTank::AdvanceV37(float Distance,float Seconds)
{
    bBackingStepV839=false;
    if(!Battle||!IsOperational()||IsImmobilized()||IsHeldForFieldRepairV63())bBypassInfantryPassingV84412=false;
    if(!Battle||Distance<=0||Seconds<=0||IsImmobilized())return 0;
    const int32 Dir=Battle->GetTeamDirection(Team);const FVector Before=GetActorLocation();
    if(!StaticBodyDetourV85.IsEmpty())
    {
        const auto* BlockingPerson=StaticBodyDetourOwnerV85.Get();
        const auto* OpeningHeld=IsValid(BlockingPerson)?Battle->FindSquad(BlockingPerson->SquadId):nullptr;
        const auto* OpeningReceipt=StaticBodyOpeningTrenchV86!=INDEX_NONE?CooperativeOpeningsV86.Find(StaticBodyDetourOwnerV85):nullptr;
        FVector OpeningGoal=FVector::ZeroVector,OpeningLeg=FVector::ZeroVector;
        const bool OpeningIdentityValid=StaticBodyOpeningTrenchV86==INDEX_NONE
            ||(OpeningReceipt&&OpeningReceipt->Encounter==Battle->GetEncounterId()
                &&FrontlineReadHeldArmourLegV86(BlockingPerson,this,OpeningGoal,OpeningLeg)
                &&FVector::DistSquared2D(OpeningGoal,OpeningReceipt->Goal)<=FMath::Square(.01f));
        if(!IsValid(BlockingPerson)||!BlockingPerson->IsAlive()||BlockingPerson->IsHidden()
            ||!OpeningIdentityValid
            ||(StaticBodyOpeningTrenchV86!=INDEX_NONE&&(!OpeningHeld||OpeningHeld->bAdvancing||OpeningHeld->Trench!=StaticBodyOpeningTrenchV86))
            ||FVector::DistSquared2D(BlockingPerson->Feet(),StaticBodyDetourOwnerFeetV85)>FMath::Square(80.f))
        {if(StaticBodyOpeningTrenchV86!=INDEX_NONE)CooperativeOpeningsV86.Remove(StaticBodyDetourOwnerV85);
            StaticBodyDetourV85.Reset();StaticBodyDetourIndexV85=0;StaticBodyOpeningTrenchV86=INDEX_NONE;RouteDecisionAtV37=0;}
        else return AdvanceStaticBodyDetourV85(Distance,Seconds);
    }
    if(StaticBodyOpeningTrenchV86!=INDEX_NONE)
    {
        const auto* OpeningUnitV86=StaticBodyDetourOwnerV85.Get();
        const auto* Held=IsValid(OpeningUnitV86)?Battle->FindSquad(OpeningUnitV86->SquadId):nullptr;
        auto* Opening=CooperativeOpeningsV86.Find(StaticBodyDetourOwnerV85);
        FVector CurrentGoal=FVector::ZeroVector,CurrentLeg=FVector::ZeroVector;
        const bool SameAssignment=Opening&&Opening->Encounter==Battle->GetEncounterId()&&Held&&!Held->bAdvancing&&Held->Trench==StaticBodyOpeningTrenchV86
            &&FrontlineReadHeldArmourLegV86(OpeningUnitV86,this,CurrentGoal,CurrentLeg)
            &&FVector::DistSquared2D(CurrentGoal,Opening->Goal)<=FMath::Square(.01f);
        const bool GuidedChanged=SameAssignment&&Opening->FloorPath.IsEmpty()
            &&FVector::DistSquared2D(CurrentLeg,Opening->Guided)>FMath::Square(.01f);
        // The existing floor router advances a reached waypoint within 2 cm.
        // That is a real completed route leg, not a changed order. Longer
        // truncated legs acknowledge only actual travel past their end plane.
        const bool ReachedRouteWaypoint=GuidedChanged&&FVector::DistSquared2D(Opening->Guided,Opening->Leg)<=FMath::Square(.01f)
            &&FVector::DistSquared2D(OpeningUnitV86->Feet(),Opening->Guided)<=FMath::Square(2.f);
        bool Acknowledged=SameAssignment&&(ReachedRouteWaypoint
            ||FVector::DistSquared2D(OpeningUnitV86->Feet(),Opening->Leg)<=FMath::Square(.01f));
        bool LegPhysicallyChanged=SameAssignment
            &&((!Acknowledged&&!FrontlineArmourYieldDirectPathClearV84411(OpeningUnitV86,Opening->Leg))
                ||(!Opening->FloorPath.IsEmpty()&&!FrontlineCooperativeFloorRouteClearV86(OpeningUnitV86,this,Opening->FloorPath)));
        if(Acknowledged&&!LegPhysicallyChanged&&!Opening->FloorPath.IsEmpty())
        {
            // Consume the proven route through real feet, not only its first
            // bend. Every next short guide retains the original goal/order;
            // normal vehicle motion cannot close the successor corridor.
            while(!Opening->FloorPath.IsEmpty()
                &&FVector::DistSquared2D(OpeningUnitV86->Feet(),Opening->FloorPath[0])<=FMath::Square(.01f))
                Opening->FloorPath.RemoveAt(0);
            if(!Opening->FloorPath.IsEmpty())
            {
                FVector Leg=Opening->FloorPath[0]-OpeningUnitV86->Feet();Leg.Z=0;
                FVector Next=OpeningUnitV86->Feet()+Leg.GetSafeNormal2D()*FMath::Min(24.,Leg.Size2D());
                Next.Z=FrontlineInfantryV45::SurfaceHeight(Battle,Next);
                if(FrontlineArmourYieldDirectPathClearV84411(OpeningUnitV86,Next))
                {Opening->From=OpeningUnitV86->Feet();Opening->Leg=Next;Opening->Guided=CurrentLeg;Acknowledged=false;}
                else LegPhysicallyChanged=true;
            }
        }
        if(Acknowledged)CooperativeOpeningsV86.FindChecked(StaticBodyDetourOwnerV85).Acknowledged=true;
        if(!SameAssignment||GuidedChanged||Acknowledged||LegPhysicallyChanged)
        {if(!SameAssignment||(GuidedChanged&&!Acknowledged)||LegPhysicallyChanged)CooperativeOpeningsV86.Remove(StaticBodyDetourOwnerV85);
            if(LegPhysicallyChanged)StaticBodyDetourRetryAtV85=0;
            StaticBodyOpeningTrenchV86=INDEX_NONE;RouteDecisionAtV37=0;}
        else
        {
            // Finishing the vehicle's centimetres is not the other actor's
            // acknowledgement. Keep the opened leg until his actual feet
            // complete it; normal lane travel must not eat the space back.
            RouteStallSecondsV837+=Seconds;
            return 0;
        }
    }
    // A reached station was certified for this actual heading's whole pass.
    // Re-ranking a different lane on the very next tick steered the hull back
    // through that station and chased the man into the next packed section.
    // Preserve the certified straight route only until the real rear passes
    // his real body, or the ordinary route feedback reports no progress.
    RefreshInfantryPassCertificateV86();
    bool CertifiedPassingV84412=bBypassInfantryPassingV84412;
    if(CertifiedPassingV84412)DesiredLaneV37=BypassLaneV839;
    if(bRailConvoyV32)
    {
        const FVector Candidate=Before+FVector(Dir*Distance,0,0);
        if(CanOccupyV48(Candidate,HeadingYawV37,Seconds))FitGroundV35(Candidate,Seconds);
        return FVector::Dist2D(Before,GetActorLocation());
    }
    if(!bNationalGermanGroundV82&&IsWheeledV8218()&&GetActorForwardVector().X*Dir<.45f)
        return AdvanceCombatArcV8311(Before+FVector(Dir*1800.f,0,0),Distance,Seconds);
    const auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld());
    if(!Fleet)return 0;
    const float RecoveryForwardYawV86=Dir>0?0.f:180.f;
    const float RecoveryHullAcrossV86=FootprintHalfLengthV37()*FMath::Abs(FMath::Sin(FMath::DegreesToRadians(HeadingYawV37)))
        +FootprintHalfWidthV8311()*FMath::Abs(FMath::Cos(FMath::DegreesToRadians(HeadingYawV37)));
    bool HeldRecoveryApproachV86=false;
    if(!bNationalGermanGroundV82&&!FrontlineUseLegacyArmourYieldV84411()&&!WarfareFRRUCampaignV82::Active(Battle))
        for(const AWarfareFrontlineSoldier* U:Battle->Units)
        {
            if(!IsValid(U)||!U->IsAlive()||U->IsHidden()||U->Team!=Team
                ||FMath::Abs(float(U->Feet().X-Before.X))>FootprintHalfLengthV37()+180.f
                ||FMath::Abs(float(U->Feet().Y-Before.Y))>RecoveryHullAcrossV86+180.f)continue;
            const auto* Held=Battle->FindSquad(U->SquadId);
            if(Held&&!Held->bAdvancing&&Battle->Trenches.IsValidIndex(Held->Trench)){HeldRecoveryApproachV86=true;break;}
        }
    if(HeldRecoveryApproachV86&&FRotator(0,HeadingYawV37,0).Vector().X*Dir<.35f)
    {
        // Lane and finite recovery use the same forward-facing domain. If a
        // prior manoeuvre faces farther across it, re-enter with the original
        // stationary turn and complete supported body/field predicate.
        const float Next=FMath::FixedTurn(HeadingYawV37,RecoveryForwardYawV86,SteeringRateV8311(0.f,true)*Seconds);
        if(StaticBodyDetourPoseClearV85(Before,Next,Seconds))
        {HeadingYawV37=Next;FitGroundV35(Before,Seconds);}
        return 0;
    }
    // A held body is a complete-path constraint at the first blocked contact.
    // Waiting for a greedy shoulder lease on a steep bank can lower the hull
    // into a supported pose with neither a forward nor a reverse first step.
    // Only the actual held-body witness in the finite planner can accept this;
    // terrain alone, moving infantry and soft armour do not acquire a route.
    if(!CertifiedPassingV84412&&BypassPhaseV839==0
        &&!CanOccupyV48(Before+FRotator(0,HeadingYawV37,0).Vector()*Distance,HeadingYawV37,Seconds)
        &&PrepareStaticBodyDetourV85(Distance/Seconds,Seconds))
        return AdvanceStaticBodyDetourV85(Distance,Seconds);
    if(!bNationalGermanGroundV82&&BypassPhaseV839)
    {
        const auto* Neighbour=BypassNeighbourV839.Get();
        auto* YieldOwner=BypassInfantryV84411.Get();
        const bool InfantryOwner=HasInfantryYieldRecoveryV84411(YieldOwner);
        if(!InfantryOwner&&(!IsValid(Neighbour)||!Neighbour->IsOperational()||Neighbour->Team!=Team))
        {BypassPhaseV839=0;BypassRemainingV839=BypassBlockedSecondsV839=0;BypassNeighbourV839.Reset();}
        else
        {
            DesiredLaneV37=BypassLaneV839;
            if(InfantryOwner)
            {
                // Reverse is a cooperative lease, not permission to resume the
                // same blocked nose-first route immediately after a short gap.
                // Keep the checked side station until this man actually reaches
                // it. Its safe cache is retained by the exact owner/phase guard.
                FVector Station;
                const bool SideReady=FrontlineMobilityV41::ArmourYieldDestinationV8218(YieldOwner,Station)
                    &&FVector::DistSquared2D(YieldOwner->Feet(),Station)<=FMath::Square(10.f)
                    &&FrontlineArmourYieldReachableStationClearV84411(YieldOwner,this,YieldOwner->Feet())
                    &&!BlocksInfantryAtV8218(YieldOwner->Feet(),Before+FRotator(0,HeadingYawV37,0).Vector()*Distance,
                        HeadingYawV37,34.f,Seconds,YieldOwner);
                if(SideReady&&StraightInfantryPassWithinFieldV86(YieldOwner))
                {BypassPhaseV839=0;BypassRemainingV839=BypassBlockedSecondsV839=0;RouteStallSecondsV837=0;
                    BypassPassFeetV86=YieldOwner->Feet();BypassPassHeadingV86=HeadingYawV37;
                    bBypassInfantryPassingV84412=CertifiedPassingV84412=true;RouteDecisionAtV37=0;}
                else if(BypassPhaseV839==2)
                {
                    // A reached shoulder is not proof that the long rear can
                    // finish its pass within the field. Keep the lease until
                    // a whole, physically checked exit has replaced it.
                    if(SideReady&&PrepareStaticBodyDetourV85(Distance/Seconds,Seconds,YieldOwner))
                        return AdvanceStaticBodyDetourV85(Distance,Seconds);
                    BypassBlockedSecondsV839+=Seconds;
                    if(BypassBlockedSecondsV839<1.5f)
                    {
                        // Keep the pose of a hull which already exposes a
                        // checked complete route to its yielding man. A legal
                        // turn at his current feet can still close a later leg.
                        // Only a physically blocked route prepares steering,
                        // with the original lane, angular budget and contact.
                        if(!FrontlineArmourYieldHasCheckedRouteV84411(YieldOwner))
                        {
                            const FVector RecoveryWish(Dir,FMath::Clamp((BypassLaneV839-float(Before.Y))/620.f,-.85f,.85f),0);
                            const float RecoveryYaw=FMath::FixedTurn(HeadingYawV37,float(RecoveryWish.Rotation().Yaw),
                                SteeringRateV8311(0.f,true)*Seconds);
                            if(FMath::Abs(FMath::FindDeltaAngleDegrees(HeadingYawV37,RecoveryYaw))>.01f
                                &&CanOccupyV48(Before,RecoveryYaw,Seconds))
                            {HeadingYawV37=RecoveryYaw;FitGroundV35(Before,Seconds);}
                        }
                        return 0;
                    }
                    // The finite reverse lease has failed. Return to normal
                    // forward/lane routing; this same unmoved body cannot start
                    // another reverse lease indefinitely.
                    BypassPhaseV839=0;BypassRemainingV839=BypassBlockedSecondsV839=0;RouteDecisionAtV37=0;
                }
            }
            if(BypassPhaseV839==1)
            {
                const float Step=FMath::Min3(Distance,BypassRemainingV839,20.f);
                const FVector Candidate=Before-FRotator(0,HeadingYawV37,0).Vector()*Step;
                bool Clear=FMath::Abs(Candidate.Y)<=980.f;
                const int32 Samples=FMath::Max(1,FMath::CeilToInt(Step/10.f));
                for(int32 I=1;I<=Samples&&Clear;++I)
                    Clear=CanOccupyV48(FMath::Lerp(Before,Candidate,double(I)/Samples),HeadingYawV37,Seconds*I/Samples);
                if(Clear&&Step>.01f)
                {
                    FitGroundV35(Candidate,Seconds);const float Moved=FVector::Dist2D(Before,GetActorLocation());
                    bBackingStepV839=Moved>.01f;BypassBackingTotalV839+=Moved;
                    BypassRemainingV839=FMath::Max(0.f,BypassRemainingV839-Moved);BypassBlockedSecondsV839=0;
                    if(BypassRemainingV839<=.01f)BypassPhaseV839=2;
                    return Moved;
                }
                // A rear obstruction is a real stop. Do not force a backing
                // escape through another hull or a section behind the vehicle.
                BypassBlockedSecondsV839+=Seconds;
                if(BypassBlockedSecondsV839>=1.5f){BypassPhaseV839=0;BypassRemainingV839=0;RouteDecisionAtV37=0;}
                return 0;
            }
        }
    }
    static constexpr float Lanes[4]={-900,-300,300,900};
    const bool NativeGermanHull=bNationalGermanGroundV82&&FRRUGermanStaticContactV82::Eligible(Battle,Team,Type)
        &&WarfareFRRUWorldV82::Ready(Battle)&&HasEnteredField();
    if(NativeGermanHull&&GermanRecoveryPhaseV826>0)
    {
        DesiredLaneV37=GermanRecoveryLaneV826;
        if(Battle->Elapsed<GermanRecoveryReadyAtV826)return 0;
        const auto RecoveryFailed=[&]()
        {
            GermanRecoveryBlockedSecondsV826+=Seconds;
            if(GermanRecoveryBlockedSecondsV826>=1.f)
            {
                // Stay in this bounded recovery phase and keep its lane. A
                // rear hull may clear later; immediately resuming the blocked
                // forward route would make the vehicle rock back and forth.
                GermanRecoveryReadyAtV826=Battle->Elapsed+4.f;
                GermanRecoveryBlockedSecondsV826=0;
            }
        };
        if(GermanRecoveryPhaseV826==1)
        {
            if(GermanRecoveryRemainingCmV826<=.01f||GermanAcceptedRouteV826.Num()<2)
            {GermanRecoveryPhaseV826=2;return 0;}
            // Follow the actually accepted inbound curve and heading. A fixed
            // reverse heading can run a shoe into the bank beside that curve.
            // Rechecking each segment also respects newly arrived live hulls.
            const auto& Target=GermanAcceptedRouteV826[GermanAcceptedRouteV826.Num()-2];
            const float ToTarget=float(FVector::Dist2D(Before,Target.Position));
            const float Step=FMath::Min3(Distance,10.f,GermanRecoveryRemainingCmV826);
            const float Turn=FMath::FindDeltaAngleDegrees(HeadingYawV37,Target.Heading);
            const float Fraction=ToTarget>.001f?FMath::Min(1.f,Step/ToTarget):1.f;
            const float LimitedFraction=FMath::Abs(Turn)>3.f?FMath::Min(Fraction,3.f/FMath::Abs(Turn)):Fraction;
            FVector Candidate=FMath::Lerp(Before,Target.Position,LimitedFraction);Candidate.Z=Before.Z;
            const float NextHeading=HeadingYawV37+Turn*LimitedFraction;
            const float Travel=float(FVector::Dist2D(Before,Candidate));
            if(FMath::Abs(Candidate.Y)<=980.f&&CanOccupyV48(Candidate,NextHeading)
                &&CommitNationalGroundV82(Candidate,NextHeading,Travel,false,true))
            {
                const float Moved=float(FVector::Dist2D(Before,GetActorLocation()));
                GermanRecoveryRemainingCmV826=FMath::Max(0.f,GermanRecoveryRemainingCmV826-Moved);
                GermanRecoveryBlockedSecondsV826=0;
                if(FVector::Dist2D(GetActorLocation(),Target.Position)<=.01
                    &&FMath::Abs(FMath::FindDeltaAngleDegrees(HeadingYawV37,Target.Heading))<=.01f)
                    GermanAcceptedRouteV826.Pop(EAllowShrinking::No);
                if(GermanRecoveryRemainingCmV826<=.01f)GermanRecoveryPhaseV826=2;
                return Moved;
            }
            RecoveryFailed();return 0;
        }
        if(GermanRecoveryPhaseV826==2)
        {
            FVector Wish(Dir,FMath::Clamp((DesiredLaneV37-float(Before.Y))/620.f,-.85f,.85f),0);
            const float Target=float(Wish.Rotation().Yaw),Next=FMath::FixedTurn(HeadingYawV37,Target,48.f*Seconds);
            if(CanOccupyV48(Before,Next)&&CommitNationalGroundV82(Before,Next,0))
            {
                GermanRecoveryBlockedSecondsV826=0;
                if(FMath::Abs(FMath::FindDeltaAngleDegrees(HeadingYawV37,Target))<=.5f)GermanRecoveryPhaseV826=3;
            }
            else RecoveryFailed();
            return 0;
        }
    }
    RouteDecisionAtV37-=Seconds;
    // SideReady can acquire the pass above during this same call.
    const bool HoldingCertifiedPassV84412=bBypassInfantryPassingV84412;
    if(HoldingCertifiedPassV84412)DesiredLaneV37=BypassLaneV839;
    if(RouteDecisionAtV37<=0&&GermanRecoveryPhaseV826==0&&BypassPhaseV839==0&&!HoldingCertifiedPassV84412)
    {
        RouteDecisionAtV37=.45f+(Serial%5)*.037f;
        const auto* Mission=Battle->GetEncounterCampaignMission();
        const bool bRegionalWet=Mission&&(Mission->TerrainStyleV32==EFrontlineMissionTerrainV32::Marsh
            ||Mission->TerrainStyleV32==EFrontlineMissionTerrainV32::Canal);
        const auto FeaturesAt=[](float X,float Y){return AWarfareFrontlineGround::QueryTerrainFeaturesV54(X,Y);};
        const bool bGermanNativeLookAhead=bNationalGermanGroundV82&&Type==EFrontlineVehicle::BeuteMarkIV
            &&WarfareFRRUWorldV82::Ready(Battle);
        TArray<float,TInlineAllocator<10>> CandidateLanes;
        for(float Lane:Lanes)CandidateLanes.Add(Lane);
        if(!bNationalGermanGroundV82&&!LegacyFixedLanesV8448)
        {
            const auto AddLane=[&](float Lane)
            {
                Lane=FMath::Clamp(Lane,-900.f,900.f);
                if(!CandidateLanes.ContainsByPredicate([&](float P){return FMath::Abs(P-Lane)<65.f;}))CandidateLanes.Add(Lane);
            };
            AddLane(float(Before.Y));AddLane(DesiredLaneV37);AddLane(PreferredLaneV37);
            // Fixed recruitment lanes are not walls. Keep a usable current
            // corridor and rank the nearest actual body's two shoulders too.
            // The bounded list avoids an all-bodies quadratic route search.
            const AWarfareFrontlineSoldier* Nearest=nullptr;float Closest=1900.f;
            for(const AWarfareFrontlineSoldier* U:Battle->Units)if(IsValid(U)&&U->IsAlive()&&!U->IsHidden())
            {
                const float Ahead=(U->Feet().X-Before.X)*Dir;
                if(Ahead>-FootprintHalfLengthV37()&&Ahead<Closest&&FMath::Abs(U->Feet().Y-Before.Y)<600.f
                    &&BlocksInfantryAtV8218(U->Feet(),FVector(U->Feet().X,Before.Y,Before.Z),HeadingYawV37,34.f,0.f,U))
                {Closest=Ahead;Nearest=U;}
            }
            if(Nearest){const float Shoulder=FootprintHalfWidthV8311()+114.f;AddLane(Nearest->Feet().Y-Shoulder);AddLane(Nearest->Feet().Y+Shoulder);}
        }
        float BestCost=FLT_MAX,Chosen=DesiredLaneV37;
        for(float Lane:CandidateLanes)
        {
            float Cost=VehicleLaneScoreV55(Before,Lane,PreferredLaneV37,DesiredLaneV37,Dir,bRegionalWet,FeaturesAt);
            // The long Beute track needs an approach before a steep face reaches it.
            // A7V retains its separately verified lane selection. Current contact
            // still gates every translation and turn in CommitNationalGroundV82.
            if(bGermanNativeLookAhead&&!WarfareFRRUWorldV82::CanTraverse(Battle,Before,
                FVector(Before.X+Dir*1000.f,Lane,Before.Z)))Cost+=4200.f;
            // A rejected native translation is route feedback, not permission
            // to bypass contact. Rank another lane at the normal decision time.
            if(bNationalGermanGroundV82&&Battle->Elapsed<FailedGermanLaneUntilHostV82
                &&FMath::IsNearlyEqual(Lane,FailedGermanLaneV82,1.f))Cost+=4200.f;
            if(!bNationalGermanGroundV82&&Battle->Elapsed<BlockedLaneUntilV837
                &&FMath::IsNearlyEqual(Lane,BlockedLaneV837,1.f))Cost+=2700.f;
            for(const AFrontlineTank* Other:Fleet->GetVehicles())if(IsValid(Other)&&Other!=this&&!Other->IsHidden()
                &&(!bNationalGermanGroundV82||Other->IsOperational()))
            {
                const float Ahead=(Other->GetActorLocation().X-Before.X)*Dir;
                if(Ahead< -FootprintHalfLengthV37()-Other->FootprintHalfLengthV37()-90.f||Ahead>3300.f)continue;
                const float Angle=FMath::DegreesToRadians(float(Other->GetActorRotation().Yaw));
                const float AcrossRadius=Other->FootprintHalfLengthV37()*FMath::Abs(FMath::Sin(Angle))
                    +Other->FootprintHalfWidthV8311()*FMath::Abs(FMath::Cos(Angle));
                const float Clearance=bNationalGermanGroundV82?530.f:FootprintHalfWidthV8311()+AcrossRadius+85.f;
                if(FMath::Abs(Other->GetActorLocation().Y-Lane)<Clearance)
                    Cost+=(Other->IsOperational()&&!Other->IsImmobilized()?4200.f:6400.f)*(1.f-FMath::Clamp(Ahead,0.f,3300.f)/4600.f);
                else if(!bNationalGermanGroundV82&&Ahead>=0.f)
                {
                    // Stable tie-break only: consecutive hulls do not all pick
                    // the first equally clear shoulder at the same decision.
                    const float PreferredSide=(Serial+Team)%2?1.f:-1.f;
                    if((Lane-Other->GetActorLocation().Y)*PreferredSide<0.f)Cost+=8.f;
                }
            }
            // A deployed section is a physical obstacle. Rank a clear lane
            // rather than driving into it or waiting forever for it to move.
            for(const AWarfareFrontlineSoldier* U:Battle->Units)if(IsValid(U)&&U->IsAlive()&&!U->IsHidden())
            {
                const float Ahead=(U->Feet().X-Before.X)*Dir;
                if(Ahead>=-FootprintHalfLengthV37()&&Ahead<1500.f&&FMath::Abs(U->Feet().Y-Lane)<290.f
                    &&BlocksInfantryAtV8218(U->Feet(),FVector(U->Feet().X,Lane,Before.Z),Dir>0?0.f:180.f,34.f,0.f,U))
                    Cost+=1500.f*(1.f-FMath::Clamp(Ahead,0.f,1500.f)/2000.f);
            }
            if(Cost<BestCost){BestCost=Cost;Chosen=Lane;}
        }
        DesiredLaneV37=Chosen;
    }
    float Forward=1.f;bool SoftNeighbourNearV8311=false;
    const float HullYaw=FMath::DegreesToRadians(HeadingYawV37);
    const float HullHalfAcross=FootprintHalfLengthV37()*FMath::Abs(FMath::Sin(HullYaw))
        +FootprintHalfWidthV8311()*FMath::Abs(FMath::Cos(HullYaw));
    for(const AFrontlineTank* Other:Fleet->GetVehicles())if(IsValid(Other)&&Other!=this&&!Other->IsHidden()
        &&(!bNationalGermanGroundV82||Other->IsOperational()))
    {
        const float Ahead=(Other->GetActorLocation().X-Before.X)*Dir;
        const float Clearance=FootprintHalfLengthV37()+Other->FootprintHalfLengthV37()+260.f;
        const float OtherYaw=FMath::DegreesToRadians(float(Other->GetActorRotation().Yaw));
        const float OtherHalfAcross=Other->FootprintHalfLengthV37()*FMath::Abs(FMath::Sin(OtherYaw))
            +Other->FootprintHalfWidthV8311()*FMath::Abs(FMath::Cos(OtherYaw));
        SoftNeighbourNearV8311|=!bNationalGermanGroundV82&&Ahead>=-FootprintHalfLengthV37()-Other->FootprintHalfLengthV37()-80.f&&Ahead<2000.f
            &&FMath::Abs(Other->GetActorLocation().Y-Before.Y)<1200.f;
        const float AcrossClearance=bNationalGermanGroundV82?545.f:HullHalfAcross+OtherHalfAcross+12.f;
        if(Ahead>0&&Ahead<Clearance&&FMath::Abs(Other->GetActorLocation().Y-Before.Y)<AcrossClearance)
            Forward=FMath::Min(Forward,FMath::Clamp((Ahead-Clearance+230.f)/230.f,bNationalGermanGroundV82?0.f:.72f,1.f));
    }
    const float Across=FMath::Clamp((DesiredLaneV37-float(Before.Y))/620.f,-.85f,.85f);
    FVector Wish(Dir*Forward,Across,0);
    if(HoldingCertifiedPassV84412)Wish=FRotator(0,HeadingYawV37,0).Vector();
    // A queue at the currently preferred lane can have no desired vector.
    // It must still visit the physical short-arc search and stalled-lane
    // feedback below. Returning here bypassed that recovery indefinitely.
    // This direction alone grants no travel: every candidate keeps the
    // ordinary swept hull and grounded contact checks.
    if(!Wish.Normalize())Wish=FVector(Dir,0,0);
    if(bRollingAcquisitionV8448&&!bNationalGermanGroundV82&&!LegacyContactTurnV8448&&RouteStallSecondsV837<.40f&&!HoldingCertifiedPassV84412)
    {
        // Fixed sponsons can acquire their arc along a swept rolling route.
        // Keep it in the forward hemisphere; a rear contact grants no reverse.
        const float ForwardYaw=Dir>0?0.f:180.f;
        const float Delta=FMath::FindDeltaAngleDegrees(ForwardYaw,RollingAcquisitionYawV8448);
        Wish=FRotator(0,ForwardYaw+FMath::Clamp(Delta,-60.f,60.f),0).Vector();
    }
    const float TargetYaw=float(Wish.Rotation().Yaw);
    const float SteeringRate=bNationalGermanGroundV82?48.f:SteeringRateV8311(Distance/Seconds,false);
    const float FrameStartYawV837=HeadingYawV37;
    const float NextYaw=FMath::FixedTurn(HeadingYawV37,TargetYaw,SteeringRate*Seconds);
    if(!bNationalGermanGroundV82&&CanOccupyV48(Before,NextYaw,Seconds))HeadingYawV37=NextYaw;
    FVector P=Before;
    const float CandidateHeading=bNationalGermanGroundV82?NextYaw:HeadingYawV37;
    const FVector Heading=FRotator(0,CandidateHeading,0).Vector();
    if(FVector::DotProduct(Heading,Wish)>.82f)
    {
        const float Remaining=FMath::Abs(DesiredLaneV37-float(Before.Y));
        const float Step=(Forward<.01f)?FMath::Min(Distance,Remaining):Distance;
        const FVector Candidate=Before+Heading*Step;
        bool ClearPath=FMath::Abs(Candidate.Y)<=980.f;
        const int32 ContactSteps=FMath::Max(1,FMath::CeilToInt(Step/20.f));
        for(int32 I=1;I<=ContactSteps&&ClearPath;++I)
            ClearPath=CanOccupyV48(FMath::Lerp(Before,Candidate,double(I)/ContactSteps),CandidateHeading,Seconds*I/ContactSteps);
        if(ClearPath)P=Candidate;
    }
    // A coarse preferred lane is not a movement command. A hull can have a
    // clear, short forward arc while the complete turn toward that lane is
    // blocked by a neighbour. Keep collision on and try only bounded steering
    // steps through the same swept hull/floor planner; never slide or teleport.
    if(!bNationalGermanGroundV82&&BypassPhaseV839==0&&FVector::DistSquared2D(P,Before)<.0001&&Distance>0.f&&!HoldingCertifiedPassV84412)
    {
        float BestProgress=-1.f;FVector Best=P;float BestYaw=HeadingYawV37;
        const float MaxTurn=SteeringRate*Seconds;
        for(float Offset:{0.f,-MaxTurn,MaxTurn})
        {
            // A rejected lane turn must not discard last frame's usable arc
            // or spend a second turn budget. Search from the actual start pose.
            const float Yaw=FrameStartYawV837+Offset;
            if(!CanOccupyV48(Before,Yaw,Seconds))continue;
            const FVector Facing=FRotator(0,Yaw,0).Vector();
            if(Facing.X*Dir<.10f)continue;
            const FVector Candidate=Before+Facing*Distance;
            bool Clear=FMath::Abs(Candidate.Y)<=980.f;
            const int32 Samples=FMath::Max(1,FMath::CeilToInt(Distance/20.f));
            for(int32 I=1;I<=Samples&&Clear;++I)
                Clear=CanOccupyV48(FMath::Lerp(Before,Candidate,double(I)/Samples),Yaw,Seconds*I/Samples);
            if(!Clear)continue;
            const float Progress=float((Candidate.X-Before.X)*Dir)
                +.12f*(FMath::Abs(DesiredLaneV37-Before.Y)-FMath::Abs(DesiredLaneV37-Candidate.Y));
            if(Progress>BestProgress){BestProgress=Progress;Best=Candidate;BestYaw=Yaw;}
        }
        if(BestProgress>=0.f){P=Best;HeadingYawV37=BestYaw;}
    }
    if(SoftNeighbourNearV8311&&FVector::DistSquared2D(P,Before)>.0001&&!HoldingCertifiedPassV84412)
    {
        // Rank short feasible arcs using visible hulls and wrecks. A soft cost
        // may choose a cleaner corridor but never makes armour a hard blocker.
        const float MaxTurn=SteeringRate*Seconds;float BestCost=MAX_flt;FVector Best=P;float BestYaw=HeadingYawV37;
        const float NominalYaw=HeadingYawV37;
        for(float Yaw:{NominalYaw,FrameStartYawV837-MaxTurn,FrameStartYawV837,FrameStartYawV837+MaxTurn})
        {
            const FVector Facing=FRotator(0,Yaw,0).Vector();if(Facing.X*Dir<.35f)continue;
            const FVector Candidate=Before+Facing*Distance*Forward;
            bool Clear=FMath::Abs(Candidate.Y)<=980.f;const int32 Samples=FMath::Max(1,FMath::CeilToInt(Distance/20.f));
            for(int32 I=1;I<=Samples&&Clear;++I)
                Clear=CanOccupyV48(FMath::Lerp(Before,Candidate,double(I)/Samples),Yaw,Seconds*I/Samples);
            if(!Clear)continue;
            float Cost=FMath::Abs(DesiredLaneV37-Candidate.Y)*.12f+FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw,TargetYaw))*.6f;
            for(const AFrontlineTank* Other:Fleet->GetVehicles())if(IsValid(Other)&&Other!=this&&!Other->IsHidden())
            {
                const float Ahead=(Other->GetActorLocation().X-Before.X)*Dir;
                if(Ahead<-FootprintHalfLengthV37()-Other->FootprintHalfLengthV37()-80.f||Ahead>2000.f)continue;
                Cost+=ArmourOverlapV8311(Candidate,Yaw,Other,24.f)*5.f;
                // The selected lane already ranks the whole approach. A
                // straight extrapolation of this frame's tiny turn falsely
                // predicts a collision for every gradual long-hull detour,
                // and rewards continuing straight through the obstacle.
            }
            if(Cost<BestCost){BestCost=Cost;Best=Candidate;BestYaw=Yaw;}
        }
        P=Best;HeadingYawV37=BestYaw;
    }
    if(!bNationalGermanGroundV82&&FVector::DistSquared2D(P,Before)<.0001)
    {
        // A rejected travel proposal cannot retain its moving turn allowance.
        // Use the existing stationary drivetrain rate (zero for a wheel car),
        // with the same supported hull/body gate as the attempted move.
        const float StationaryYaw=FMath::FixedTurn(FrameStartYawV837,HeadingYawV37,
            SteeringRateV8311(0.f,true)*Seconds);
        HeadingYawV37=CanOccupyV48(Before,StationaryYaw,Seconds)?StationaryYaw:FrameStartYawV837;
    }
    if(bNationalGermanGroundV82)
    {
        const float ProposedTravel=float(FVector::Dist2D(Before,P));
        const bool CommittedMove=CommitNationalGroundV82(P,CandidateHeading,ProposedTravel);
        if(!CommittedMove)
        {
            if(ProposedTravel>.01f){FailedGermanLaneV82=DesiredLaneV37;FailedGermanLaneUntilHostV82=Battle->Elapsed+2.f;}
            FString ContactReason;
            const auto FailedContact=FRRUGermanStaticContactV82::Diagnosis(this);
            if(FailedContact)FailedContact->TryGetStringField(TEXT("reason"),ContactReason);
            // A blocked translation must not prevent physically acquiring a
            // different route heading. No XY travel or track phase is spent.
            bool TurnCommitted=false;
            if(ProposedTravel>.01f&&FMath::Abs(FMath::FindDeltaAngleDegrees(HeadingYawV37,CandidateHeading))>.01f)
                TurnCommitted=CommitNationalGroundV82(Before,CandidateHeading,0);
            // A contact-fit budget or domain refusal also blocks real ground
            // travel. It needs the same bounded route recovery, not a looser fit.
            const bool TerrainBlocked=ContactReason==TEXT("german_actual_shoe_face_penetration")
                ||ContactReason==TEXT("german_shoe_articulation_limit")
                ||ContactReason==TEXT("support_search_budget")||ContactReason==TEXT("plane_residual")
                ||ContactReason==TEXT("german_articulated_support_domain");
            const bool CompleteHullBlocked=ContactReason==TEXT("german_complete_hull_endpoint_overlap")
                ||ContactReason==TEXT("german_complete_hull_swept_block");
            if(NativeGermanHull&&!TurnCommitted&&(TerrainBlocked||CompleteHullBlocked||GermanRecoveryPhaseV826==3))
            {
                GermanTerrainStallSecondsV826+=Seconds;
                if(GermanRecoveryPhaseV826==3)
                {
                    GermanRecoveryBlockedSecondsV826+=Seconds;
                    if(GermanRecoveryBlockedSecondsV826>=1.f)
                    {GermanRecoveryReadyAtV826=Battle->Elapsed+4.f;GermanRecoveryBlockedSecondsV826=0;}
                }
                else if(GermanTerrainStallSecondsV826>=.6f&&Battle->Elapsed>=GermanRecoveryReadyAtV826
                    &&FMath::Abs(DesiredLaneV37-Before.Y)>1.f)
                {
                    // Pick a different existing lane once. This centre-line
                    // rank never grants physical traversal; each recovery step
                    // still passes CommitNationalGroundV82's complete checks.
                    const float Chosen=DesiredLaneV37;
                    if(GermanAcceptedRouteV826.Num()>=2
                        &&WarfareFRRUWorldV82::CanTraverse(Battle,Before,FVector(Before.X+Dir*600.f,Chosen,Before.Z)))
                    {
                        GermanRecoveryLaneV826=Chosen;GermanRecoveryPhaseV826=1;
                        GermanRecoveryRemainingCmV826=FMath::Min(140.f,FootprintHalfLengthV37());GermanRecoveryBlockedSecondsV826=0;
                    }
                    else GermanRecoveryReadyAtV826=Battle->Elapsed+4.f;
                }
            }
            else GermanTerrainStallSecondsV826=0;
        }
        else if(ProposedTravel>.01f)
        {GermanTerrainStallSecondsV826=0;GermanRecoveryBlockedSecondsV826=0;GermanRecoveryPhaseV826=0;}
    }
    else
    {
        FitGroundV35(P,Seconds);
        const float Moved=float(FVector::Dist2D(Before,GetActorLocation()));
        const bool Turning=FMath::Abs(FMath::FindDeltaAngleDegrees(FrameStartYawV837,HeadingYawV37))>.01f;
        // A small blocked-lane yaw oscillation is not route progress. Counting
        // only motionless headings could keep recovery at zero indefinitely.
        const float ForwardProgress=float((GetActorLocation().X-Before.X)*Dir);
        const bool RouteProgress=ForwardProgress>=FMath::Min(2.f,Distance*.10f)&&Moved>.01f;
        // Sideways millimetres and tiny alternating arcs do not resolve a
        // blocked approach. Only useful forward travel clears route feedback.
        RouteStallSecondsV837=RouteProgress?0.f:RouteStallSecondsV837+Seconds;
        if(bBypassInfantryPassingV84412&&RouteStallSecondsV837>=.60f)
        {bBypassInfantryPassingV84412=false;RouteDecisionAtV37=0;}
        if(ForwardProgress<.05f&&RouteStallSecondsV837>=.60f&&!bBypassInfantryPassingV84412
            &&PrepareStaticBodyDetourV85(Distance/Seconds,Seconds))return Moved;
        if(BypassPhaseV839==2&&Moved>.01f){BypassPhaseV839=0;BypassNeighbourV839.Reset();}
        // A held man can have a real floor route with every first lateral step
        // closed by this stopped hull. Moving forward waits for the man, while
        // the man waits for room. Give that actual friend one finite, swept
        // reverse gap; soft armour neighbours never acquire this lease.
        if(BypassPhaseV839==0&&!FrontlineUseLegacyArmourYieldV84411()
            &&!WarfareFRRUCampaignV82::Active(Battle)&&ForwardProgress<.05f&&RouteStallSecondsV837>=.60f)
        {
            for(auto It=BypassInfantryAttemptsV84411.CreateIterator();It;++It)
                if(!It.Key().IsValid()||!It.Key()->IsAlive()||It.Key()->IsHidden())It.RemoveCurrent();
            for(AWarfareFrontlineSoldier* U:Battle->Units)
            {
                if(!IsValid(U)||!U->IsAlive()||U->IsHidden()||U->Team!=Team)continue;
                const auto* Held=Battle->FindSquad(U->SquadId);
                const FVector* Attempt=BypassInfantryAttemptsV84411.Find(U);
                if(!Held||Held->bAdvancing||!Battle->Trenches.IsValidIndex(Held->Trench)
                    ||(Attempt&&FVector::DistSquared2D(U->Feet(),*Attempt)<FMath::Square(56.f)))continue;
                FVector Station;
                if(!BlocksInfantryAtV8218(U->Feet(),Before+FRotator(0,HeadingYawV37,0).Vector()*Distance,
                        HeadingYawV37,34.f,Seconds,U)||!InfantryYieldPointV8218(U,Station))continue;
                const float Back=FMath::Min(Distance,20.f);
                const FVector Rear=Before-FRotator(0,HeadingYawV37,0).Vector()*Back;
                bool ReverseClear=FMath::Abs(Rear.Y)<=980.f;
                const int32 Samples=FMath::Max(1,FMath::CeilToInt(Back/10.f));
                for(int32 I=1;I<=Samples&&ReverseClear;++I)
                    ReverseClear=CanOccupyV48(FMath::Lerp(Before,Rear,double(I)/Samples),HeadingYawV37,Seconds*I/Samples);
                if(!ReverseClear)continue;
                BypassNeighbourV839.Reset();BypassInfantryV84411=U;BypassInfantryStartV84411=U->Feet();
                bBypassInfantryPassingV84412=false;
                BypassInfantryAttemptsV84411.Add(U,U->Feet());
                BypassLaneV839=DesiredLaneV37;BypassPhaseV839=1;
                BypassRemainingV839=FMath::Min(140.f,FootprintHalfLengthV37());BypassBlockedSecondsV839=0;
                BypassBackingLimitV839=BypassBackingTotalV839+BypassRemainingV839;
                break;
            }
        }
        if(RouteStallSecondsV837>=(Turning?1.20f:.60f))
        {BlockedLaneV837=DesiredLaneV37;BlockedLaneUntilV837=Battle->Elapsed+2.f;RouteDecisionAtV37=0.f;RouteStallSecondsV837=.25f;}
    }
    return FVector::Dist2D(Before,GetActorLocation());
}
float AFrontlineTank::AdvanceCombatArcV8311(const FVector& Target,float Distance,float Seconds)
{
    bBackingStepV839=false;
    if(!Battle||!IsWheeledV8218()||bNationalGermanGroundV82||Distance<=0.f||Seconds<=0.f||IsImmobilized())return 0.f;
    // Couple rolling speed to the capped steering rate. Without slowing here,
    // a 28-degree/s limit at road speed expands the assumed 550cm bend to 767cm
    // and carries a rear-contact U-turn into the frontage boundary.
    Distance=FMath::Min(Distance,550.f*FMath::DegreesToRadians(28.f)*Seconds);
    const FVector Before=GetActorLocation();const float StartYaw=HeadingYawV37;
    float Delta=FMath::FindDeltaAngleDegrees(StartYaw,float((Target-Before).Rotation().Yaw));
    if(RoadCombatTurnSignV8311==0.f)
    {
        // Choose the usable shoulder before a rear contact needs a U-turn.
        // The sign stays leased until a mount actually acquires that contact.
        if(FMath::Abs(Delta)>110.f)
        {
            const float IntoField=-float(Before.Y)*FMath::Cos(FMath::DegreesToRadians(StartYaw));
            RoadCombatTurnSignV8311=FMath::Abs(IntoField)>1.f?FMath::Sign(IntoField):FMath::Sign(Delta);
        }
        else RoadCombatTurnSignV8311=FMath::Sign(Delta);
    }
    if(FMath::Abs(Delta)>90.f&&Delta*RoadCombatTurnSignV8311<0.f)Delta+=360.f*RoadCombatTurnSignV8311;
    const float Turn=FMath::Clamp(Delta,-SteeringRateV8311(Distance/Seconds,false)*Seconds,SteeringRateV8311(Distance/Seconds,false)*Seconds);
    // Integrate a short centre-line arc, then validate its same intermediate
    // grounded poses against infantry. Chassis yaw is committed only with roll.
    const float VisibleApron=FMath::Min(FrontlinePacingV24::CombatHalfLength()+550.f,FrontlineBattlefield::GroundHalfLength-600.f);
    float Best=MAX_flt,ChosenTurn=Turn;FVector Chosen=Before;
    for(float ArcTurn:{Turn,Turn*.65f,0.f})
    {
        const float HalfTurn=FMath::DegreesToRadians(ArcTurn)*.5f;
        const float Chord=FMath::Abs(HalfTurn)>.00001f?Distance*FMath::Sin(HalfTurn)/HalfTurn:Distance;
        const FVector Candidate=Before+FRotator(0,StartYaw+ArcTurn*.5f,0).Vector()*Chord;
        bool Clear=FMath::Abs(Candidate.Y)<=980.f&&FMath::Abs(Candidate.X)<VisibleApron;
        const int32 Samples=FMath::Max(1,FMath::CeilToInt(Distance/10.f));
        for(int32 I=1;I<=Samples&&Clear;++I)
            Clear=CanOccupyV48(FMath::Lerp(Before,Candidate,double(I)/Samples),StartYaw+ArcTurn*I/Samples,Seconds*I/Samples);
        if(!Clear)continue;
        float Cost=FMath::Abs(Delta-ArcTurn);
        if(auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld()))for(const AFrontlineTank* Other:Fleet->GetVehicles())
            if(IsValid(Other)&&Other!=this&&!Other->IsHidden())Cost+=ArmourOverlapV8311(Candidate,StartYaw+ArcTurn,Other,24.f)*5.f;
        if(Cost<Best){Best=Cost;Chosen=Candidate;ChosenTurn=ArcTurn;}
    }
    if(Best==MAX_flt)return 0.f;
    bBypassInfantryPassingV84412=false;
    HeadingYawV37=StartYaw+ChosenTurn;FitGroundV35(Chosen,Seconds);return FVector::Dist2D(Before,GetActorLocation());
}

void AFrontlineTank::Simulate(float DT)
{
    if(WarfareFRRUArmourV82::National(this)){WarfareFRRUArmourV82::Simulate(this,DT);return;}
    if(!Battle||DT<=0)return;
    const float RunningGearStartYawV8311=HeadingYawV37;
    const bool Outro=Battle->BattleOutroWinner!=INDEX_NONE;
    if(Outro||!IsOperational()||IsImmobilized()||IsHeldForFieldRepairV63())bBypassInfantryPassingV84412=false;
    if(Outro||!IsOperational()||IsImmobilized()||IsHeldForFieldRepairV63())
    {GermanRecoveryPhaseV826=0;GermanTerrainStallSecondsV826=GermanRecoveryRemainingCmV826=0;GermanRecoveryBlockedSecondsV826=0;GermanRecoveryReadyAtV826=-1;GermanAcceptedRouteV826.Reset();bArmourClearanceV851=false;ArmourClearanceTargetV851.Reset();ArmourClearanceBackingV851=0.f;ArmourClearanceBudgetTargetV851.Reset();bArmourClearanceReverseV851=false;}
    if(Outro)
    {
        // Shots committed before the final order must not kill surrendering
        // soldiers a fraction of a second later. Retire their owned visuals.
        for(auto& S:Shells)if(S.Visual.IsValid())S.Visual->DestroyComponent();Shells.Empty();
    }
    else UpdateShells(DT); // During combat, shots survive the loss of their firing tank.
    UpdateEffects(DT);
    // A burnt hull belongs to this encounter until its normal cleanup. Its
    // fire and audio may die away; neither timer removes the physical wreck.
    if(!IsOperational()){WreckAge+=DT;if(auto* A=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->UpdateTankAudio(Serial+Team*1000,GetActorLocation(),0,false,WreckAge<70,true);return;}
    const auto& D=UFrontlineVehicleSubsystem::Definition(Type);const int32 Dir=Battle->GetTeamDirection(Team);
    if(Outro)
    {
        if(auto* A=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->UpdateTankAudio(Serial+Team*1000,GetActorLocation(),0,!IsImmobilized(),HullIntegrity<D.Hull*.45f,true);
        return;
    }
    bEntered|=HasEnteredField();
    NextVoiceCheck-=DT;
    if(bEntered&&NextVoiceCheck<=0)
    {
        NextVoiceCheck=2.f;
        if(!bArmorReported)bArmorReported=CrewWarning(this,EFrontlineVoiceEvent::FriendlyTankAdvance);
        if(!bEnemyArmorReported)bEnemyArmorReported=CrewWarning(this,EFrontlineVoiceEvent::ArmorSighted,true);
    }
    const float VisibleRange=D.RangeCm*Battle->EncounterVisibilityScale();
    bAcquisitionFeasibleV8448=bMobilityFiringReadyV8448=bMobilityTraversingV8448=false;
    bRollingAcquisitionV8448=false;
    bArmourClearanceStepV851=false;
    ArmourClearanceLeaseOwnerV851=INDEX_NONE;
    float TankDistance=VisibleRange;FVector ArmourAim=FVector::ZeroVector;
    AFrontlineTank* EnemyTank=SelectArmourContactV84414(TankDistance,ArmourAim);
    const AFrontlineTank* BudgetTargetV851=ArmourClearanceBudgetTargetV851.Get();
    // Losing a temporary aiming/visibility predicate does not grant another
    // vehicle-length of backing against the same observed enemy.
    if((EnemyTank&&EnemyTank!=BudgetTargetV851)||!IsValid(BudgetTargetV851)||!BudgetTargetV851->IsOperational())
    {ArmourClearanceBudgetTargetV851=EnemyTank;ArmourClearanceBackingV851=0.f;bArmourClearanceReverseV851=false;}
    if(!EnemyTank||EnemyTank!=ArmourClearanceTargetV851.Get())
    {bArmourClearanceV851=false;ArmourClearanceTargetV851.Reset();bArmourClearanceReverseV851=false;}
    ArmourContactAgeV48+=DT;
    if(EnemyTank){if(ArmourContactV48.Get()!=EnemyTank)bArmourContactUsableV837=false;ArmourContactV48=EnemyTank;LastArmourPointV48=ArmourAim;ArmourContactAgeV48=0;}
    const AFrontlineTank* Remembered=ArmourContactV48.Get();
    const bool RecentArmour=bEntered&&IsValid(Remembered)&&Remembered->IsOperational()&&Remembered->Team!=Team
        &&ArmourContactAgeV48<=1.5f&&ObservedVehicleDistanceV837(this,LastArmourPointV48,true)<VisibleRange;
    const float MachinegunRangeV836=MountedInfantryRangeV836(Battle,Type,false);
    const float InfantryRangeV836=Cannons.IsEmpty()?MachinegunRangeV836:VisibleRange;
    AWarfareFrontlineSoldier* Infantry=nullptr;float InfantryDistance=InfantryRangeV836,InfantryScoreV8448=InfantryRangeV836;bool SelectedInfantryMountedV8311=false;
    // All infantry candidates share this immutable query. Rebuilding the full
    // ignore list for every candidate in every gun arc made crowded scans quadratic.
    const auto InfantrySight=SightParams(GetWorld(),this,nullptr);
    if(bEntered)for(AWarfareFrontlineSoldier* U:Battle->Units)if(U&&U->IsActiveCombatant()&&U->Team!=Team)
    {
        const float Distance=bRailConvoyV32?FMath::Min(ObservedVehicleDistanceV837(this,U->Feet(),true),ObservedVehicleDistanceV837(this,U->Feet(),false)):
            float(FVector::Dist2D(AimPoint(),U->Feet()));if(Distance>=InfantryRangeV836||(SelectedInfantryMountedV8311&&Distance>=InfantryDistance&&U!=InfantryContactV8448.Get())
                ||(bNationalGermanGroundV82&&Distance>=InfantryDistance))continue;
        bool MountedReady=false;FVector MountedPoint;
        if(!bNationalGermanGroundV82)
        {
            for(int32 I=0;I<CannonMounts.Num()&&!MountedReady;++I)MountedReady=InfantryFiringSolutionV828(U,I,true,MountedPoint,&InfantrySight);
            for(int32 I=0;I<GunMounts.Num()&&!MountedReady;++I)MountedReady=InfantryFiringSolutionV828(U,I,false,MountedPoint,&InfantrySight);
        }
        bool Seen=MountedReady;
        const FVector Points[]={FrontlineMachinegunsV30::SightTarget(U),U->Soldier?U->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5):U->Feet()+FVector(0,0,145)};
        for(const FVector& Point:Points)if(!Seen&&!bRailConvoyV32)
        {
            Seen=Battle->DirectFireMultiplier(AimPoint(),Point)>=.9f&&ClearSight(GetWorld(),this,U,AimPoint(),Point,&InfantrySight);
        }
        if(bNationalGermanGroundV82&&!Seen)
        {
            for(int32 I=0;I<CannonMounts.Num()&&!Seen;++I)Seen=InfantryFiringSolutionV828(U,I,true,MountedPoint,&InfantrySight);
            for(int32 I=0;I<GunMounts.Num()&&!Seen;++I)Seen=InfantryFiringSolutionV828(U,I,false,MountedPoint,&InfantrySight);
        }
        const float Score=Distance-(!bNationalGermanGroundV82&&!LegacyContactTurnV8448&&U==InfantryContactV8448.Get()?140.f:0.f);
        if(Seen&&((MountedReady&&!SelectedInfantryMountedV8311)||(MountedReady==SelectedInfantryMountedV8311&&Score<InfantryScoreV8448)))
        {InfantryDistance=Distance;InfantryScoreV8448=Score;Infantry=U;SelectedInfantryMountedV8311=MountedReady;}
    }
    InfantryContactV8448=Infantry;
    bool HaltForArmour=false,CurrentArmourAimV8448=false;
    if(bEntered&&EnemyTank&&!Cannons.IsEmpty()&&TankDistance<FMath::Min(6500.f,VisibleRange))
    {
        const float BeforeHeading=HeadingYawV37;
        FaceArmourV48(ArmourAim,DT,true,EnemyTank);
        bool FiringPosition=false;
        for(int32 I=0;I<CannonMounts.Num()&&!FiringPosition;++I)
        {
            FVector Point;
            if(!bNationalGermanGroundV82)FiringPosition=ArmourFiringSolutionV8219(EnemyTank,I,Point);
            else
            {
                const FVector Muzzle=WeaponMuzzle(I,true);
                const FRotator Bearing=(GetActorTransform().InverseTransformPosition(ArmourAim)-CannonMounts[I]).Rotation();
                FiringPosition=FMath::Abs(FMath::FindDeltaAngleDegrees(CannonBaseYaw[I],float(Bearing.Yaw)))<=55.f
                    &&Bearing.Pitch>=-12.f&&Bearing.Pitch<=18.f&&Battle->DirectFireMultiplier(Muzzle,ArmourAim)>=.9f
                    &&ClearSight(GetWorld(),this,EnemyTank,Muzzle,ArmourAim);
            }
        }
        // Halt during a real traverse or with a usable mounted sight. A hull-eye
        // observation above an occluded muzzle must not freeze both opponents.
        bArmourContactUsableV837=FiringPosition;
        bMobilityFiringReadyV8448=FiringPosition;
        if(FiringPosition){ArmourClearanceBackingV851=0.f;bArmourClearanceReverseV851=false;}
        if(!bNationalGermanGroundV82&&!LegacyContactTurnV8448)for(int32 I=0;I<CannonMounts.Num()&&!CurrentArmourAimV8448;++I)
        {FVector Point;CurrentArmourAimV8448=ArmourFiringSolutionV8219(EnemyTank,I,Point,true);}
        HaltForArmour=FiringPosition||CurrentArmourAimV8448
            ||FMath::Abs(FMath::FindDeltaAngleDegrees(BeforeHeading,HeadingYawV37))>.01f
            ||(bNationalGermanGroundV82&&TankDistance<700.f);
    }
    else if(EnemyTank)bArmourContactUsableV837=false;
    // A briefly obscured, already observed enemy is a reason to retain the
    // firing position, not permission to fire through smoke or track it unseen.
    if(!EnemyTank&&RecentArmour&&!Cannons.IsEmpty()&&!bRailConvoyV32
        &&(bNationalGermanGroundV82||bArmourContactUsableV837))HaltForArmour=true;
    // Any observed close infantry contact can require a hull turn, not just
    // enemies in the final victory sector. Mounts keep their real traverse.
    const bool CloseInfantry=Infantry&&InfantryDistance<(WarfareFRRUCampaignV82::Active(Battle)?3200.f:InfantryRangeV836);
    const float HeadingBeforeInfantry=HeadingYawV37;
    // A committed, physically feasible cannon acquisition is the higher
    // priority chassis task. Nearby infantry can still be engaged by each
    // independent mount but must not overwrite its rolling acquisition yaw.
    const bool AcquiringArmourV84414=!bNationalGermanGroundV82&&!LegacyContactChoiceV84414
        &&EnemyTank&&!Cannons.IsEmpty()&&bAcquisitionFeasibleV8448&&(bRollingAcquisitionV8448||bArmourClearanceStepV851);
    // An observed heavy hull is not a usable main-gun target for a machine-gun
    // car. Keep its real infantry mounts facing the men it can actually fight.
    const bool InfantryBearingReady=!CloseInfantry||AcquiringArmourV84414||(WarfareFRRUCampaignV82::Active(Battle)?EnemyTank!=nullptr:HaltForArmour)
        ||SelectedInfantryMountedV8311||FaceArmourV48(Infantry->Feet()+FVector(0,0,105),DT,false,Infantry);
    bool InfantryMountReady=false,CurrentInfantryAimV8448=false;
    if(CloseInfantry&&InfantryBearingReady&&!bNationalGermanGroundV82)
    {
        FVector Point;
        for(int32 I=0;I<CannonMounts.Num()&&!InfantryMountReady;++I)InfantryMountReady=InfantryFiringSolutionV828(Infantry,I,true,Point,&InfantrySight);
        for(int32 I=0;I<GunMounts.Num()&&!InfantryMountReady;++I)InfantryMountReady=InfantryFiringSolutionV828(Infantry,I,false,Point,&InfantrySight);
    }
    if(CloseInfantry&&!bNationalGermanGroundV82&&!LegacyContactTurnV8448)
    {
        FVector Point;
        for(int32 I=0;I<CannonMounts.Num()&&!CurrentInfantryAimV8448;++I)CurrentInfantryAimV8448=InfantryFiringSolutionV828(Infantry,I,true,Point,&InfantrySight,true);
        for(int32 I=0;I<GunMounts.Num()&&!CurrentInfantryAimV8448;++I)CurrentInfantryAimV8448=InfantryFiringSolutionV828(Infantry,I,false,Point,&InfantrySight,true);
    }
    const bool TurningToInfantry=FMath::Abs(FMath::FindDeltaAngleDegrees(HeadingBeforeInfantry,HeadingYawV37))>.01f;
    bMobilityFiringReadyV8448|=InfantryMountReady;
    bMobilityTraversingV8448=!bMobilityFiringReadyV8448&&(CurrentInfantryAimV8448||CurrentArmourAimV8448||TurningToInfantry
        ||(HaltForArmour&&EnemyTank&&!bArmourContactUsableV837));
    // Seeing over a lip at hull-eye height does not mean a mounted gun can
    // shoot. Retain a real turn or firing position; otherwise keep approaching
    // until a muzzle clears, instead of both sides waiting forever.
    const bool HaltForGun=bEntered&&(HaltForArmour||(CloseInfantry&&!AcquiringArmourV84414
        &&(bNationalGermanGroundV82||InfantryMountReady||CurrentInfantryAimV8448||(LegacyContactTurnV8448&&TurningToInfantry))));
    const bool RoadCombatArcV8311=!bNationalGermanGroundV82&&IsWheeledV8218()&&CloseInfantry&&!HaltForArmour&&!InfantryMountReady
        &&!CurrentInfantryAimV8448
        &&(LegacyContactTurnV8448||bAcquisitionFeasibleV8448)
        &&GunIntegrity.ContainsByPredicate([](float Integrity){return Integrity>0.f;});
    if(RoadCombatArcV8311)
    {if(RoadCombatTargetV8311.Get()!=Infantry){RoadCombatTurnSignV8311=0.f;RoadCombatTargetV8311=Infantry;}}
    else{RoadCombatTurnSignV8311=0.f;RoadCombatTargetV8311.Reset();}
    bool HaltForRailDoors=false;
    if(bRailConvoyV32&&!bRailUnloadedV75&&bEntered)
    {
        const float Progress=GetActorLocation().X*Dir;
        const float Home=Battle->Trenches[Battle->GetHomeTrenchIndex(Team)].X*Dir;
        // Troop cars must actually reach the army's forward railhead before
        // their doors open. Contact holds the train while infantry dismounts.
        const FVector FrontDoor=GetActorTransform().TransformPosition(FVector(1320,0,0));
        const FVector RearDoor=GetActorTransform().TransformPosition(FVector(-1400,0,0));
        const bool DoorsInside=FMath::Abs(FrontDoor.X)<FrontlinePacingV24::CombatHalfLength()-100.f
            &&FMath::Abs(RearDoor.X)<FrontlinePacingV24::CombatHalfLength()-100.f;
        HaltForRailDoors=DoorsInside&&(Progress>=Home+700.f||HaltForGun||IsImmobilized()||RailSectionMaskV837!=0);
        UpdateRailDisembarkationV837(DT,HaltForRailDoors);
    }
    else if(bRailConvoyV32)UpdateRailDisembarkationV837(DT,false);
    // Hold inside the authored rear approach if resistance remains. Re-evaluate
    // victory while stationary too, after allies clear an obscured defender.
    const float HullRearOffset=FMath::Max(FootprintHalfLengthV37(),float(GetActorLocation().X*Dir)-RearProgressV49());
    const float EndTravel=FMath::Max(0.f,FrontlineGameplayV49::BreakthroughLine()+HullRearOffset+240.f-GetActorLocation().X*Dir);
    const float TerrainPace=bRailConvoyV32||IsImmobilized()||HaltForGun?1.f:
        (bNationalGermanGroundV82?WarfareFRRUWorldV82::TravelScale(Battle,GetActorLocation()):FrontlineTerrainTacticsV54::VehicleTravelScale(GetActorLocation()));
    float WheelPaceV8218=1.f;
    if(IsWheeledV8218())
    {
        const auto Features=AWarfareFrontlineGround::QueryTerrainFeaturesV54(GetActorLocation().X,GetActorLocation().Y);
        const float Wet=FMath::Max3(Features.MarshChannel,Features.CanalCut,FMath::Max(Features.TankChurn,Features.RailWash));
        WheelPaceV8218=FMath::Lerp(.35f,1.f,Battle->EncounterMudMoveScale())*FMath::Clamp(1.f-.55f*Wet,.40f,1.f);
    }
    const float ResearchPaceV8218=bNationalGermanGroundV82||bRailConvoyV32?1.f:FrontlineArmourV63::TravelSpeedScaleV8218(Battle,Team);
    const float TravelBudgetV8311=D.SpeedCm*TerrainPace*WheelPaceV8218*ResearchPaceV8218*DT;
    const float RequestedMove=!IsImmobilized()&&!HaltForGun&&!HaltForRailDoors&&!IsHeldForFieldRepairV63()?(RoadCombatArcV8311?TravelBudgetV8311:FMath::Min(TravelBudgetV8311,EndTravel)):0;
    const float Move=bArmourClearanceStepV851?AdvanceArmourClearanceV851(RequestedMove,DT):RoadCombatArcV8311?AdvanceCombatArcV8311(Infantry->Feet()+FVector(0,0,105),RequestedMove,DT):AdvanceV37(RequestedMove,DT);
    MobilityStepV839=DT;MobilityHostTimeV839=Battle->Elapsed;MobilityRequestedV839=RequestedMove;MobilityMovedV839=Move;
    MobilityGateV839=Move>.01f?(bBackingStepV839?TEXT("physical_backing"):TEXT("moving")):IsImmobilized()?TEXT("running_gear_disabled"):
        HaltForGun?(bMobilityFiringReadyV8448?TEXT("observed_mounted_contact"):bMobilityTraversingV8448?TEXT("feasible_mounted_traverse"):TEXT("remembered_mounted_contact")):HaltForRailDoors?TEXT("passengers_disembarking"):
        IsHeldForFieldRepairV63()?TEXT("field_repair"):EndTravel<=0.f?TEXT("defended_enemy_end"):
        bArmourClearanceStepV851?TEXT("acquisition_turn_clearance"):
        RequestedMove>0.f?TEXT("physical_route_blocked"):TEXT("no_requested_travel");
    const float TurnRadiansV8311=FMath::DegreesToRadians(FMath::FindDeltaAngleDegrees(RunningGearStartYawV8311,HeadingYawV37));
    if(Move>0||FMath::Abs(TurnRadiansV8311)>.00001f)
    {
        const FVector P=GetActorLocation();
        DistanceTravelled+=Move;TrackUpdate+=DT;TrackDistanceV37+=bBackingStepV839?-Move:Move;
        TrackTurnV8311+=TurnRadiansV8311;
        if(!bRailConvoyV32&&!bNationalGermanGroundV82&&TrackUpdate>.033f){UpdateTracks(TrackDistanceV37,TrackTurnV8311);TrackUpdate=0;TrackDistanceV37=0;TrackTurnV8311=0;}
        if(bNationalGermanGroundV82){TrackUpdate=0;TrackDistanceV37=0;}
        if(bRailConvoyV32)
        {
            for(int32 I=0;I<RailWheelsV33.Num();++I)if(RailWheelsV33[I])
            {const float Radius=54.f;RailWheelsV33[I]->SetRelativeRotation(FRotator(-FMath::RadiansToDegrees(DistanceTravelled/Radius),0,0));}
            const float Angle=DistanceTravelled/54.f;
            for(int32 I=0;I<RailRodsV33.Num();++I)if(RailRodsV33[I])RailRodsV33[I]->SetRelativeLocation(FVector(260.f+22.f*FMath::Cos(Angle),(I?1:-1)*102.f,69.f+22.f*FMath::Sin(Angle)));
            const float BogieRoll=-FMath::RadiansToDegrees(DistanceTravelled/48.f);
            for(UStaticMeshComponent* Wheel:RailBogieWheelsV837)if(Wheel)Wheel->SetRelativeRotation(FRotator(BogieRoll,0,0));
        }
    }
    if(!WarfareFRRUCampaignV82::Active(Battle)&&HasClearedEnemyEndV49())
    {Battle->FinishBattle(Team,TEXT("装甲突破了最后一道防线。"));return;}
    if(auto* A=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->UpdateTankAudio(Serial+Team*1000,GetActorLocation(),Move>0||FMath::Abs(TurnRadiansV8311)>.00001f?1.f:0.f,!IsImmobilized(),HullIntegrity<D.Hull*.45f,true);
    if(!bEntered)return;
    TArray<TSharedPtr<FJsonValue>> GermanCannonDecisions;
    if(bNationalGermanGroundV82)
    {
        GermanCannonReadbackV82=MakeShared<FJsonObject>();GermanCannonReadbackV82->SetNumberField(TEXT("hostElapsed"),Battle->Elapsed);
        GermanCannonReadbackV82->SetNumberField(TEXT("vehicleSerial"),Serial);GermanCannonReadbackV82->SetBoolField(TEXT("entered"),bEntered);
    }
    for(int32 I=0;I<Cannons.Num();++I)
    {
        CannonClock[I]-=DT;if(!RailCannonOperationalV837(I))continue;
        AFrontlineTank* GunArmour=EnemyTank;AWarfareFrontlineSoldier* GunInfantry=Infantry;FVector GunArmourPoint=ArmourAim;
        FVector GunInfantryPoint=Infantry?Infantry->Feet()+FVector(0,0,85):FVector::ZeroVector;
        TSharedPtr<FJsonObject> CannonDecision;
        if(bNationalGermanGroundV82)
        {
            CannonDecision=MakeShared<FJsonObject>();GermanCannonDecisions.Add(MakeShared<FJsonValueObject>(CannonDecision));
            CannonDecision->SetNumberField(TEXT("mount"),I);CannonDecision->SetNumberField(TEXT("cannonClock"),CannonClock[I]);
            CannonDecision->SetNumberField(TEXT("rangeLimitCm"),VisibleRange);CannonDecision->SetBoolField(TEXT("selected"),false);
            CannonDecision->SetBoolField(TEXT("fired"),false);CannonDecision->SetStringField(TEXT("targetType"),TEXT("none"));
            CannonDecision->SetStringField(TEXT("failedPredicate"),TEXT("no_eligible_mounted_target"));
            CannonDecision->SetNumberField(TEXT("globalArmourSerial"),EnemyTank?EnemyTank->Serial:INDEX_NONE);
            GunArmour=nullptr;GunInfantry=nullptr;float Best=VisibleRange;double Observed=DBL_MAX;
            const FVector SearchMuzzle=WeaponMuzzle(I,true);
            const auto Consider=[&](AFrontlineTank* Armour,AWarfareFrontlineSoldier* Person,const FVector& Point)
            {
                if(Point.ContainsNaN())return;const AActor* Actor=Armour?static_cast<AActor*>(Armour):static_cast<AActor*>(Person);
                const double Distance=FVector::Dist2D(GetActorLocation(),Point),MuzzleDistance=FVector::Dist2D(SearchMuzzle,Point);
                const FRotator Bearing=(GetActorTransform().InverseTransformPosition(Point)-CannonMounts[I]).Rotation();
                const double YawDelta=FMath::FindDeltaAngleDegrees(CannonBaseYaw[I],float(Bearing.Yaw));
                const bool Range=Distance<VisibleRange&&MuzzleDistance<VisibleRange,Arc=FMath::Abs(YawDelta)<=55.,Pitch=Bearing.Pitch>=-12&&Bearing.Pitch<=18;
                const bool SmokeChecked=Range&&Arc&&Pitch;const float Smoke=SmokeChecked?Battle->DirectFireMultiplier(SearchMuzzle,Point):0.f;
                const bool LOSChecked=SmokeChecked&&Smoke>=.9f,LOS=LOSChecked&&ClearSight(GetWorld(),this,Actor,SearchMuzzle,Point,Person?&InfantrySight:nullptr);
                const bool Eligible=Range&&Arc&&Pitch&&Smoke>=.9f&&LOS,Choose=Eligible&&Distance<Best;
                if(Choose||(!GunArmour&&!GunInfantry&&Distance<Observed))
                {
                    Observed=Distance;CannonDecision->SetStringField(TEXT("targetType"),Armour?TEXT("armour"):TEXT("infantry"));
                    CannonDecision->SetNumberField(TEXT("targetSerial"),Armour?Armour->Serial:INDEX_NONE);
                    CannonDecision->SetNumberField(TEXT("targetSquad"),Person?Person->SquadId:INDEX_NONE);CannonDecision->SetNumberField(TEXT("targetMember"),Person?Person->MemberIndex:INDEX_NONE);
                    CannonDecision->SetStringField(TEXT("targetPoint"),Point.ToString());CannonDecision->SetStringField(TEXT("muzzle"),SearchMuzzle.ToString());
                    CannonDecision->SetNumberField(TEXT("rootRangeCm"),Distance);CannonDecision->SetNumberField(TEXT("muzzleRangeCm"),MuzzleDistance);
                    CannonDecision->SetNumberField(TEXT("yawDelta"),YawDelta);CannonDecision->SetNumberField(TEXT("pitch"),Bearing.Pitch);
                    CannonDecision->SetBoolField(TEXT("smokeChecked"),SmokeChecked);CannonDecision->SetNumberField(TEXT("smokeMultiplier"),Smoke);
                    CannonDecision->SetBoolField(TEXT("muzzleLOSChecked"),LOSChecked);CannonDecision->SetBoolField(TEXT("muzzleLOSClear"),LOS);
                    CannonDecision->SetBoolField(TEXT("selected"),Choose);
                    CannonDecision->SetStringField(TEXT("failedPredicate"),!Range?TEXT("range"):!Arc?TEXT("yaw_arc"):!Pitch?TEXT("pitch_arc"):Smoke<.9f?TEXT("smoke"):!LOS?TEXT("muzzle_los"):TEXT("awaiting_aim_and_clock"));
                }
                if(Choose){Best=float(Distance);GunArmour=Armour;GunInfantry=Person;GunArmourPoint=Point;}
            };
            if(auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld()))for(AFrontlineTank* Other:Fleet->GetVehicles())
                if(Other&&Other!=this&&Other->Team!=Team&&Other->IsOperational()&&Other->HasEnteredField())Consider(Other,nullptr,Other->AimPoint());
            // An out-of-arc armour contact cannot suppress visible infantry in
            // this actual mount's sector. Both searches use the firing point.
            if(!GunArmour)for(AWarfareFrontlineSoldier* Person:Battle->Units)
                if(Person&&Person->Team!=Team&&Person->IsActiveCombatant())Consider(nullptr,Person,Person->Feet()+FVector(0,0,85));
        }
        else
        {
            GunArmour=nullptr;GunInfantry=nullptr;float Best=FMath::Min(6500.f,VisibleRange);
            const FVector GunOrigin=WeaponMuzzle(I,true);
            if(auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld()))for(AFrontlineTank* V:Fleet->GetVehicles())
                if(V&&V!=this&&V->Team!=Team&&V->IsOperational()&&V->HasEnteredField())
                {
                    FVector P;float Distance=MAX_flt;
                    if(!ArmourFiringSolutionV8219(V,I,P,true,&Distance))continue;
                    if(Distance<Best)
                    {Best=Distance;GunArmour=V;GunArmourPoint=P;}
                }
            if(!GunArmour)for(AWarfareFrontlineSoldier* U:Battle->Units)if(U&&U->Team!=Team&&U->IsActiveCombatant())
            {
                FVector P;const float Distance=FVector::Dist2D(GunOrigin,U->Feet());
                if(Distance<Best&&InfantryFiringSolutionV828(U,I,true,P,&InfantrySight,true))
                {Best=Distance;GunInfantry=U;GunInfantryPoint=P;}
            }
        }
        if(!GunArmour&&!GunInfantry)continue;
        const FVector Target=GunArmour?GunArmourPoint:bNationalGermanGroundV82?GunInfantry->Feet()+FVector(0,0,85):GunInfantryPoint;
        const FVector Local=GetActorTransform().InverseTransformPosition(Target)-CannonMounts[I];const FRotator Desired=Local.Rotation();
        const float Delta=FMath::FindDeltaAngleDegrees(CannonBaseYaw[I],float(Desired.Yaw));
        const float Traverse=CannonTraverseV837(this);
        Cannons[I]->SetRelativeRotation(FMath::RInterpTo(Cannons[I]->GetRelativeRotation(),FRotator(FMath::Clamp(Desired.Pitch,-12.,18.),CannonBaseYaw[I]+FMath::Clamp(Delta,-Traverse,Traverse),0),DT,2.4f));
        const FVector Muzzle=WeaponMuzzle(I,true);
        const AActor* TargetActor=GunArmour?static_cast<AActor*>(GunArmour):static_cast<AActor*>(GunInfantry);
        bool GermanCanFire=false;
        if(bNationalGermanGroundV82)
        {
            const double RootRange=FVector::Dist2D(GetActorLocation(),Target),MuzzleRange=FVector::Dist2D(Muzzle,Target);
            const double YawError=FMath::Abs(FMath::FindDeltaAngleDegrees(Cannons[I]->GetRelativeRotation().Yaw,Desired.Yaw));
            const double PitchError=FMath::Abs(FMath::FindDeltaAngleDegrees(Cannons[I]->GetRelativeRotation().Pitch,Desired.Pitch));
            const float FireVisibility=Battle->DirectFireMultiplier(Muzzle,Target);const bool LOS=ClearSight(GetWorld(),this,TargetActor,Muzzle,Target,GunArmour?nullptr:&InfantrySight);
            const TCHAR* Failed=RootRange>=VisibleRange||MuzzleRange>=VisibleRange?TEXT("range"):
                FMath::Abs(Delta)>55?TEXT("yaw_arc"):Desired.Pitch< -12||Desired.Pitch>18?TEXT("pitch_arc"):
                CannonClock[I]>0?TEXT("cannon_clock"):YawError>=5?TEXT("yaw_alignment"):PitchError>=3?TEXT("pitch_alignment"):
                FireVisibility<.9f?TEXT("smoke"):!LOS?TEXT("muzzle_los"):TEXT("none");
            GermanCanFire=FCString::Strcmp(Failed,TEXT("none"))==0;
            CannonDecision->SetStringField(TEXT("muzzle"),Muzzle.ToString());CannonDecision->SetNumberField(TEXT("rootRangeCm"),RootRange);CannonDecision->SetNumberField(TEXT("muzzleRangeCm"),MuzzleRange);
            CannonDecision->SetNumberField(TEXT("yawDelta"),Delta);CannonDecision->SetNumberField(TEXT("pitch"),Desired.Pitch);
            CannonDecision->SetNumberField(TEXT("yawAimError"),YawError);CannonDecision->SetNumberField(TEXT("pitchAimError"),PitchError);
            CannonDecision->SetBoolField(TEXT("smokeChecked"),true);CannonDecision->SetNumberField(TEXT("smokeMultiplier"),FireVisibility);
            CannonDecision->SetBoolField(TEXT("muzzleLOSChecked"),true);CannonDecision->SetBoolField(TEXT("muzzleLOSClear"),LOS);
            CannonDecision->SetStringField(TEXT("failedPredicate"),Failed);
        }
        if((bNationalGermanGroundV82&&GermanCanFire)||(!bNationalGermanGroundV82&&FMath::Abs(Delta)<=Traverse&&Desired.Pitch>=-12&&Desired.Pitch<=18&&CannonClock[I]<=0
            &&FMath::Abs(FMath::FindDeltaAngleDegrees(Cannons[I]->GetRelativeRotation().Yaw,Desired.Yaw))<5
            &&FMath::Abs(FMath::FindDeltaAngleDegrees(Cannons[I]->GetRelativeRotation().Pitch,Desired.Pitch))<3
            &&(!GunArmour||FVector::Dist2D(Muzzle,Target)<FMath::Min(6500.f,VisibleRange))
            &&Battle->DirectFireMultiplier(Muzzle,Target)>=.9f&&ClearSight(GetWorld(),this,TargetActor,Muzzle,Target,GunArmour?nullptr:&InfantrySight)))
        {
            const int32 PriorShots=CannonShots;FireCannon(I,Target,GunArmour);
            if(CannonDecision)CannonDecision->SetBoolField(TEXT("fired"),CannonShots>PriorShots);
            CannonClock[I]=(Random.FRandRange(7.2f,9.6f)+(Move>0?2:0))*(bNationalGermanGroundV82?1.f:FrontlineArmourV63::CannonCycleScale(Battle,Team));
        }
    }
    if(GermanCannonReadbackV82)GermanCannonReadbackV82->SetArrayField(TEXT("mounts"),GermanCannonDecisions);
    // Each mount searches its own physical arc. A nearest man behind the
    // tank must not monopolise every side gun and silence a visible front target.
    bool SingleCrewFired=false;
    GunTargetsV828.SetNum(Guns.Num());
    for(int32 I=0;I<Guns.Num();++I)
    {
        GunClock[I]-=DT;if(GunIntegrity[I]<=0)continue;
        if(Type==EFrontlineVehicle::Whippet&&SingleCrewFired)continue;
        AWarfareFrontlineSoldier* GunTarget=nullptr;float Best=MAX_flt;
        FRotator Aim;const FVector Muzzle=WeaponMuzzle(I,false);
        for(AWarfareFrontlineSoldier* U:Battle->Units)if(U&&U->Team!=Team&&U->IsActiveCombatant())
        {
            const float Distance=FVector::Dist2D(bRailConvoyV32?Muzzle:GetActorLocation(),U->Feet());
            if(Distance>=MachinegunRangeV836)continue;
            const float Score=Distance+(Distance>900.f?U->Suppression*650.f:0.f)-(GunTargetsV828[I].Get()==U?180.f:0.f);
            if(Score>=Best)continue;
            FVector Point;if(!InfantryFiringSolutionV828(U,I,false,Point,&InfantrySight,!bNationalGermanGroundV82))continue;
            const FRotator Candidate=(GetActorTransform().InverseTransformPosition(Point)-GunMounts[I]).Rotation();
            if(FMath::Abs(FMath::FindDeltaAngleDegrees(GunBaseYaw[I],float(Candidate.Yaw)))>60.f
                ||Candidate.Pitch< -15.f||Candidate.Pitch>18.f)continue;
            Best=Score;GunTarget=U;Aim=Candidate;
        }
        GunTargetsV828[I]=GunTarget;
        if(!GunTarget)continue;
        Guns[I]->SetRelativeRotation(FMath::RInterpTo(Guns[I]->GetRelativeRotation(),Aim,DT,6.f));
        if(GunClock[I]<=0&&FMath::Abs(FMath::FindDeltaAngleDegrees(Guns[I]->GetRelativeRotation().Yaw,Aim.Yaw))<7.f
            &&FMath::Abs(FMath::FindDeltaAngleDegrees(Guns[I]->GetRelativeRotation().Pitch,Aim.Pitch))<4.f)
        {
            FVector ConfirmedPoint;if(!InfantryFiringSolutionV828(GunTarget,I,false,ConfirmedPoint,&InfantrySight))continue;
            FireMachinegun(I,GunTarget);SingleCrewFired=true;
            if(BurstRemaining[I]<=0)BurstRemaining[I]=Random.RandRange(7,10);
            --BurstRemaining[I];GunClock[I]=BurstRemaining[I]>0?.11f:Random.FRandRange(1.35f,2.05f)*(bNationalGermanGroundV82?1.f:FrontlineArmourV63::MachinegunCycleScaleV8218(Battle,Team));
        }
    }
}
void AFrontlineTank::FireMachinegun(int32 Index,AWarfareFrontlineSoldier* Target)
{
    if(!Battle||Battle->BattleOutroWinner!=INDEX_NONE||!Target||!Target->IsActiveCombatant())return;++MachinegunShots;++Battle->ShotsFired;
    const FVector From=WeaponMuzzle(Index,false);const float Cover=WarfareFRRUCampaignV82::Active(Battle)?WarfareFRRUWorldV82::CoverFrom(Target,From):Battle->CoverFor(Target),SmokeFactor=Battle->DirectFireMultiplier(From,Target->Feet());
    const float FieldShelter=FrontlineTerrainTacticsV54::DirectFireHitScale(From,Target->Feet(),Target->bMoving);
    bool Escorted=false;
    if(!bNationalGermanGroundV82&&FrontlineArmourV63::Has(Battle,6,Team))
        for(const AWarfareFrontlineSoldier* U:Battle->Units)if(U&&U->Team==Team&&U->IsActiveCombatant()
            &&FVector::DistSquared2D(U->Feet(),GetActorLocation())<FMath::Square(900.f)){Escorted=true;break;}
    const float Distance=FVector::Dist2D(From,Target->Feet());
    const float BaseAccuracy=.75f*FMath::Lerp(.105f,.073f,FMath::Clamp(Distance/5100.f,0.f,1.f));
    const float Chance=FrontlineWeapons::FirearmHitChance(FMath::Clamp(BaseAccuracy*(1.f-Cover*.90f)*SmokeFactor*FieldShelter,0.f,.105f)
        *Battle->IncomingBulletHitChanceScale(Target)*(Escorted?1.1f:1.f)*(bNationalGermanGroundV82?1.f:FrontlineArmourV63::CloseGunAccuracyScaleV8218(Battle,Distance,Team)),true);
    Target->ReceiveHit(Random.FRand()<Chance?1:0,.29f*SmokeFactor*Battle->IncomingBulletPressureScale(Target),Team,Target->Feet()-From,EFrontlineDamageKind::Bullet,1.f,FFrontlineKillSourceV65::Vehicle(this));
    if(MachinegunShots%3==0)Battle->Tracers.Add({From,Target->Feet()+FVector(0,0,95),.055f,.055f});
    const auto PressureSight=SightParams(GetWorld(),this,nullptr);
    for(AWarfareFrontlineSoldier* U:Battle->Units)if(U&&U!=Target&&U->Team!=Team&&U->IsActiveCombatant()&&FVector::Dist2D(U->Feet(),Target->Feet())<520)
    {
        FVector Point;if(!InfantryFiringSolutionV828(U,Index,false,Point,&PressureSight))continue;
        const float NearbyCover=Battle->CoverFor(U),Gap=FVector::Dist2D(U->Feet(),Target->Feet());
        U->ReceiveHit(0,.095f*(1.f-NearbyCover*.65f)*FMath::Lerp(1.f,.65f,Gap/520.f)
            *Battle->DirectFireMultiplier(From,Point)*Battle->IncomingBulletPressureScale(U),Team);
    }
    FlashAge[Index]=0;PresentMuzzleFlashV839(Index);
    if(auto* A=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->PlayWeapon(true,Battle->GetFactionForTeam(Team)==EFrontlineFaction::German,From,-1000-Serial);
}
void AFrontlineTank::FireCannon(int32 Index,const FVector& Position,AFrontlineTank* TankTarget)
{
    if(!Battle||Battle->BattleOutroWinner!=INDEX_NONE)return;
    ++CannonShots;++Battle->ShotsFired;FlashAge[Guns.Num()+Index]=0;PresentMuzzleFlashV839(Guns.Num()+Index);const FVector From=WeaponMuzzle(Index,true);
    if(auto* A=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->PlayTankCannon(From);
    FVector Impact;
    if(TankTarget)
    {
        // Armour dispersion belongs in the plane perpendicular to the sight
        // line. The old world-XY offset had no elevation spread and changed
        // accuracy simply when a tank changed heading.
        const FVector Forward=(Position-From).GetSafeNormal();
        const FVector Right=FVector::CrossProduct(FVector::UpVector,Forward).GetSafeNormal();
        const FVector Up=FVector::CrossProduct(Forward,Right).GetSafeNormal();
        const float Scale=FMath::Clamp(float(FVector::Dist(From,Position)/6000.f),.35f,1.f);
        Impact=Position+Right*Random.FRandRange(-90.f,90.f)*Scale+Up*Random.FRandRange(-45.f,45.f)*Scale;
    }
    else Impact=Position+FVector(Random.FRandRange(-150,150),Random.FRandRange(-90,90),0);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(V28TankShell),true);Query.AddIgnoredActor(this);
    for(const AWarfareFrontlineSoldier* U:Battle->Units)if(U)Query.AddIgnoredActor(U);
    FHitResult Hit;if(GetWorld()->LineTraceSingleByChannel(Hit,From,Impact,ECC_Visibility,Query))Impact=Hit.ImpactPoint;
    FShellInFlight Shell;Shell.From=From;Shell.Impact=Impact;Shell.Duration=FMath::Clamp(float(FVector::Dist(From,Impact)/19000.f),.12f,.34f);
    if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")))
    {
        auto* C=NewObject<UStaticMeshComponent>(this);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);
        if(auto* M=Asset<UMaterialInterface>(TEXT("M_Vehicle_Steel")))C->SetMaterial(0,M);
        C->RegisterComponent();C->SetWorldLocation(From);C->SetWorldRotation((Impact-From).Rotation());C->SetWorldScale3D(FVector(.16,.055,.055));Shell.Visual=C;
    }
    Shells.Add(Shell);
}
void AFrontlineTank::UpdateShells(float DT)
{
    for(int32 I=Shells.Num()-1;I>=0;--I)
    {
        auto& S=Shells[I];S.Age+=DT;const float T=FMath::Clamp(S.Age/S.Duration,0.f,1.f);
        const FVector P=FMath::Lerp(S.From,S.Impact,T);
        if(S.Visual.IsValid())S.Visual->SetWorldLocation(P);
        if(T<1.f){const FVector Tail=P+(S.From-S.Impact).GetSafeNormal()*FMath::Min(100.f,float(FVector::Dist(P,S.From)));Battle->Tracers.Add({Tail,P,.035f,.035f});}
        else{if(S.Visual.IsValid())S.Visual->DestroyComponent();ResolveShell(S.Impact);Shells.RemoveAtSwap(I);}
    }
}
void AFrontlineTank::ResolveShell(const FVector& Impact)
{
    if(!Battle||Battle->BattleOutroWinner!=INDEX_NONE)return;
    // One impact route only. Never invoke the global blast hook again for this shell.
    const auto* National=WarfareFRRUArmourV82::Definition(this);
    const float Radius=National?National->ShellRadius:390.f;
    const float InfantryRadius=Radius*(bNationalGermanGroundV82?1.f:FrontlineArmourV63::InfantryShellRadiusScaleV8218(Battle,Team));
    const float Energy=National?National->ShellEnergy:Random.FRandRange(105,135);
    if(auto* S=UFrontlineVehicleSubsystem::Get(GetWorld()))S->ApplyBlast(Impact,Radius,Energy*(bNationalGermanGroundV82?1.f:FrontlineArmourV63::ArmourShellDamageScaleV8218(Battle,Team)),Team,FFrontlineKillSourceV65::Vehicle(this));
    // Actual shell impact applies to either army; no surviving fractional infantry health.
    int32 ShellCasualtiesV8211=0;
    for(AWarfareFrontlineSoldier* U:Battle->Units)if(U&&U->IsActiveCombatant())
    {const float Distance=FVector::Dist2D(U->Feet(),Impact);if(Distance<InfantryRadius){const float P=.86f*FMath::Pow(1.f-Distance/InfantryRadius,1.5f)*(1.f-Battle->CoverFor(U)*.7f);U->ReceiveHit(Random.FRand()<P?1:0,.78f,Team,U->Feet()-Impact,EFrontlineDamageKind::Artillery,.65f,FFrontlineKillSourceV65::Vehicle(this));if(!U->IsAlive())++ShellCasualtiesV8211;}}
    FVector GroundImpact=Impact;
    if(WarfareFRRUCampaignV82::Active(Battle))
    {
        float NativeHeight=0.f;
        if(WarfareFRRUWorldV82::TryHeight(Battle,Impact.X,Impact.Y,NativeHeight))GroundImpact.Z=NativeHeight;
    }
    else GroundImpact.Z=AWarfareFrontlineGround::HeightAt(Impact.X,Impact.Y);
    if(auto* FX=GetWorld()->SpawnActor<AWarfareFrontlineImpact>(GroundImpact,FRotator::ZeroRotator)){FX->ConfigureBlast(false,true);FX->SetCasualtyDustV8211(ShellCasualtiesV8211);}
    if(auto* A=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->PlayImpact(false,GroundImpact);
}
bool AFrontlineTank::IsWheeledV8218() const
{
    if(const auto* National=WarfareFRRUArmourV82::Definition(this))return National->Wheeled;
    return Type==EFrontlineVehicle::Austin1918;
}
bool AFrontlineTank::CanRepairTrackV60(int32 Side) const
{
    if(const auto* National=WarfareFRRUArmourV82::Definition(this);National&&National->Rail)return false;
    return Side>=0&&Side<2&&IsOperational()&&HasEnteredField()&&!bRailConvoyV32
        &&TrackIntegrity[Side]<=0&&!(RepairedTracksV60&(1u<<Side));
}
bool AFrontlineTank::RepairTrackV60(int32 Side)
{
    if(!CanRepairTrackV60(Side))return false;
    if(const auto* National=WarfareFRRUArmourV82::Definition(this))
    {TrackIntegrity[Side]=(National->Wheeled?100.f:230.f)*.5f;RepairedTracksV60|=1u<<Side;return true;}
    TrackIntegrity[Side]=(IsWheeledV8218()?95.f:180.f)*(bNationalGermanGroundV82?.50f:FrontlineArmourV63::RepairRestoredFractionV8218(Battle,Team)*FrontlineArmourV63::TrackScale(Battle,Team));RepairedTracksV60|=1u<<Side;
    return true;
}
int32 AFrontlineTank::RepairableMachinegunV63() const
{
    if(!IsOperational()||!HasEnteredField()||bRailConvoyV32||bMachinegunRepairedV63||bNationalGermanGroundV82||!FrontlineArmourV63::Has(Battle,7,Team))return INDEX_NONE;
    for(int32 I=0;I<GunIntegrity.Num();++I)if(GunIntegrity[I]<=0)return I;
    return INDEX_NONE;
}
bool AFrontlineTank::RepairMachinegunV63(int32 Index)
{
    if(Index<0||RepairableMachinegunV63()!=Index)return false;
    GunIntegrity[Index]=60.f*FrontlineArmourV63::RepairRestoredFractionV8218(Battle,Team)*FrontlineArmourV63::GunIntegrityScale(Battle,Team);
    Guns[Index]->SetRelativeRotation(FRotator(0,GunBaseYaw[Index],0));
    GunClock[Index]=2.f;BurstRemaining[Index]=0;bMachinegunRepairedV63=true;return true;
}
void AFrontlineTank::SetFieldRepairHoldV63(bool Hold)
{
    FieldRepairHoldUntilV63=Hold&&Battle&&RepairableMachinegunV63()!=INDEX_NONE?Battle->Elapsed+.4f:-1.f;
}
bool AFrontlineTank::IsHeldForFieldRepairV63() const
{
    return Battle&&Battle->Elapsed<FieldRepairHoldUntilV63&&RepairableMachinegunV63()!=INDEX_NONE;
}
void AFrontlineTank::ReceiveModuleHit(EFrontlineTankModule Module,float Energy,int32 Index,const FFrontlineKillSourceV65& SourceV65)
{
    if(!IsOperational()||!FMath::IsFinite(Energy)||Energy<=0)return;
    const float HullBefore=HullIntegrity,TracksBefore=TrackIntegrity[0]+TrackIntegrity[1];
    if(Module==EFrontlineTankModule::Hull)HullIntegrity=FMath::Max(0.f,HullIntegrity-Energy);
    else if(Module==EFrontlineTankModule::Machinegun){if(GunIntegrity.IsValidIndex(Index)){GunIntegrity[Index]=FMath::Max(0.f,GunIntegrity[Index]-Energy);if(GunIntegrity[Index]<=0)Guns[Index]->SetRelativeRotation(FRotator(-37,GunBaseYaw[Index],11));}}
    else{const bool WasMobile=!IsImmobilized();const int32 Side=Module==EFrontlineTankModule::LeftTrack?0:1;TrackIntegrity[Side]=FMath::Max(0.f,TrackIntegrity[Side]-Energy);if(WasMobile&&IsImmobilized())CrewWarning(this,EFrontlineVoiceEvent::TracksDisabled);}
    FrontlineReportV839::ArmourDamage(Battle,Team,SourceV65,HullBefore-HullIntegrity,TracksBefore-TrackIntegrity[0]-TrackIntegrity[1]);
    if(HullIntegrity<=0){RailDestructionSourceV837=SourceV65;WarfareFRRUArmourV82::CrewLost(this,SourceV65);FrontlineCombatRecordV65::ArmourDestroyedV8220(Battle,this,SourceV65);Wreck();}
}
int32 AFrontlineTank::RailPassengersV75() const
{
    if(WarfareFRRUArmourV82::National(this))return WarfareFRRUArmourV82::Passengers(this);
    if(!bRailConvoyV32||bRailUnloadedV75||!IsOperational())return 0;
    int32 Aboard=0;
    for(int32 Car=0;Car<2;++Car)
    {
        if(RailCarIntegrityV837_[Car?3:1]<=0)continue;
        const auto* S=Battle?Battle->FindSquad(RailSquadIdsV837[Car]):nullptr;
        Aboard+=S?FMath::Max(0,S->Members.Num()-RailReleasedV837[Car]):
            (Battle?Battle->GetSquadSize(Car?EFrontlineRole::Pioneer:EFrontlineRole::Rifle,Team):Car?4:6);
    }
    return Aboard;
}
bool AFrontlineTank::RailCannonOperationalV837(int32 Mount)const
{return Cannons.IsValidIndex(Mount)&&(!bRailConvoyV32||(Mount>=0&&Mount<2&&RailCarIntegrityV837_[Mount?4:0]>0));}
void AFrontlineTank::ReceiveRailContactV837(const FVector& WorldImpact,float Energy,const FFrontlineKillSourceV65& SourceV65)
{
    if(!bRailConvoyV32||!IsOperational()||WorldImpact.ContainsNaN()||!FMath::IsFinite(Energy)||Energy<=0)return;
    const FVector P=GetActorTransform().InverseTransformPosition(WorldImpact);
    constexpr float Centres[]={2210.f,1320.f,-580.f,-1400.f,-2260.f};
    int32 Car=INDEX_NONE;float Gap=MAX_flt;
    for(int32 I=0;I<5;++I){const float D=FMath::Abs(float(P.X)-Centres[I]);if(D<Gap){Gap=D;Car=I;}}
    if(P.X>=-180.f&&P.X<=740.f)
    {
        RailEngineIntegrityV837_=FMath::Max(0.f,RailEngineIntegrityV837_-Energy*.65f);
        if(RailEngineIntegrityV837_<=0){TrackIntegrity[0]=TrackIntegrity[1]=0;CrewWarning(this,EFrontlineVoiceEvent::TracksDisabled);}
        return;
    }
    if(Car==INDEX_NONE||Gap>450.f)return;
    const bool WasIntact=RailCarIntegrityV837_[Car]>0;
    RailCarIntegrityV837_[Car]=FMath::Max(0.f,RailCarIntegrityV837_[Car]-Energy*.72f);
    if(WasIntact&&RailCarIntegrityV837_[Car]<=0)
    {
        for(int32 I=0;I<GunMounts.Num();++I)if(FMath::Abs(float(GunMounts[I].X)-Centres[Car])<460.f)
            ReceiveModuleHit(EFrontlineTankModule::Machinegun,10000.f,I);
        if(Car==0||Car==4)
        {
            const int32 Mount=Car==0?0:1;
            if(Cannons.IsValidIndex(Mount))Cannons[Mount]->SetRelativeRotation(FRotator(-25,CannonBaseYaw[Mount],9));
            if(RailCupolasV837.IsValidIndex(Mount))RailCupolasV837[Mount]->SetRelativeRotation(FRotator(4,CannonBaseYaw[Mount],6));
        }
        if(Car==1||Car==3)
        {
            const int32 Section=Car==1?0:1;
            if(auto* S=Battle->FindSquad(RailSquadIdsV837[Section]))
                for(auto& Member:S->Members)if(auto* U=Member.Get();U&&U->IsAlive()&&U->IsHidden())
                    U->ReceiveHit(1,0,SourceV65.Team,FVector(0,0,1),EFrontlineDamageKind::Artillery,1.f,SourceV65);
        }
    }
}
void AFrontlineTank::ReceiveImpact(const FVector& Impact,float Energy,bool bExplosive,const FFrontlineKillSourceV65& SourceV65,float ModulePressureScaleV844)
{
    if(!IsOperational()||!FMath::IsFinite(Energy)||Energy<=0||!FMath::IsFinite(ModulePressureScaleV844))return;
    const float ModuleScale=FMath::Clamp(ModulePressureScaleV844,0.f,1.f);
    const FVector P=GetActorTransform().InverseTransformPosition(Impact);
    ReceiveRailContactV837(Impact,Energy*ModuleScale,SourceV65);
    if(P.Z<100&&FMath::Abs(P.Y)>85)ReceiveModuleHit(P.Y<0?EFrontlineTankModule::LeftTrack:EFrontlineTankModule::RightTrack,Energy*(bExplosive?.38f:.55f)*ModuleScale,0,SourceV65);
    int32 Gun=INDEX_NONE;float Near=90;for(int32 I=0;I<GunMounts.Num();++I){const float D=FVector::Dist(P,GunMounts[I]);if(D<Near){Near=D;Gun=I;}}
    if(Gun!=INDEX_NONE)ReceiveModuleHit(EFrontlineTankModule::Machinegun,Energy*.9f*ModuleScale,Gun,SourceV65);
    if(bExplosive)ReceiveModuleHit(EFrontlineTankModule::Hull,Energy*(P.Z<90?.44f:.72f),0,SourceV65);
}
void AFrontlineTank::Wreck()
{
    // Only sections actually materialized for disembarkation can be aboard.
    // Their casualties use the real final hit source and ordinary death path.
    if(bRailConvoyV32&&Battle)for(int32 Car=0;Car<2;++Car)
        if(auto* S=Battle->FindSquad(RailSquadIdsV837[Car]))for(auto& Member:S->Members)
            if(auto* U=Member.Get();U&&U->IsAlive()&&U->IsHidden())
                U->ReceiveHit(1,0,RailDestructionSourceV837.Team,FVector(0,0,1),EFrontlineDamageKind::Artillery,1.f,RailDestructionSourceV837);
    SetActorEnableCollision(false);
    TInlineComponentArray<UPrimitiveComponent*> Components(this);
    for(UPrimitiveComponent* C:Components){C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);}
    TrackIntegrity[0]=TrackIntegrity[1]=0;for(int32 I=0;I<GunIntegrity.Num();++I)GunIntegrity[I]=0;
    for(UStaticMeshComponent* C:Cannons)C->SetRelativeRotation(C->GetRelativeRotation()+FRotator(-12,0,7));
    FFrontlineArmourWreckV8220Access::Apply(this);
    if(Battle)
    {
        FrontlineArmourV63::ReleasePioneerEscortV8445(Battle,this);
        FrontlineMoraleV36::OnArmourDestroyed(Battle,Team,bRailConvoyV32,!bRailUnloadedV75);
        if(Team==0)Battle->LastMessage=bRailConvoyV32?TEXT("装甲列车被击毁。铁路支援中断。"):TEXT("战车被击毁，步兵寻找掩护");
    }
    if(auto* FX=GetWorld()->SpawnActor<AWarfareFrontlineImpact>(GetActorLocation()+FVector(0,0,80),FRotator::ZeroRotator))FX->ConfigureBlast(false,false);
    if(auto* A=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->PlayImpact(false,GetActorLocation());
    CrewWarning(this,EFrontlineVoiceEvent::TankBurning);
    UE_LOG(LogTemp,Display,TEXT("WF_V28_TANK_WRECK type=%d team=%d serial=%d"),int32(Type),Team,Serial);
}
void AFrontlineTank::PresentMuzzleFlashV839(int32 I)
{
    if(!Flashes.IsValidIndex(I)||!FlashAge.IsValidIndex(I)||!Flashes[I])return;
    const bool Cannon=I>=Guns.Num();
    // Short impulse: no brightness envelope spanning almost the entire burst.
    // Present in the firing step, not one vehicle-subsystem step later.
    const float Lifetime=Cannon?.055f:.030f;
    const bool Active=IsOperational()&&FlashAge[I]<Lifetime;
    Flashes[I]->SetVisibility(Active);
    if(!Active)return;
    Flashes[I]->SetWorldLocation(WeaponMuzzle(Cannon?I-Guns.Num():I,Cannon));
    Flashes[I]->SetWorldRotation(Billboard(GetWorld()));
    Flashes[I]->SetWorldScale3D(FVector(Cannon?1.3f:.47f));
    if(FlashMaterials.IsValidIndex(I)&&FlashMaterials[I])
    {
        FlashMaterials[I]->SetVectorParameterValue(TEXT("DustColor"),Cannon?FLinearColor(30,13,3):FLinearColor(22,10,2));
        FlashMaterials[I]->SetScalarParameterValue(TEXT("Density"),1.f);
    }
}
void AFrontlineTank::UpdateEffects(float DT)
{
    if(IsWheeledV8218())for(int32 I=0;I<RoadTurretsV8218.Num();++I)if(RoadTurretsV8218[I]&&Guns.IsValidIndex(I))
        RoadTurretsV8218[I]->SetRelativeRotation(FRotator(0,Guns[I]->GetRelativeRotation().Yaw,0));
    if(bRailConvoyV32)for(int32 I=0;I<RailCupolasV837.Num();++I)
        if(RailCupolasV837[I]&&Cannons.IsValidIndex(I)&&RailCannonOperationalV837(I))
            RailCupolasV837[I]->SetRelativeRotation(FRotator(0,Cannons[I]->GetRelativeRotation().Yaw,0));

    SmokeAge+=DT;const float Damage=FMath::Clamp(1.f-HullIntegrity/FMath::Max(1.f,MaximumHullIntegrity),0.f,1.f);
    for(UMaterialInstanceDynamic* M:BodyMaterials)if(M)M->SetScalarParameterValue(TEXT("VehicleDamage"),Damage);
    if(bRailConvoyV32&&IsOperational())
    {
        for(int32 Mount=0;Mount<RailCupolasV837.Num();++Mount)if(RailCupolasV837[Mount])
            for(int32 Slot=0;Slot<RailCupolasV837[Mount]->GetNumMaterials();++Slot)
                if(auto* M=Cast<UMaterialInstanceDynamic>(RailCupolasV837[Mount]->GetMaterial(Slot)))
                    M->SetScalarParameterValue(TEXT("VehicleDamage"),FMath::Max(Damage,1.f-RailCarIntegrityV837_[Mount?4:0]/260.f));
        if(auto* Fire=AFrontlineFuelFireV33::Get(GetWorld()))
        {
            for(int32 Car=0;Car<5;++Car)if(RailCarIntegrityV837_[Car]<=0)
                Fire->EmitRailCarWreckV838(GetActorTransform(),DT,Car,Serial*.71f+Car*1.63f,Body);
        }
    }
    for(int32 I=0;I<Flashes.Num();++I)
    {
        FlashAge[I]+=DT;PresentMuzzleFlashV839(I);
    }
    const FRotator R=Billboard(GetWorld());
    const bool Steam=bRailConvoyV32&&IsOperational();
    const bool Smoking=(IsOperational()&&Damage>.55f)||Steam;
    if(!IsOperational()&&WreckAge<120)
        if(auto* Fire=AFrontlineFuelFireV33::Get(GetWorld()))
        {
            const float Phase=Serial*.71f+Team*1.37f;
            Fire->EmitVehicleWreckV838(Type,GetActorTransform(),DT,WreckAge,Phase,Body);
        }
    for(int32 I=0;I<Smoke.Num();++I)
    {
        Smoke[I]->SetVisibility(Smoking);if(!Smoking)continue;
        const float Life=Steam?3.8f:5.f;const float Age=FMath::Fmod(SmokeAge+I*.85f,Life);const float Fade=FMath::Sin(Age/Life*PI);
        const FVector Source=GetActorTransform().TransformPosition(Steam?FVector(486.5f,0,410):FVector(-80,0,220));
        Smoke[I]->SetWorldLocation(Source+FVector((Steam?-28.f:36.f)*Age,FMath::Sin(I*1.7f)*18,Age*(Steam?83.f:95.f)));Smoke[I]->SetWorldRotation(R);Smoke[I]->SetWorldScale3D(FVector((Steam?.45f:1.f)+Age*(Steam?.40f:.7f)));
        if(SmokeMaterials[I]){SmokeMaterials[I]->SetScalarParameterValue(TEXT("Density"),Fade*(Steam?.14f:IsOperational()?.28f:.8f));SmokeMaterials[I]->SetVectorParameterValue(TEXT("DustColor"),Steam?FLinearColor(.12f,.125f,.13f):FLinearColor(.045f,.043f,.04f));}
    }
}

TArray<AFrontlineTank*> UFrontlineVehicleSubsystem::SpawnVehicleArtReviewV37()
{
    TArray<AFrontlineTank*> Out;
    FString UserDir;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV37Review"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty()||!Battle())return Out;
    for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)
    {
        const bool German=GermanVehicle(EFrontlineVehicle(I));const int32 InTeam=(Battle()->GetFactionForTeam(0)==EFrontlineFaction::German)==German?0:1;
        if(auto* V=Spawn(EFrontlineVehicle(I),InTeam,true))
        {
            const float Y=German?550.f:-550.f;const int32 Column=Female(EFrontlineVehicle(I))?2:I%2;
            V->SetActorLocation(FVector((Column-1)*1100.f,Y,0));V->HeadingYawV37=0;V->FitGroundV35(V->GetActorLocation(),0);
            V->PreferredLaneV37=V->DesiredLaneV37=Y;V->bEntered=true;Out.Add(V);
        }
    }
    return Out;
}
TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunVehicleChecksV37()
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
    auto Check=[&](const TCHAR* Name,bool Pass){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),Name);R->SetBoolField(TEXT("passed"),Pass);Rows.Add(MakeShared<FJsonValueObject>(R));};
    ON_SCOPE_EXIT{int32 Passed=0;for(const auto& R:Rows)Passed+=R->AsObject()->GetBoolField(TEXT("passed"))?1:0;Report->SetArrayField(TEXT("checks"),Rows);Report->SetNumberField(TEXT("passed"),Passed);Report->SetNumberField(TEXT("total"),Rows.Num());Report->SetBoolField(TEXT("success"),Rows.Num()>0&&Passed==Rows.Num());};
    FString UserDir;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV37Review"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty()||!Battle())
    {Check(TEXT("isolated_v37_vehicle_review_required"),false);return Report;}
    auto* B=Battle();const auto SavedVehicles=Vehicles;const auto SavedUnits=B->Units;const auto SavedSmoke=B->SmokeZones;
    const float M0=B->Morale[0],M1=B->Morale[1],EnemyAt=NextEnemyAt;const int32 SavedSerial=NextSerial,Count0=DeploymentCountV37[0],Count1=DeploymentCountV37[1],Outro=B->BattleOutroWinner;const FString Message=B->LastMessage;
    float SavedReady[2][int32(EFrontlineVehicle::Count)];for(int32 T=0;T<2;++T)for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)SavedReady[T][I]=ReadyAt[T][I];
    const float SavedCycle[2]={ArmourCycleSecondsV40[0],ArmourCycleSecondsV40[1]};
    ON_SCOPE_EXIT{ArmourCycleSecondsV40[0]=SavedCycle[0];ArmourCycleSecondsV40[1]=SavedCycle[1];};
    TArray<AFrontlineTank*> Fixtures;
    ON_SCOPE_EXIT{for(auto* V:Fixtures)if(IsValid(V))V->Destroy();Vehicles=SavedVehicles;B->Units=SavedUnits;B->SmokeZones=SavedSmoke;B->Morale[0]=M0;B->Morale[1]=M1;B->LastMessage=Message;B->BattleOutroWinner=Outro;NextEnemyAt=EnemyAt;NextSerial=SavedSerial;DeploymentCountV37[0]=Count0;DeploymentCountV37[1]=Count1;for(int32 T=0;T<2;++T)for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)ReadyAt[T][I]=SavedReady[T][I];};
    Vehicles.Empty();B->Units.Empty();B->SmokeZones.Empty();B->BattleOutroWinner=INDEX_NONE;DeploymentCountV37[0]=DeploymentCountV37[1]=0;
    TSet<int32> Lanes[2];bool Assets=true;
    for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)for(int32 T=0;T<2;++T)
    {
        auto* V=Spawn(EFrontlineVehicle(I),T,true);if(!V){Assets=false;continue;}Fixtures.Add(V);Lanes[T].Add(FMath::RoundToInt(V->GetActorLocation().Y));
        Assets&=V->AreAssetsReady()&&V->MarkingsV37.Num()==1;
        for(UMaterialInstanceDynamic* M:V->BodyMaterials)Assets&=M&&M->GetBaseMaterial()->GetPathName().StartsWith(TEXT("/Game/Warfare/Vehicles/V37/"));
    }
    Check(TEXT("interleaved_recruitment_retains_four_distinct_lanes_per_army"),Lanes[0].Num()==4&&Lanes[1].Num()==4);
    Check(TEXT("all_vehicle_variants_use_v37_surface_parents_and_bonded_markings"),Assets&&Fixtures.Num()==2*int32(EFrontlineVehicle::Count));
    for(int32 T=0;T<2;++T)for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)ReadyAt[T][I]=100.f+T*10+I;
    const int32 BeforeCount=Vehicles.Num();ResetReadinessForDebug(-1);ResetReadinessForDebug(2);bool InvalidIntact=ReadyAt[0][0]==100&&ReadyAt[1][3]==113;
    ResetReadinessForDebug(0);bool ReadyValid=true;for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)ReadyValid&=ReadyAt[0][I]==0&&ReadyAt[1][I]==110+I;
    Check(TEXT("debug_readiness_changes_only_requested_army_cooldowns"),InvalidIntact&&ReadyValid&&Vehicles.Num()==BeforeCount&&NextEnemyAt==EnemyAt);
    {
        // Exercise the same lane score against the authored, physically sampled
        // St Quentin cut. Restore the active battlefield before moving fixtures.
        int32 OldOp;EFrontlineMissionTerrainV32 OldStyle;FrontlineFieldV32::TerrainProfileV43(OldOp,OldStyle);
        const float OldDepth=FrontlineBattlefield::OperationDepthScale;
        ON_SCOPE_EXIT{FrontlineFieldV32::TerrainProfileV43(OldOp,OldStyle,true);
            FrontlineBattlefield::ConfigureOperationDepth(OldDepth,OldOp);};
        int32 CanalOp=16;auto CanalStyle=EFrontlineMissionTerrainV32::Canal;
        FrontlineFieldV32::TerrainProfileV43(CanalOp,CanalStyle,true);
        FrontlineBattlefield::ConfigureOperationDepth(FrontlineGameplayV33::OperationDepthScale(CanalOp),CanalOp);
        constexpr float CandidateLanes[]={-900.f,-300.f,300.f,900.f};
        const auto Actual=[](float X,float Y){return AWarfareFrontlineGround::QueryTerrainFeaturesV54(X,Y);};
        const auto Flat=[](float,float){return FFrontlineTerrainFeaturesV54{};};
        bool BothDirections=true,FlatKeepsLane=true,GapIsCheaper=true;
        float ChosenLanes[2][2]={};int32 DirectionIndex=0;
        for(int32 Direction:{1,-1})
        {
            for(int32 Side:{-1,1})
            {
                const float Centre=Side*300.f,Breach=Side*900.f;
                const FVector Start(-Direction*2100.f,Centre,0.f);
                float ActualBest=FLT_MAX,FlatBest=FLT_MAX,ActualLane=0.f,FlatLane=0.f;
                for(float Lane:CandidateLanes)
                {
                    const float Physical=VehicleLaneScoreV55(Start,Lane,Centre,Centre,Direction,true,Actual);
                    const float Plain=VehicleLaneScoreV55(Start,Lane,Centre,Centre,Direction,true,Flat);
                    if(Physical<ActualBest){ActualBest=Physical;ActualLane=Lane;}
                    if(Plain<FlatBest){FlatBest=Plain;FlatLane=Lane;}
                }
                // Each army should turn toward either real, graded opening
                // from its nearby centre lane; a featureless field stays put.
                BothDirections&=ActualLane==Breach;
                FlatKeepsLane&=FlatLane==Centre;
                GapIsCheaper&=CrossingLaneCostV55(Start,Breach,Direction,Actual)
                    +350.f<CrossingLaneCostV55(Start,Centre,Direction,Actual);
                ChosenLanes[DirectionIndex][Side<0?0:1]=ActualLane;
            }
            ++DirectionIndex;
        }
        Report->SetNumberField(TEXT("v55_canal_north_lane_british_y_cm"),ChosenLanes[0][1]);
        Report->SetNumberField(TEXT("v55_canal_south_lane_british_y_cm"),ChosenLanes[0][0]);
        Report->SetNumberField(TEXT("v55_canal_north_lane_german_y_cm"),ChosenLanes[1][1]);
        Report->SetNumberField(TEXT("v55_canal_south_lane_german_y_cm"),ChosenLanes[1][0]);
        Check(TEXT("v55_both_playable_canal_breaches_redirect_both_armies_tanks"),BothDirections&&FlatKeepsLane&&GapIsCheaper);
    }
    if(Fixtures.Num()<2)return Report;
    auto* Probe=Fixtures[0];auto* Blocker=Fixtures[1];Vehicles={Probe,Blocker};
    bool PassDisabled=true,PassWreck=true,Bounded=true;double MaxStep=0;
    for(int32 Test=0;Test<4;++Test)
    {
        Probe->Team=Test%2;Blocker->Team=Probe->Team;const int32 Dir=B->GetTeamDirection(Probe->Team);
        Probe->HullIntegrity=350;Probe->TrackIntegrity[0]=Probe->TrackIntegrity[1]=180;Probe->HeadingYawV37=Dir>0?0:180;Probe->PreferredLaneV37=Probe->DesiredLaneV37=-300;Probe->RouteDecisionAtV37=0;Probe->bEntered=true;
        Probe->SetActorLocation(FVector(-Dir*2200,-300,0));Probe->FitGroundV35(Probe->GetActorLocation(),0);
        Blocker->HullIntegrity=350;Blocker->TrackIntegrity[0]=Blocker->TrackIntegrity[1]=180;Blocker->HeadingYawV37=Probe->HeadingYawV37;Blocker->SetActorLocation(FVector(0,-300,0));Blocker->FitGroundV35(Blocker->GetActorLocation(),0);Blocker->bEntered=true;
        if(Test<2)Blocker->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,181);else Blocker->ReceiveModuleHit(EFrontlineTankModule::Hull,1000);
        bool Diverged=false;
        for(int32 Step=0;Step<600;++Step)
        {
            const FVector Before=Probe->GetActorLocation();Probe->Simulate(.1f);const float Moved=FVector::Dist2D(Before,Probe->GetActorLocation());MaxStep=FMath::Max(MaxStep,double(Moved));
            Bounded&=Moved<=Definition(Probe->Type).SpeedCm*.1001f&&FMath::Abs(Probe->GetActorLocation().Y)<=980.1f;
            Diverged|=FMath::Abs(Probe->GetActorLocation().Y+300.f)>450.f;
            if(Probe->GetActorLocation().X*Dir>1100.f)break;
        }
        const bool Passed=Probe->GetActorLocation().X*Dir>1100.f&&Diverged;
        if(Test<2)PassDisabled&=Passed;else PassWreck&=Passed;
    }
    Check(TEXT("both_armies_drive_around_disabled_friendly_tank"),PassDisabled);
    Check(TEXT("both_armies_drive_around_wreck_without_queue_lock"),PassWreck);
    Check(TEXT("bypass_preserves_speed_cap_continuous_motion_and_frontage_bounds"),Bounded);Report->SetNumberField(TEXT("maximum_motion_step_cm"),MaxStep);
    Vehicles={Probe};Probe->HeadingYawV37=0;Probe->SetActorLocation(FVector(0,0,0));Probe->FitGroundV35(Probe->GetActorLocation(),0);Probe->HullIntegrity=350;Probe->TrackIntegrity[0]=Probe->TrackIntegrity[1]=180;Probe->bEntered=true;
    const FVector Contact=Probe->GetActorTransform().TransformPosition(FVector(0,-190,35));
    ApplyBlast(Contact,400,180,1-Probe->Team);
    Check(TEXT("one_full_energy_field_shell_does_not_instantly_cripple_fresh_tracks"),!Probe->IsImmobilized()&&FMath::IsNearlyEqual(Probe->TrackIntegrity[0],111.6f,.01f));
    int32 TrackContacts=0,HullContacts=0;float MaxTrackLoss=0,MaxHullLoss=0;bool NoMissPenalty=true;
    Probe->Random.Initialize(371917);
    for(int32 I=0;I<1000;++I)
    {
        Probe->HullIntegrity=350;Probe->TrackIntegrity[0]=Probe->TrackIntegrity[1]=180;ApplyGrenadeBlast(Contact,500,1-Probe->Team);
        const float TL=180-Probe->TrackIntegrity[0],HL=350-Probe->HullIntegrity;TrackContacts+=TL>0;HullContacts+=HL>0;MaxTrackLoss=FMath::Max(MaxTrackLoss,TL);MaxHullLoss=FMath::Max(MaxHullLoss,HL);NoMissPenalty&=!Probe->IsImmobilized();
    }
    Check(TEXT("fragmentation_grenades_have_limited_real_contact_damage_without_one_hit_stun"),TrackContacts==1000&&HullContacts==1000&&MaxTrackLoss<=42.01f&&MaxHullLoss<=12.01f&&NoMissPenalty);
    Report->SetNumberField(TEXT("seeded_point_blank_grenade_track_hits_of_1000"),TrackContacts);Report->SetNumberField(TEXT("seeded_point_blank_grenade_hull_hits_of_1000"),HullContacts);
    const float HullBefore=Probe->HullIntegrity,TrackBefore=Probe->TrackIntegrity[0];
    for(int32 I=0;I<40;++I)ApplyGrenadeBlast(Probe->GetActorTransform().TransformPosition(FVector(0,-350,35)),500,1-Probe->Team);
    Check(TEXT("wide_infantry_blast_radius_does_not_extend_grenade_armour_contact"),HullBefore==Probe->HullIntegrity&&TrackBefore==Probe->TrackIntegrity[0]);
    const FVector Shooter=Probe->GetActorLocation()+FVector(-1600,0,0),Near=Probe->GrenadeAimPointFor(Shooter);
    Check(TEXT("grenade_aim_uses_nearest_visible_hull_edge_instead_of_centre"),FVector::Dist2D(Shooter,Near)<1250&&FVector::Dist2D(Shooter,Probe->AimPoint())>1500);
    {
        const auto Phase=B->Phase;const int32 InitialStrikes=B->Strikes.Num(),InitialThrown=B->GrenadesThrown;
        AWarfareFrontlineSoldier* Thrower=GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
        ON_SCOPE_EXIT{for(int32 I=InitialStrikes;I<B->Strikes.Num();++I)if(B->Strikes[I].Projectile.IsValid())B->Strikes[I].Projectile->Destroy();B->Strikes.SetNum(InitialStrikes);B->GrenadesThrown=InitialThrown;B->Phase=Phase;if(IsValid(Thrower))Thrower->Destroy();};
        bool Acquired=false,Prepared=false,Released=false;
        if(Thrower)
        {
            B->Phase=EFrontlinePhase::Fighting;Probe->HullIntegrity=350;Probe->TrackIntegrity[0]=Probe->TrackIntegrity[1]=180;
            Probe->SetActorLocation(FVector(-4500,900,0));Probe->HeadingYawV37=0;Probe->FitGroundV35(Probe->GetActorLocation(),0);
            Thrower->Initialize(B,370091,1-Probe->Team,EFrontlineRole::Mills,0);const FVector TargetFeet=Probe->GetActorLocation()+FVector(-1600,0,0);
            Thrower->SetActorLocation(FVector(TargetFeet.X,TargetFeet.Y,AWarfareFrontlineGround::HeightAt(TargetFeet.X,TargetFeet.Y)+88));
            Thrower->bFieldEntered=true;Thrower->bMoving=false;Thrower->WeaponPhase=EFrontlineWeaponPhase::Ready;
            FVector Aim;Acquired=AcquireGrenadeTarget(Thrower,FrontlineGrenade::MaxRangeCm,Aim)==Probe&&FVector::Dist2D(Thrower->Feet(),Aim)<=FrontlineGrenade::MaxRangeCm&&FVector::Dist2D(Thrower->Feet(),Probe->AimPoint())>FrontlineGrenade::MaxRangeCm;
            if(Acquired)
            {
                Thrower->StartGrenadeThrow(nullptr,Probe);
                Prepared=Thrower->WeaponPhase==EFrontlineWeaponPhase::GrenadeThrow&&Thrower->GrenadeThrowsStarted==1&&B->Strikes.Num()==InitialStrikes&&Thrower->GrenadesReleased==0;
                Thrower->ReleaseGrenade();Thrower->ReleaseGrenade();
                Released=Thrower->GrenadesReleased==1&&B->Strikes.Num()==InitialStrikes+1&&B->Strikes.Last().bGrenade&&B->Strikes.Last().ImpactAt>B->Elapsed+1.f;
            }
        }
        Check(TEXT("actual_assault_targeting_acquires_reachable_hull_when_centre_is_out_of_range"),Acquired);
        Check(TEXT("anti_armour_throw_enters_real_animation_before_one_notify_releases_flying_grenade"),Prepared&&Released);
    }
    Report->SetStringField(TEXT("scope"),TEXT("Actual production vehicle spawning, Simulate routing, module/blast paths and seeded grenade contacts. No stand-in combat simulation; integrated throwing animation is reviewed by the caller."));
    return Report;
}
struct FFrontlineArmourConfigurationV63Review
{
    static void RunV8218(UFrontlineVehicleSubsystem* Fleet,TFunctionRef<void(const FString&,bool)> Check)
    {
        auto* B=Fleet?Fleet->Battle():nullptr;if(!FrontlineArmourV63::ReviewRequestedV8218(B))return;
        const auto Runs=B->CampaignRuns;const auto Faction=B->PlayerFaction;const auto Active=B->ActiveCampaignId,Bound=B->BoundCampaignId,Binding=B->BoundCampaignEncounterId;
        const auto Units=B->Units;const auto Squads=B->Squads;const auto FleetBefore=Fleet->Vehicles;
        const int32 Mission=B->BoundCampaignMission,Attachment=B->BoundAttachmentStageV53,SquadId=B->NextSquadId,Serial=Fleet->NextSerial,Outro=B->BattleOutroWinner;
        const auto Phase=B->Phase;const auto Random=B->Random;const auto Message=B->LastMessage;const float Elapsed=B->Elapsed,Morale0=B->Morale[0],Morale1=B->Morale[1];
        const int32 Deployments[2]={Fleet->DeploymentCountV37[0],Fleet->DeploymentCountV37[1]};
        float Ready[2][int32(EFrontlineVehicle::Count)];FMemory::Memcpy(Ready,Fleet->ReadyAt,sizeof(Ready));
        const float Cycle[2]={Fleet->ArmourCycleSecondsV40[0],Fleet->ArmourCycleSecondsV40[1]};
        TSet<AActor*> Existing;for(TActorIterator<AActor> I(B->GetWorld());I;++I)Existing.Add(*I);
        ON_SCOPE_EXIT{
            for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(!Existing.Contains(*I))I->Destroy();
            B->CampaignRuns=Runs;B->PlayerFaction=Faction;B->ActiveCampaignId=Active;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=Binding;
            B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;B->NextSquadId=SquadId;B->Units=Units;B->Squads=Squads;B->Random=Random;
            B->Phase=Phase;B->BattleOutroWinner=Outro;B->LastMessage=Message;B->Elapsed=Elapsed;B->Morale[0]=Morale0;B->Morale[1]=Morale1;Fleet->Vehicles=FleetBefore;Fleet->NextSerial=Serial;
            FMemory::Memcpy(Fleet->ReadyAt,Ready,sizeof(Ready));for(int32 T=0;T<2;++T){Fleet->ArmourCycleSecondsV40[T]=Cycle[T];Fleet->DeploymentCountV37[T]=Deployments[T];}
        };
        B->Units.Empty();B->Squads.Empty();Fleet->Vehicles.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=1000;
        for(int32 Nation=0;Nation<2;++Nation)
        {
            FFrontlineCampaignProgress Run;Run.Id=FGuid::NewGuid();Run.Faction=EFrontlineFaction(Nation);Run.SelectedMission=Run.CompletedMissions=19;Run.ArmourResearchMaskV63=65535;
            B->CampaignRuns={Run};B->PlayerFaction=Run.Faction;B->ActiveCampaignId=B->BoundCampaignId=Run.Id;B->BoundCampaignEncounterId=B->GetEncounterId();B->BoundCampaignMission=B->BoundAttachmentStageV53=19;
            const bool DE=Nation==1;const FString Prefix=DE?TEXT("v8218_de_"):TEXT("v8218_uk_");
            auto& Record=B->CampaignRuns[0];
            const auto Base=DE?EFrontlineVehicle::BeuteMarkIV:EFrontlineVehicle::MarkIVMale,Alternative=DE?EFrontlineVehicle::BeuteMarkIVFemale:EFrontlineVehicle::MarkIVFemale;
            const auto Roster=Fleet->GetRoster();bool AllUnlocked=true;for(const auto& D:Roster)AllUnlocked&=Fleet->IsUnlocked(D.Type);
            Check(Prefix+TEXT("actual_platform_roster_and_dated_late_unlocks"),Roster.Num()==(DE?2:5)&&AllUnlocked&&!Fleet->IsUnlocked(DE?EFrontlineVehicle::BeutePioneer:EFrontlineVehicle::MarkIVPioneer));
            Record.ArmourSelectionMaskV63=(1u<<int32(Base))|(1u<<int32(Alternative));
            const uint32 Eligible=(1u<<int32(EFrontlineVehicle::Count))-1;
            const uint32 Choice=FrontlineArmourV63::GetSelectedMask(B,Eligible);
            Check(Prefix+TEXT("same_platform_variant_cannot_consume_two_slots"),FrontlineCampaignV32::BitCount(Choice)==1);
            Record.ArmourSelectionMaskV63=1u<<int32(Alternative);
            Check(Prefix+TEXT("actual_roster_uses_selected_same_slot_mg_configuration"),FrontlineArmourV63::ConfigurationV8218(B,Base)==Alternative&&Fleet->GetRoster().ContainsByPredicate([&](const auto& D){return D.Type==Alternative;}));
            Record.ArmourSelectionMaskV63=1u<<int32(Base);
            AFrontlineTank* HeavyTank=Fleet->Spawn(Base,0,true);
            Check(Prefix+TEXT("researched_hull_and_running_gear_are_actual_actor_values"),HeavyTank&&HeavyTank->AreAssetsReady()&&FMath::IsNearlyEqual(HeavyTank->MaximumHullIntegrity,Fleet->Definition(Base).Hull*(DE?1.22f:1.18f),.02f)&&FMath::IsNearlyEqual(HeavyTank->TrackIntegrity[0],180.f*(DE?1.18f:1.25f),.02f));
            if(HeavyTank)
            {
                HeavyTank->bEntered=true;
                HeavyTank->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,HeavyTank->TrackIntegrity[0]+1);
                const bool TrackRepair=HeavyTank->RepairTrackV60(0);
                HeavyTank->ReceiveModuleHit(EFrontlineTankModule::Machinegun,HeavyTank->GetMachinegunIntegrity(0)+1,0);
                const bool GunRepair=HeavyTank->RepairMachinegunV63(0);
                Check(Prefix+TEXT("field_recovery_restores_actual_track_and_gun_to_seventy_five_percent"),TrackRepair&&GunRepair&&FMath::IsNearlyEqual(HeavyTank->TrackIntegrity[0],180.f*.75f*(DE?1.18f:1.25f),.02f)&&FMath::IsNearlyEqual(HeavyTank->GunIntegrity[0],60.f*.75f*(DE?1.35f:1.2f),.02f));
            }
            Check(Prefix+TEXT("new_research_scales_feed_speed_burst_armour_and_infantry_effects"),FMath::IsNearlyEqual(FrontlineArmourV63::TravelSpeedScaleV8218(B),DE?1.10f:1.12f)&&FMath::IsNearlyEqual(FrontlineArmourV63::MachinegunCycleScaleV8218(B),DE?.75f:.80f)&&FMath::IsNearlyEqual(FrontlineArmourV63::ArmourShellDamageScaleV8218(B),DE?1.30f:1.25f)&&FMath::IsNearlyEqual(FrontlineArmourV63::InfantryShellRadiusScaleV8218(B),DE?1.20f:1.25f)&&FMath::IsNearlyEqual(FrontlineArmourV63::CloseGunAccuracyScaleV8218(B,1201),1.f)&&FrontlineArmourV63::CloseGunAccuracyScaleV8218(B,1200)>1.2f);
            if(!DE)
            {
                auto* Star=Fleet->Spawn(EFrontlineVehicle::MarkVStar,0,true);auto* Car=Fleet->Spawn(EFrontlineVehicle::Austin1918,0,true);
                Check(TEXT("v8218_mark_v_star_real_extended_body_tracks_two_cannon_and_four_carried_hotchkiss"),Star&&Star->AreAssetsReady()&&Star->Cannons.Num()==2&&Star->Guns.Num()==4&&Star->GunMounts.Num()==4&&Star->CannonMounts.Num()==2&&FMath::IsNearlyEqual(float(Star->CannonMounts[0].X),156.f,.01f)&&FMath::IsNearlyEqual(float(Star->CannonMounts[1].X),156.f,.01f)&&Star->GunMounts[0].Equals(FVector(341,0,177),.01)&&Star->GunMounts[1].Equals(FVector(-29,-177,170),.01)&&Star->GunMounts[2].Equals(FVector(-29,177,170),.01)&&Star->GunMounts[3].Equals(FVector(-350,0,254),.01)&&Fleet->Definition(EFrontlineVehicle::MarkVStar).MachinegunCount==4);
                bool Wheels=Car&&Car->AreAssetsReady()&&Car->RoadWheelsV8218.Num()==4&&Car->RoadTurretsV8218.Num()==2&&Car->Cannons.IsEmpty()&&Car->Guns.Num()==2&&Car->TrackLinks->GetInstanceCount()==0;
                if(Car){const float Offset=Car->TrackOffset;Car->UpdateTracks(64);Wheels&=Car->TrackOffset>Offset&&FMath::Abs(Car->RoadWheelsV8218[0]->GetRelativeRotation().Pitch)>1.f;
                    if(Car->Guns.IsValidIndex(0))Wheels&=Car->Guns[0]->GetComponentTransform().InverseTransformPosition(Car->WeaponMuzzle(0,false)).Equals(FVector(81.5,0,0),.01);
                    else Wheels=false;}
                Check(TEXT("v8218_austin_real_four_wheels_twin_turrets_vickers_and_no_track_belt"),Wheels);
                Check(TEXT("v8218_heavy_star_and_fast_wheeled_car_have_distinct_shared_readiness"),Fleet->Definition(EFrontlineVehicle::Austin1918).Cooldown==54&&Fleet->Definition(EFrontlineVehicle::MarkVStar).Cooldown==142&&Fleet->Definition(EFrontlineVehicle::Austin1918).SpeedCm>Fleet->Definition(EFrontlineVehicle::MarkVStar).SpeedCm*2);
            }
            for(AFrontlineTank* V:Fleet->Vehicles)if(IsValid(V))V->Destroy();Fleet->Vehicles.Empty();
        }
    }
    static void Run(UFrontlineVehicleSubsystem* Fleet,TFunctionRef<void(const FString&,bool)> Check)
    {
        auto* B=Fleet?Fleet->Battle():nullptr;FString UserDir;
        const bool Isolated=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV63Review"))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
        Check(TEXT("v63_armour_requires_disposable_no_save_profile"),Isolated);if(!Isolated)return;
        const auto OldRuns=B->CampaignRuns;const auto OldFaction=B->PlayerFaction;
        const auto OldActive=B->ActiveCampaignId,OldBound=B->BoundCampaignId,OldBinding=B->BoundCampaignEncounterId;
        const int32 OldMission=B->BoundCampaignMission,OldAttachment=B->BoundAttachmentStageV53,OldSquadId=B->NextSquadId;
        const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldVehicles=Fleet->Vehicles;
        const auto OldPhase=B->Phase;const auto OldRandom=B->Random;const auto OldMessage=B->LastMessage;
        const float OldElapsed=B->Elapsed;const int32 OldOutro=B->BattleOutroWinner,OldSerial=Fleet->NextSerial;
        const auto OldEncounter=Fleet->LastEncounter;const bool OldPause=UGameplayStatics::IsGamePaused(B);
        float OldReady[2][int32(EFrontlineVehicle::Count)];FMemory::Memcpy(OldReady,Fleet->ReadyAt,sizeof(OldReady));
        const float OldCycle[2]={Fleet->ArmourCycleSecondsV40[0],Fleet->ArmourCycleSecondsV40[1]};
        const int32 OldDeployments[2]={Fleet->DeploymentCountV37[0],Fleet->DeploymentCountV37[1]};
        struct FActorState {AActor* Actor;bool Hidden,Collision;};TArray<FActorState> ActorStates;
        auto Hide=[&](AActor* A){if(IsValid(A)){ActorStates.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
        for(AWarfareFrontlineSoldier* U:OldUnits)Hide(U);for(AFrontlineTank* V:OldVehicles)Hide(V);
        TSet<AActor*> Existing;for(TActorIterator<AActor> I(B->GetWorld());I;++I)Existing.Add(*I);
        ON_SCOPE_EXIT
        {
            for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(!Existing.Contains(*I))I->Destroy();
            for(const auto& S:ActorStates)if(IsValid(S.Actor)){S.Actor->SetActorHiddenInGame(S.Hidden);S.Actor->SetActorEnableCollision(S.Collision);}
            B->CampaignRuns=OldRuns;B->PlayerFaction=OldFaction;B->ActiveCampaignId=OldActive;B->BoundCampaignId=OldBound;
            B->BoundCampaignEncounterId=OldBinding;B->BoundCampaignMission=OldMission;B->BoundAttachmentStageV53=OldAttachment;
            B->NextSquadId=OldSquadId;B->Units=OldUnits;B->Squads=OldSquads;B->Random=OldRandom;
            B->Phase=OldPhase;B->Elapsed=OldElapsed;B->BattleOutroWinner=OldOutro;B->LastMessage=OldMessage;
            Fleet->Vehicles=OldVehicles;Fleet->LastEncounter=OldEncounter;Fleet->NextSerial=OldSerial;
            FMemory::Memcpy(Fleet->ReadyAt,OldReady,sizeof(OldReady));
            for(int32 T=0;T<2;++T){Fleet->ArmourCycleSecondsV40[T]=OldCycle[T];Fleet->DeploymentCountV37[T]=OldDeployments[T];}
            UGameplayStatics::SetGamePaused(B,OldPause);
        };
        B->Units.Empty();B->Squads.Empty();Fleet->Vehicles.Empty();B->Elapsed=1000;B->BattleOutroWinner=INDEX_NONE;
        B->Phase=EFrontlinePhase::Fighting;Fleet->LastEncounter=B->GetEncounterId();Fleet->NextSerial=639000;
        UGameplayStatics::SetGamePaused(B,false);
        for(int32 F=0;F<2;++F)
        {
            const bool German=F==1;const FString Prefix=German?TEXT("v63_de_"):TEXT("v63_uk_");
            FFrontlineCampaignProgress Run;Run.Id=FGuid::NewGuid();Run.Faction=EFrontlineFaction(F);
            Run.SelectedMission=Run.CompletedMissions=19;Run.ArmourResearchMaskV63=0xff;
            const auto Specialist=German?EFrontlineVehicle::A7VGretchen:EFrontlineVehicle::MarkVMale;
            const auto Escort=German?EFrontlineVehicle::BeutePioneer:EFrontlineVehicle::MarkIVPioneer;
            Run.ArmourSelectionMaskV63=(1u<<int32(Specialist))|(1u<<int32(Escort));
            B->CampaignRuns={Run};B->PlayerFaction=Run.Faction;B->ActiveCampaignId=B->BoundCampaignId=Run.Id;
            B->BoundCampaignEncounterId=B->GetEncounterId();B->BoundCampaignMission=B->BoundAttachmentStageV53=19;
            const auto Roster=Fleet->GetRoster();uint32 RosterMask=0;bool AllUnlocked=true;
            for(const auto& D:Roster){RosterMask|=1u<<int32(D.Type);AllUnlocked&=Fleet->IsUnlocked(D.Type);}
            const uint32 Expected=German?((1u<<2)|(1u<<3)|(1u<<5)|(1u<<8)|(1u<<9)):((1u<<0)|(1u<<1)|(1u<<4)|(1u<<6)|(1u<<7));
            const auto Equipped=Fleet->GetEquippedRoster();
            Check(Prefix+TEXT("five_legal_configurations_two_selected"),Roster.Num()==5&&RosterMask==Expected&&AllUnlocked
                &&Equipped.Num()==2&&Equipped.ContainsByPredicate([&](const auto& D){return D.Type==Specialist;})
                &&Equipped.ContainsByPredicate([&](const auto& D){return D.Type==Escort;}));
            Fleet->ResetReadinessForDebug();
            Check(Prefix+TEXT("unselected_configuration_cannot_recruit"),!Fleet->RecruitBlockReason(Roster[0].Type).IsEmpty());
            const int32 VehicleStart=Fleet->Vehicles.Num();const bool Recruited=Fleet->Recruit(Specialist);
            AFrontlineTank* Special=Recruited&&Fleet->Vehicles.Num()==VehicleStart+1?Fleet->Vehicles.Last().Get():nullptr;
            Check(Prefix+TEXT("derived_hull_and_correct_mounts_load"),Special&&Special->AreAssetsReady()
                &&Special->Guns.Num()==(German?8:4)&&Special->Cannons.Num()==(German?0:2)
                &&(German?(Special->GunMounts[6].X>340&&Special->GunMounts[7].X>340):(Special->GunMounts[3].Z>250)));
            const float ExpectedCycle=FrontlineGameplayV43::PreparationSeconds(Fleet->Definition(Specialist).Cooldown
                *FrontlinePacingV24::TempoIntervalScale*FrontlineArmourV63::ReadinessScale(B,Specialist),0,B->GetCampaignDifficulty());
            bool Shared=Special&&FMath::IsNearlyEqual(Fleet->ArmourCycleSecondsV40[0],ExpectedCycle,.001f);
            for(float Time:Fleet->ReadyAt[0])Shared&=FMath::IsNearlyEqual(Time,B->Elapsed+ExpectedCycle,.001f);
            const int32 BeforeBlocked=Fleet->Vehicles.Num();Shared&=!Fleet->Recruit(Escort)&&Fleet->Vehicles.Num()==BeforeBlocked;
            Shared&=Fleet->CooldownFraction(Specialist)>.99f&&FMath::IsNearlyEqual(Fleet->CooldownFraction(Specialist),Fleet->CooldownFraction(Escort));
            Check(Prefix+TEXT("both_slots_share_researched_dispatch_cooldown"),Shared);
            Fleet->ResetReadinessForDebug();
            if(German)Check(TEXT("v63_gretchen_single_active_limit_is_explicit"),Special
                &&Fleet->RecruitBlockReason(Specialist)==FrontlineArmourV63::L(TEXT("Gretchen is already in the field."),TEXT("Gretchen 已在战场。"))
                &&!Fleet->Recruit(Specialist)&&Fleet->Vehicles.Num()==BeforeBlocked);
            const int32 UnitStart=B->Units.Num(),SquadStart=B->Squads.Num();const bool Escorted=Fleet->Recruit(Escort);
            bool Two=Escorted&&B->Units.Num()==UnitStart+2&&B->Squads.Num()==SquadStart+1;
            if(Two)
            {
                const auto& S=B->Squads.Last();Two=S.Role==EFrontlineRole::Pioneer&&S.Members.Num()==2&&S.LivingCount()==2&&S.bReinforcement;
                for(const auto& Member:S.Members){const auto* U=Member.Get();Two&=U&&U->Role==EFrontlineRole::Pioneer&&U->IsAlive()&&!U->bFieldEntered;}
            }
            Check(Prefix+TEXT("escort_deploys_two_living_pioneers_from_entry"),Two);
            if(Two&&Special)
            {
                auto* Carrier=Fleet->Vehicles.Last().Get();auto* Engineer=B->Squads.Last().Members[0].Get();
                Carrier->bEntered=Special->bEntered=true;Carrier->SetActorLocation(FVector(0,0,10000));
                Special->SetActorLocation(FVector(12000,0,10000));
                Engineer->bFieldEntered=true;
                FVector First,Second;const bool Linked=FrontlineArmourV63::EscortDestination(Engineer,First);
                Carrier->SetActorLocation(Carrier->GetActorLocation()+FVector(400,150,0));
                const bool Follows=FrontlineArmourV63::EscortDestination(Engineer,Second);
                Engineer->SetActorLocation(Second+FVector(0,0,88));
                for(const auto& Member:B->Squads.Last().Members)if(auto* U=Member.Get())
                {U->bFieldEntered=true;U->bMoving=false;U->Suppression=0;}
                B->UpdateSquads();
                Check(Prefix+TEXT("escort_slots_follow_hull_and_do_not_join_a_trench"),Linked&&Follows
                    &&FMath::IsNearlyEqual(float(Second.X-First.X),400.f,.1f)
                    &&FMath::IsNearlyEqual(float(Second.Y-First.Y),150.f,.1f)
                    &&FrontlineArmourV63::SquadIsEscorting(B,Engineer->SquadId)
                    &&B->FindSquad(Engineer->SquadId)->bAdvancing&&B->FindSquad(Engineer->SquadId)->Trench==INDEX_NONE);
                FrontlineFieldSitesV59::RunRepairChecksV63(B,Carrier,Engineer,Special,Check);
                Carrier->HullIntegrity=0;
                Check(Prefix+TEXT("wreck_releases_escort_to_normal_squad_orders"),!FrontlineArmourV63::EscortFor(Engineer)
                    &&!FrontlineArmourV63::SquadIsEscorting(B,Engineer->SquadId)
                    &&!FrontlineArmourV63::EscortDestination(Engineer,Second));
            }
            // The second faction must not inherit the first faction's fitters.
            for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Empty();B->Squads.Empty();
            for(AFrontlineTank* V:Fleet->Vehicles)if(IsValid(V))V->Destroy();Fleet->Vehicles.Empty();
        }
    }
};
void UFrontlineVehicleSubsystem::RunPlatformChecksV8218(TFunctionRef<void(const FString&,bool)> Check)
{
    FFrontlineArmourConfigurationV63Review::RunV8218(this,Check);
}
void UFrontlineVehicleSubsystem::RunConfigurationChecksV63(TFunctionRef<void(const FString&,bool)> Check)
{
    FFrontlineArmourConfigurationV63Review::Run(this,Check);
}

TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunFocusedChecks()
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
    auto Check=[&](const TCHAR* Name,bool Pass){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),Name);R->SetBoolField(TEXT("passed"),Pass);Rows.Add(MakeShared<FJsonValueObject>(R));};
    ON_SCOPE_EXIT{int32 Passed=0;for(const auto& R:Rows)Passed+=R->AsObject()->GetBoolField(TEXT("passed"))?1:0;Report->SetArrayField(TEXT("checks"),Rows);Report->SetNumberField(TEXT("passed"),Passed);Report->SetNumberField(TEXT("total"),Rows.Num());};
    FString UserDir;if((!FParse::Param(FCommandLine::Get(),TEXT("WFV28Review"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV33Review")))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty()||!Battle())
    {Check(TEXT("isolated_vehicle_review_required"),false);return Report;}
    auto* B=Battle();const float Morale0=B->Morale[0],Morale1=B->Morale[1];const FString Message=B->LastMessage;
    TArray<TObjectPtr<AFrontlineTank>> Fixtures;
    ON_SCOPE_EXIT{for(AFrontlineTank* T:Fixtures)if(IsValid(T))T->Destroy();Vehicles.RemoveAll([&](const TObjectPtr<AFrontlineTank>& T){return Fixtures.Contains(T);});B->Morale[0]=Morale0;B->Morale[1]=Morale1;B->LastMessage=Message;};
    bool Meshes=true,Models=true,Portraits=true;
    for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)
    {
        const EFrontlineVehicle Kind=EFrontlineVehicle(I);const bool German=GermanVehicle(Kind);const int32 InTeam=(B->GetFactionForTeam(0)==EFrontlineFaction::German)==German?0:1;
        auto* V=Spawn(Kind,InTeam,true);if(!V){Meshes=false;continue;}Fixtures.Add(V);Meshes&=V->AreAssetsReady();Portraits&=GetPortrait(Kind)!=nullptr;
        Models&=Definition(Kind).CannonCount==(Kind==EFrontlineVehicle::Whippet||Kind==EFrontlineVehicle::Austin1918||Female(Kind)||Kind==EFrontlineVehicle::A7VGretchen?0:A7VFamily(Kind)?1:2)
            &&V->GetTrackIntegrity(0)>0&&V->Cannons.Num()==Definition(Kind).CannonCount;
        if(Female(Kind)&&!Pioneer(Kind))Models&=V->Guns.Num()==5&&V->Cannons.IsEmpty()&&Definition(Kind).Cooldown<Definition(German?EFrontlineVehicle::BeuteMarkIV:EFrontlineVehicle::MarkIVMale).Cooldown;
    }
    Check(TEXT("all_vehicle_assets_and_separate_moving_track_meshes"),Meshes&&Fixtures.Num()==int32(EFrontlineVehicle::Count));
    Check(TEXT("female_variants_have_five_mg_no_cannon_and_shorter_shared_cycle"),Models);
    Check(TEXT("all_vehicle_portraits_loaded"),Portraits);
    if(Fixtures.IsEmpty())return Report;
    auto* V=Fixtures[0].Get();const FVector BeforeMove=V->GetActorLocation();V->Simulate(.1f);
    Check(TEXT("off_field_entry_moves_in_team_direction_and_turns_tracks"),(V->GetActorLocation().X-BeforeMove.X)*B->GetTeamDirection(V->Team)>0 && V->GetTrackTravelPhase()>0 && !V->HasEnteredField());
    {
        // Two disposable actors isolate perception from other fixtures. Their
        // actual Simulate path chooses targets and either moves or fires.
        const auto SavedVehicles=Vehicles;const auto SavedUnits=B->Units;
        const auto SavedSmoke=B->SmokeZones;const auto SavedTracers=B->Tracers;const int32 SavedShots=B->ShotsFired,SavedSerial=NextSerial;
        const auto SavedPhase=B->Phase;const int32 SavedOutro=B->BattleOutroWinner;const float SavedEnemyAt=NextEnemyAt;
        const FGuid SavedEncounter=LastEncounter;const bool SavedPause=UGameplayStatics::IsGamePaused(GetWorld());
        const int32 GermanTeam=B->GetFactionForTeam(0)==EFrontlineFaction::German?0:1;
        AFrontlineTank* Probe=Spawn(EFrontlineVehicle::A7V,GermanTeam,true);
        AFrontlineTank* Enemy=Spawn(EFrontlineVehicle::MarkIVMale,1-GermanTeam,true);
        ON_SCOPE_EXIT{
            if(IsValid(Probe))Probe->Destroy();if(IsValid(Enemy))Enemy->Destroy();Vehicles=SavedVehicles;
            B->Units=SavedUnits;B->SmokeZones=SavedSmoke;B->Tracers=SavedTracers;B->ShotsFired=SavedShots;NextSerial=SavedSerial;
            B->Phase=SavedPhase;B->BattleOutroWinner=SavedOutro;NextEnemyAt=SavedEnemyAt;LastEncounter=SavedEncounter;UGameplayStatics::SetGamePaused(GetWorld(),SavedPause);
        };
        bool EnteredDespiteContact=false,SmokePreventsShot=false,OutroCeaseFire=false;
        if(Probe && Enemy && Probe->Cannons.Num()==1)
        {
            Vehicles={Probe,Enemy};B->Units.Empty();B->SmokeZones.Empty();
            const int32 Direction=B->GetTeamDirection(Probe->Team);
            auto Place=[&](AFrontlineTank* Tank,float X){Tank->SetActorLocation(FVector(X,0,AWarfareFrontlineGround::HeightAt(X,0)+4));};
            Place(Probe,-Direction*(FrontlineBattlefield::SpawnHalfLength+700));
            Place(Enemy,Probe->GetActorLocation().X+Direction*1600);Enemy->bEntered=true;Probe->bEntered=false;
            const bool VisibleContact=ClearSight(GetWorld(),Probe,Enemy,Probe->AimPoint(),Enemy->AimPoint())
                && FVector::Dist2D(Probe->AimPoint(),Enemy->AimPoint())<Definition(Probe->Type).RangeCm*B->EncounterVisibilityScale();
            const FVector EntryBefore=Probe->GetActorLocation();Probe->CannonClock[0]=0;Probe->Simulate(.1f);
            EnteredDespiteContact=VisibleContact&&!Probe->HasEnteredField()&&Probe->CannonShots==0
                && (Probe->GetActorLocation().X-EntryBefore.X)*Direction>0;

            Place(Probe,-Direction*1000);Place(Enemy,Direction*1000);Probe->bEntered=true;
            Probe->SetActorRotation(FRotator(0,Direction>0?0:180,0));
            const FRotator GunAim=(Probe->GetActorTransform().InverseTransformPosition(Enemy->AimPoint())-Probe->CannonMounts[0]).Rotation();
            Probe->Cannons[0]->SetRelativeRotation(GunAim);Probe->CannonClock[0]=0;
            const int32 ShotsBeforeClear=Probe->CannonShots;Probe->Simulate(.01f);
            const bool ClearControlFired=Probe->CannonShots==ShotsBeforeClear+1&&Probe->Shells.Num()==1;
            // Remove that just-launched control shell before a later tick can
            // resolve it. Both conditions therefore use an unchanged target.
            for(auto& S:Probe->Shells)if(S.Visual.IsValid())S.Visual->DestroyComponent();Probe->Shells.Empty();
            FFrontlineSmokeZone Screen;Screen.Team=Probe->Team;Screen.Centre=FVector::ZeroVector;Screen.Radius=650;
            Screen.StartedAt=B->Elapsed;Screen.ExpiresAt=B->Elapsed+5;B->SmokeZones.Add(Screen);
            Probe->CannonClock[0]=0;const int32 ShotsBeforeSmoke=Probe->CannonShots;Probe->Simulate(.1f);
            SmokePreventsShot=ClearControlFired&&Probe->HasEnteredField()&&Probe->CannonShots==ShotsBeforeSmoke&&Probe->Shells.IsEmpty()
                && B->DirectFireMultiplier(Probe->AimPoint(),Enemy->AimPoint())<.9f;

            // Reuse these two real tanks: a shell is already airborne when
            // surrender begins, with another cannon round and enemy call due.
            B->SmokeZones.Empty();Probe->FireCannon(0,Enemy->AimPoint(),Enemy);
            const bool HadShell=Probe->Shells.Num()==1;const float TargetHull=Enemy->HullIntegrity;
            const FVector StopAt=Probe->GetActorLocation();const float Travel=Probe->DistanceTravelled,SmokeAgeBefore=Probe->SmokeAge;
            const int32 CannonBefore=Probe->CannonShots,MachinegunBefore=Probe->MachinegunShots,AllShotsBefore=B->ShotsFired,VehicleCount=Vehicles.Num();
            Probe->CannonClock[0]=0;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=Probe->Team;
            NextEnemyAt=0;LastEncounter=B->GetEncounterId();UGameplayStatics::SetGamePaused(GetWorld(),false);
            for(int32 I=0;I<4;++I)Tick(.1f);
            OutroCeaseFire=HadShell && Probe->Shells.IsEmpty() && Enemy->HullIntegrity==TargetHull
                && Probe->CannonShots==CannonBefore && Probe->MachinegunShots==MachinegunBefore && B->ShotsFired==AllShotsBefore
                && Probe->GetActorLocation().Equals(StopAt,.001f) && Probe->DistanceTravelled==Travel
                && Probe->SmokeAge>SmokeAgeBefore && Vehicles.Num()==VehicleCount && NextEnemyAt==0
                && !RecruitBlockReason(Probe->Type,Probe->Team).IsEmpty();
        }
        Check(TEXT("off_field_tank_keeps_entering_despite_near_observed_armour"),EnteredDespiteContact);
        Check(TEXT("entered_tank_fires_clear_control_but_not_through_smoke"),SmokePreventsShot);
        Check(TEXT("surrender_cancels_airborne_tank_shells_orders_and_reinforcements_but_keeps_effects"),OutroCeaseFire);
    }
    const float H=V->GetHullIntegrity();V->ReceiveModuleHit(EFrontlineTankModule::Hull,-1);V->ReceiveModuleHit(EFrontlineTankModule::Hull,0);
    Check(TEXT("nonpositive_damage_does_not_repair_or_damage_tank"),V->GetHullIntegrity()==H);
    V->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,V->GetTrackIntegrity(0)+1);
    Check(TEXT("track_destroyed_immobilizes_without_killing_hull"),V->IsImmobilized()&&V->IsOperational()&&V->GetHullIntegrity()==H&&V->GetTrackIntegrity(1)>0);
    const FVector Stopped=V->GetActorLocation();V->Simulate(.1f);Check(TEXT("disabled_track_really_stops_translation"),V->GetActorLocation().Equals(Stopped,.001f));
    V->ReceiveModuleHit(EFrontlineTankModule::Machinegun,61,0);
    Check(TEXT("individual_machinegun_disabled_other_mount_survives"),V->GetMachinegunIntegrity(0)==0&&V->GetMachinegunIntegrity(1)>0&&V->IsOperational());
    const float Before=V->GetHullIntegrity();ApplySmallArmsHit(V,V->AimPoint(),true,1-V->Team);
    Check(TEXT("ordinary_bullets_do_not_erode_armoured_hull"),V->GetHullIntegrity()==Before);
    const auto BeforeGuns=V->GunIntegrity;ApplySmallArmsHit(V,V->AimPoint()+FVector(0,3000,0),true,1-V->Team);
    Check(TEXT("off_hull_bullet_miss_cannot_disable_unrelated_mount"),BeforeGuns==V->GunIntegrity);
    const int32 BeforeShots=B->ShotsFired,BeforeCannon=V->CannonShots;const auto BeforeTracers=B->Tracers;
    const FVector RemoteImpact=V->GetActorLocation()+FVector(0,5000,100);
    V->FireCannon(0,RemoteImpact,nullptr);const bool Pending=V->Shells.Num()==1&&V->Shells[0].Age==0;
    if(Pending)V->UpdateShells(.025f);
    Check(TEXT("tank_shell_keeps_damage_pending_during_real_short_flight"),Pending&&V->Shells.Num()==1&&V->Shells[0].Age>0&&V->Shells[0].Age<V->Shells[0].Duration);
    for(auto& S:V->Shells)if(S.Visual.IsValid())S.Visual->DestroyComponent();V->Shells.Empty();B->ShotsFired=BeforeShots;V->CannonShots=BeforeCannon;B->Tracers=BeforeTracers;
    V->ReceiveModuleHit(EFrontlineTankModule::Hull,999);
    Check(TEXT("penetrating_hull_damage_produces_wreck"),!V->IsOperational()&&V->IsImmobilized());
    Check(TEXT("five_armour_configurations_remain_separate_from_two_equipped_slots"),GetRoster(0).Num()==5&&GetRoster(1).Num()==5&&GetEquippedRoster(0).Num()<=2&&GetEquippedRoster(1).Num()<=2);
    Report->SetStringField(TEXT("scope"),TEXT("Isolated actual asset/module fixtures; natural tank combat and normal-scale image remain separate review."));
    return Report;
}

// The rail convoy is confined to the authored composite Selle sector. Its
// weapons, collision, projectile damage, individual guns and running gear all
// use the same production module paths as armour, not invulnerable scenery.
AFrontlineTank* UFrontlineVehicleSubsystem::SpawnScenarioRailV32(int32 Team)
{
    auto* B=Battle();if(!B||Team<0||Team>1||!RailAvailableV75(Team))return nullptr;
    if(LastEncounter!=B->GetEncounterId()){ClearEncounter();LastEncounter=B->GetEncounterId();}
    for(AFrontlineTank* Existing:Vehicles)if(Existing&&Existing->Team==Team&&Existing->IsRailConvoyV32()&&Existing->IsOperational())return Existing;
    auto* V=Spawn(EFrontlineVehicle::A7V,Team,true);if(!V)return nullptr;
    if(!V->ConfigureRailV32()){Vehicles.Remove(V);V->Destroy();return nullptr;}
    const float X=-B->GetTeamDirection(Team)*(FrontlinePacingV24::CombatHalfLength()+V->FootprintHalfLengthV37()+Definition(EFrontlineVehicle::RailConvoy).SpeedCm*2.8f);
    V->SetActorLocation(FVector(X,-1130,28));
    RailReadyAtV75[Team]=B->Elapsed+RailCooldownSecondsV75;return V;
}
bool UFrontlineVehicleSubsystem::RailAvailableV75(int32 Team)const
{
    if(WarfareFRRUCampaignV82::Active(Battle()))return false;
    const auto* B=Battle();if(!B||Team<0||Team>1||B->GetFactionForTeam(Team)!=EFrontlineFaction::German)return false;
    const auto* M=B->Phase==EFrontlinePhase::Briefing?B->GetSelectedCampaignMission():B->GetEncounterCampaignMission();return M&&M->bArmoredTrain;
}
FString UFrontlineVehicleSubsystem::RailBlockReasonV75(int32 Team)const
{
    const auto* B=Battle();
    if(!RailAvailableV75(Team))return FrontlineArmourV63::L(TEXT("No rail detachment assigned."),TEXT("本次行动未配属铁路分遣队。"));
    if(B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)return FrontlineArmourV63::L(TEXT("Awaiting the field order."),TEXT("等待前线命令。"));
    if(UGameplayStatics::IsGamePaused(GetWorld()))return FrontlineArmourV63::L(TEXT("Battle paused."),TEXT("战斗已暂停。"));
    for(const AFrontlineTank* V:Vehicles)if(IsValid(V)&&V->Team==Team&&V->IsRailConvoyV32()&&V->IsOperational())
        return FrontlineArmourV63::L(TEXT("The rail convoy is already in action."),TEXT("装甲列车已在行动。"));
    return RecruitBlockReason(EFrontlineVehicle::RailConvoy,Team);
}
bool UFrontlineVehicleSubsystem::RequestRailV75(int32 Team)
{
    auto* B=Battle();if(!B)return false;const FString Reason=RailBlockReasonV75(Team);
    if(!Reason.IsEmpty()){if(Team==0)B->LastMessage=Reason;return false;}
    if(!Recruit(EFrontlineVehicle::RailConvoy,Team))return false;
    if(Team==0)B->LastMessage=FrontlineArmourV63::L(TEXT("Rail convoy approaching the forward railhead."),TEXT("装甲列车正驶向前沿铁路。"));
    return true;
}
float UFrontlineVehicleSubsystem::RailCooldownFractionV75(int32 Team)const
{
    return CooldownFraction(EFrontlineVehicle::RailConvoy,Team);
}
void AFrontlineTank::DeployRailPassengersV75()
{
    if(!Battle||!bRailConvoyV32||bRailUnloadedV75||!IsOperational()||!HasEnteredField())return;
    const int32 Dir=Battle->GetTeamDirection(Team);
    const EFrontlineRole Roles[]={EFrontlineRole::Rifle,EFrontlineRole::Pioneer};
    for(int32 Car=0;Car<2;++Car)
    {
        if(RailSectionMaskV837&(1u<<Car))continue;
        if(RailCarIntegrityV837_[Car?3:1]<=0){RailSectionMaskV837|=1u<<Car;continue;}
        const int32 SquadId=Battle->NextSquadId;
        if(!Battle->SpawnSquad(Team,Roles[Car]))continue;
        auto* Squad=Battle->FindSquad(SquadId);if(!Squad)continue;
        const FVector Door=GetActorTransform().TransformPosition(FVector(Car?-1400.f:1320.f,Dir*235.f,0));
        int32 Destination=INDEX_NONE,Passed=Battle->GetHomeTrenchIndex(Team);
        float Nearest=MAX_flt;
        for(int32 I=0;I<Battle->Trenches.Num();++I)
        {
            const float Ahead=(Battle->Trenches[I].X-Door.X)*Dir;
            if(Ahead>=250.f&&Ahead<Nearest){Nearest=Ahead;Destination=I;}
            if(Ahead<=0.f&&(Battle->Trenches[I].X-Battle->Trenches[Passed].X)*Dir>0.f)Passed=I;
        }
        Squad->Destination=Destination;Squad->Trench=INDEX_NONE;Squad->LastPassedTrench=Passed;
        Squad->bAdvancing=true;Squad->bReinforcement=false;Squad->Slot=Car?2:0;
        for(int32 I=0;I<Squad->Members.Num();++I)if(auto* U=Squad->Members[I].Get())
        {
            // These are the real section members, awaiting one physical exit.
            // They are hidden/non-ticking aboard; each becomes an ordinary
            // advancing infantry actor only after its safe door release.
            U->SetActorHiddenInGame(true);U->SetActorTickEnabled(false);
            U->SetActorEnableCollision(false);U->bFieldEntered=false;U->ReturnToIdle();
        }
        RailSquadIdsV837[Car]=SquadId;RailSectionMaskV837|=1u<<Car;
    }
    FrontlineMobilityV41::InvalidateRoster(Battle);
}
bool AFrontlineTank::RailEgressV837(int32 Car,int32 Member,FVector& Feet)const
{
    if(!Battle||Car<0||Car>1)return false;
    const auto* S=Battle->FindSquad(RailSquadIdsV837[Car]);
    const auto* U=S&&S->Members.IsValidIndex(Member)?S->Members[Member].Get():nullptr;if(!U)return false;
    const int32 Inner=Battle->GetTeamDirection(Team);
    const float DoorX=Car?-1400.f:1320.f;
    const auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld());
    for(float Offset:{0.f,90.f,-90.f,180.f,-180.f})for(float Shoulder:{275.f,365.f,455.f})
    {
        FVector P=GetActorTransform().TransformPosition(FVector(DoorX+Offset,Inner*Shoulder,0));
        if(FMath::Abs(P.X)>=FrontlinePacingV24::CombatHalfLength()-35.f||FMath::Abs(P.Y)>=FrontlineBattlefield::HalfFrontage-50.f)continue;
        P.Z=FrontlineInfantryV45::SurfaceHeight(Battle,P);
        if(P.ContainsNaN()||!FrontlineInfantryV45::AllowsTrenchStep(U,P,P)||BlocksInfantryAtV8218(P,GetActorLocation(),HeadingYawV37))continue;
        bool Clear=true;
        for(const AWarfareFrontlineSoldier* Other:Battle->Units)
            if(IsValid(Other)&&Other!=U&&Other->IsAlive()&&!Other->IsHidden()&&FVector::DistSquared2D(Other->Feet(),P)<FMath::Square(80.f)){Clear=false;break;}
        if(Fleet)for(const AFrontlineTank* Other:Fleet->GetVehicles())
            if(IsValid(Other)&&Other!=this&&Other->BlocksInfantryAtV8218(P,Other->GetActorLocation(),Other->GetActorRotation().Yaw)){Clear=false;break;}
        if(Clear){Feet=P;return true;}
    }
    return false;
}
void AFrontlineTank::UpdateRailDisembarkationV837(float Seconds,bool CanBegin)
{
    if(!bRailConvoyV32||!Battle||!IsOperational())return;
    const bool Unloading=!bRailUnloadedV75&&(CanBegin||RailSectionMaskV837!=0);
    RailDoorPhaseV837=FMath::FInterpConstantTo(RailDoorPhaseV837,Unloading?1.f:0.f,Seconds,1.2f);
    const int32 Inner=Battle->GetTeamDirection(Team);
    for(int32 I=0;I<RailDoorsV837.Num();++I)if(RailDoorsV837[I])
    {
        const float Side=I%2?1.f:-1.f;
        RailDoorsV837[I]->SetRelativeRotation(FRotator(0,Side==Inner?Side*65.f*RailDoorPhaseV837:0.f,0));
    }
    if(!Unloading)return;
    RailDisembarkClockV75+=Seconds;if(RailDisembarkClockV75<1.4f)return;
    DeployRailPassengersV75();
    RailReleaseClockV837+=Seconds;if(RailReleaseClockV837<.28f)return;RailReleaseClockV837=0;
    for(int32 Car=0;Car<2;++Car)
    {
        auto* S=Battle->FindSquad(RailSquadIdsV837[Car]);if(!S)continue;
        const int32 Member=RailReleasedV837[Car];
        if(!S->Members.IsValidIndex(Member))continue;
        auto* U=S->Members[Member].Get();
        if(!U||!U->IsAlive()){++RailReleasedV837[Car];continue;}
        FVector P;if(!RailEgressV837(Car,Member,P))continue;
        U->SetActorLocation(P+FVector(0,0,U->GroundContactOffsetV82));
        U->SetActorRotation(FRotator(0,Inner>0?0.f:180.f,0));U->bFieldEntered=true;
        U->SetActorHiddenInGame(false);U->SetActorEnableCollision(true);U->SetActorTickEnabled(true);
        U->DepartureClock=0;U->bMoving=true;U->SetMotion(U->GetWalkMotion());
        ++RailReleasedV837[Car];FrontlineMobilityV41::InvalidateRoster(Battle);
        if(RailReleasedV837[Car]>=S->Members.Num())
            FrontlineAchievementsV76::RailSectionDeployed(Battle,this,int32(Car?EFrontlineRole::Pioneer:EFrontlineRole::Rifle),S->Id);
    }
    bool Complete=RailSectionMaskV837==3;
    for(int32 Car=0;Car<2;++Car)
        if(const auto* S=Battle->FindSquad(RailSquadIdsV837[Car]))Complete&=RailReleasedV837[Car]>=S->Members.Num();
    bRailUnloadedV75=Complete;
}
bool AFrontlineTank::ConfigureRailV32()
{
    if(bRailConvoyV32)return AreAssetsReady();
    auto* Mesh=FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_RailConvoy"));
    if(!Mesh)Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Vehicles/V75/SM_RailConvoy_V75.SM_RailConvoy_V75"));
    if(!Mesh||!Body||!Battle)return false;bRailConvoyV32=true;
    Type=EFrontlineVehicle::RailConvoy;
    for(UStaticMeshComponent* C:MarkingsV37)if(C)C->DestroyComponent();MarkingsV37.Empty();
    Body->EmptyOverrideMaterials();Body->SetStaticMesh(Mesh);TrackLinks->SetVisibility(false);BodyMaterials.Reset();
    ApplySurfaceV37(Body,.85f,.7f);
    HullIntegrity=MaximumHullIntegrity=UFrontlineVehicleSubsystem::Definition(EFrontlineVehicle::RailConvoy).Hull*(Battle->HasTechnology(22,Team)?1.22f:1.f);
    TrackIntegrity[0]=TrackIntegrity[1]=420;CannonMounts[0]=FVector(2210,0,270);CannonBaseYaw[0]=0;Cannons[0]->SetRelativeLocation(CannonMounts[0]);
    CannonMounts.Add(FVector(-2260,0,270));CannonBaseYaw.Add(180.f);
    auto* RearGun=NewObject<UStaticMeshComponent>(this,TEXT("RearRailCannonV75"));RearGun->SetupAttachment(Body);
    RearGun->SetStaticMesh(Asset<UStaticMesh>(TEXT("SM_SixPounder")));RearGun->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RearGun->SetRelativeLocation(CannonMounts[1]);RearGun->SetRelativeRotation(FRotator(0,180,0));RearGun->RegisterComponent();
    ApplySurfaceV37(RearGun,.65f,.35f);Cannons.Add(RearGun);
    if(auto* RailCannonMesh=FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_RailCannon")))
        for(UStaticMeshComponent* Cannon:Cannons)Cannon->SetStaticMesh(RailCannonMesh);
    auto* RearFlash=NewObject<UStaticMeshComponent>(this,TEXT("RearRailCannonFlashV75"));RearFlash->SetupAttachment(Body);
    RearFlash->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane")));RearFlash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RearFlash->SetCastShadow(false);RearFlash->SetVisibility(false);RearFlash->RegisterComponent();Flashes.Add(RearFlash);FlashAge.Add(10.f);
    auto* RearFlashBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Characters/V13/Materials/M_MuzzleFlash_V13.M_MuzzleFlash_V13"));
    auto* RearFlashMaterial=UMaterialInstanceDynamic::Create(RearFlashBase,this);RearFlash->SetMaterial(0,RearFlashMaterial);FlashMaterials.Add(RearFlashMaterial);
    const FVector Mounts[]={{1320,-150,250},{-1400,-150,250},{1320,150,250},{-1400,150,250},{-2260,-72,250},{2210,72,250}};
    GunBaseYaw={-65.f,-120.f,65.f,120.f,180.f,0.f};
    for(int32 I=0;I<Guns.Num();++I){GunMounts[I]=Mounts[I];Guns[I]->SetRelativeLocation(Mounts[I]);Guns[I]->SetRelativeRotation(FRotator(0,GunBaseYaw[I],0));}
    for(UStaticMeshComponent* Gun:Guns)ApplySurfaceV37(Gun,.52f,.22f);
    for(UStaticMeshComponent* Gun:Cannons)ApplySurfaceV37(Gun,.58f,.26f);
    auto* WheelMesh=FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_RailWheel"));if(!WheelMesh)WheelMesh=Asset<UStaticMesh>(TEXT("SM_RailWheel_V33"));
    auto* RodMesh=FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_RailCouplingRod"));if(!RodMesh)RodMesh=Asset<UStaticMesh>(TEXT("SM_RailCouplingRod_V33"));if(!WheelMesh||!RodMesh)return false;
    auto AddPart=[&](UStaticMesh* Part,const FVector& Location,float Scale)
    {
        auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(Body);C->SetStaticMesh(Part);C->SetRelativeLocation(Location);C->SetRelativeScale3D(FVector(Scale));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(true);C->RegisterComponent();return C;
    };
    for(float X:{20.f,260.f,500.f})for(float Side:{-1.f,1.f})RailWheelsV33.Add(AddPart(WheelMesh,FVector(X,Side*87.5f,69.f),1.08f));
    for(float Side:{-1.f,1.f})RailRodsV33.Add(AddPart(RodMesh,FVector(282.f,Side*102.f,69.f),1.50f));
    if(auto* Bogie=FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_RailBogieWheel")))
    {
        constexpr float CarCentres[]={2210.f,1320.f,-580.f,-1400.f,-2260.f};
        constexpr float AxleSpacing[]={241.4f,323.f,190.4f,333.2f,234.6f};
        for(int32 Car=0;Car<5;++Car)for(float Axle:{-1.f,1.f})for(float Side:{-1.f,1.f})
        {
            auto* Part=AddPart(Bogie,FVector(CarCentres[Car]+Axle*AxleSpacing[Car],Side*81.f,56.f),1.f);
            ApplySurfaceV37(Part,.68f,.24f);RailBogieWheelsV837.Add(Part);
        }
    }
    if(auto* Door=FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_RailDoor")))
        for(int32 Car=0;Car<2;++Car)for(float Side:{-1.f,1.f})
        {
            auto* Part=AddPart(Door,FVector(Car?-1444.f:1276.f,Side*(Car?150.f:154.f),120.f),1.f);
            ApplySurfaceV37(Part,.78f,.28f);RailDoorsV837.Add(Part);
        }
    if(auto* Cupola=FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_RailGunCupola")))
        for(int32 Mount=0;Mount<2;++Mount)
        {
            auto* Part=AddPart(Cupola,CannonMounts[Mount],1.f);Part->SetRelativeRotation(FRotator(0,CannonBaseYaw[Mount],0));
            ApplySurfaceV37(Part,.66f,.22f);RailCupolasV837.Add(Part);
        }
    UE_LOG(LogTemp,Display,TEXT("WF_V75_ARMOURED_RAIL_READY team=%d hull=%.1f length_cm=%.1f bounded_track_y=-1130"),Team,HullIntegrity,FootprintHalfLengthV37()*2.f);return AreAssetsReady();
}

struct FFrontlineArmourV836Review
{
#include "WarfareFrontlineArmourReviewV836.inl"
#include "WarfareFrontlineArmourMobilityReviewV839.inl"
#include "WarfareFrontlineArmourMotionReviewV8311.inl"
};
TSharedRef<FJsonObject> RunFrontlineArmourMotionV8311Review(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineArmourV836Review::RunMotionV8311(B,Check);}
TSharedRef<FJsonObject> RunFrontlineArmourMobilityV839Review(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineArmourV836Review::RunMobilityV839(B,Check);}
TSharedRef<FJsonObject> RunFrontlineArmourV836Review(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineArmourV836Review::Run(B,Check);}
bool PrepareFrontlineArmourVisualV836(AWarfareFrontlineGameMode* B,bool German)
{return FFrontlineArmourV836Review::PrepareVisual(B,German);}

struct FFrontlineRailGameplayV837Review
{
#include "WarfareFrontlineRailGameplayReviewV837.inl"
};
TSharedRef<FJsonObject> RunFrontlineRailGameplayV837Review(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineRailGameplayV837Review::Run(B,Check);}
bool PrepareFrontlineRailVisualV837(AWarfareFrontlineGameMode* B,bool German)
{return FFrontlineRailGameplayV837Review::PrepareVisual(B,German);}
#include "WarfareFrontlineArmourArtReviewV838.inl"
AFrontlineTank* PrepareFrontlineArmourArtV838(AWarfareFrontlineGameMode* B,int32 Index)
{return PrepareArmourArtV838(B,Index);}
#include "WarfareFrontlineMuzzleFlashReviewV839.inl"
#include "WarfareFrontlineArmourMobilityReviewV8448.inl"
#include "WarfareFrontlineArmourMobilityReviewV84411.inl"
#include "WarfareFrontlineMovementReviewV84413.inl"
#include "WarfareFrontlineArmourTrenchReviewV86.inl"
#include "WarfareFrontlineArmourContactReviewV84414.inl"
#include "WarfareFrontlineArmourApproachReviewV85.inl"
#include "WarfareFrontlineArchiveArmourReviewV85.inl"
