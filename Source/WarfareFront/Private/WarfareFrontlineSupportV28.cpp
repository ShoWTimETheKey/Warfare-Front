#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineParticulateV84412.h"
#include "WarfareFrontlineReportV839.h"
#include "WarfareFrontlineAirSupportV8220.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontlineUnlocksV53.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlinePlayerGuideV43.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineSupportV32.h"
#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlineSupportArrivalV30.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineVoiceContext.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "WarfareFrontlineVisualV85.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/BoxComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "WarfareFrontlineMovementV28.h"

namespace
{
bool InsideSupportField(const FVector& P)
{return !P.ContainsNaN() && FMath::Abs(P.X)<=FrontlineBattlefield::SupportHalfLength && FMath::Abs(P.Y)<=FrontlineBattlefield::HalfFrontage;}
FCollisionQueryParams SupportTrace(const AWarfareFrontlineGameMode* B,const AActor* Target=nullptr)
{
    FCollisionQueryParams Q(SCENE_QUERY_STAT(V28SupportSight),true);
    for(const AWarfareFrontlineSoldier* U:B->Units)if(U)Q.AddIgnoredActor(U);
    if(Target)Q.AddIgnoredActor(Target);return Q;
}
bool SupportLineClear(const AWarfareFrontlineGameMode* B,const AActor* Target,const FVector& From,const FVector& To)
{FHitResult Hit;return !B->GetWorld()->LineTraceSingleByChannel(Hit,From,To,ECC_Visibility,SupportTrace(B,Target));}
bool ObserverCanSee(const AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* U,const AActor* Target,const FVector& P)
{
    if(!U || !U->IsActiveCombatant() || FVector::Dist2D(U->Feet(),P)>6000.f*B->EncounterVisibilityScale()
        || B->DirectFireMultiplier(U->Feet(),P)<.9f)return false;
    return SupportLineClear(B,Target,U->Feet()+FVector(0,0,145),P);
}
bool TankObserved(const AWarfareFrontlineGameMode* B,const AFrontlineTank* T,int32 Team)
{
    if(!T || !T->IsOperational() || !T->HasEnteredField() || T->Team==Team)return false;
    for(const AWarfareFrontlineSoldier* U:B->Units)if(U && U->Team==Team && ObserverCanSee(B,U,T,T->AimPoint()))return true;
    return false;
}
FVector AntitankOrigin(const AWarfareFrontlineGameMode* B,int32 Team)
{
    const int32 Dir=B->GetTeamDirection(Team);float Front=B->Trenches[B->GetHomeTrenchIndex(Team)].X;
    for(const auto& Trench:B->Trenches)if(Trench.Owner==Team&&Trench.X*Dir>Front*Dir)Front=Trench.X;
    const float X=FMath::Clamp(Front+Dir*480.f,-FrontlineBattlefield::SupportHalfLength,FrontlineBattlefield::SupportHalfLength),Y=Team?300.f:-300.f;
    return FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+195.f);
}
bool GunCanSee(const AWarfareFrontlineGameMode* B,const AFrontlineTank* T,int32 Team,const FVector* FixedOrigin=nullptr)
{
    const FVector From=FixedOrigin?*FixedOrigin:AntitankOrigin(B,Team);
    return T && B->DirectFireMultiplier(From,T->AimPoint())>=.9f&&SupportLineClear(B,T,From,T->AimPoint());
}
bool AiAreaSafe(const AWarfareFrontlineGameMode* B,EFrontlineSupport S,const FVector& Target,int32 Team)
{
    if(!B->IsFireSupportAreaSafe(Target,S,Team))return false;
    // Wind will carry gas east during its lifetime. Check that future corridor
    // when committing; an already released cloud is never made selectively harmless.
    const FVector Centre=S==EFrontlineSupport::Gas?Target+FVector(300,37.5,0):Target;
    const FVector2D Extent=S==EFrontlineSupport::Gas?FVector2D(1410,880):
        S==EFrontlineSupport::AntiTank?FVector2D(500,500):
        S==EFrontlineSupport::AirStrike?FVector2D(2100,1050):FVector2D(1505,940);
    for(const AWarfareFrontlineSoldier* U:B->Units)if(U && U->Team==Team && U->IsActiveCombatant())
    {const FVector D=U->Feet()-Centre;if(FMath::Abs(D.X)<=Extent.X && FMath::Abs(D.Y)<=Extent.Y)return false;}
    if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const AFrontlineTank* T:V->GetVehicles())
        if(T && T->Team==Team && T->IsOperational() && T->HasEnteredField())
        {const FVector D=T->GetActorLocation()-Centre;if(FMath::Abs(D.X)<=Extent.X+400 && FMath::Abs(D.Y)<=Extent.Y+190)return false;}
    return true;
}
struct FAirPass
{
    int32 Index=0,Direction=1;
    float Age=0,X=0;
    bool Visible=false,Firing=false;
};
FAirPass AirPass(float Age,float TargetX,int32 Forward)
{
    FAirPass P;P.Index=Age<12.f?0:1;P.Age=Age-P.Index*12.f;P.Direction=Forward*(P.Index==0?1:-1);
    P.X=TargetX+P.Direction*(P.Age*2600.f-5000.f);
    P.Visible=Age>=-3.f && Age<=20.f && P.Age<7.f && P.Age>-3.f;
    P.Firing=Age>=0 && Age<20 && P.Age>=1.15f && P.Age<=2.7f;return P;
}
FVector GasReleasePoint(const FVector& Target,int32 Group)
{
    FVector P=Target+FVector((Group%3-1)*220.f+(Group>=3?55.f:0.f),(Group%3-1)*245.f+(Group>=3?-45.f:0.f),0);
    P.X=FMath::Clamp(P.X,-double(FrontlineBattlefield::SupportHalfLength),double(FrontlineBattlefield::SupportHalfLength));
    P.Y=FMath::Clamp(P.Y,-double(FrontlineBattlefield::HalfFrontage),double(FrontlineBattlefield::HalfFrontage));
    P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y);return P;
}
}

const TCHAR* AWarfareFrontlineGameMode::GetSupportName(EFrontlineSupport S,int32 Team) const
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUEquipmentV82::SupportName(this,S,Team);
    const bool German=GetFactionForTeam(Team)==EFrontlineFaction::German;
    switch(S)
    {
        case EFrontlineSupport::CreepingBarrage:return German?TEXT("Feuerwalze 徐进弹幕"):TEXT("步兵协同徐进弹幕");
        case EFrontlineSupport::AirRecon:return German?TEXT("7.7 cm 榴霰弹齐射"):TEXT("18 磅榴霰弹齐射");
        case EFrontlineSupport::AirBombing:return German?TEXT("航空炸弹袭击"):TEXT("低空投弹编队");
        case EFrontlineSupport::Artillery:return German?TEXT("7.7 cm 野炮齐射"):TEXT("18 磅野炮齐射");
        case EFrontlineSupport::Smoke:return TEXT("烟幕弹幕");
        case EFrontlineSupport::Mortar:return German?TEXT("Minenwerfer 壕炮"):TEXT("Stokes 迫击炮");
        case EFrontlineSupport::Gas:return TEXT("毒气弹幕");
        case EFrontlineSupport::AntiTank:return German?TEXT("野炮直射 · 反装甲"):TEXT("18 磅炮 · 反装甲");
        case EFrontlineSupport::AirStrike:return German?TEXT("Halberstadt CL.II 扫射"):TEXT("Bristol F.2B 扫射");
        default:return TEXT("前沿联络");
    }
}
const TCHAR* AWarfareFrontlineGameMode::GetSupportDescription(EFrontlineSupport S,int32 Team) const
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUEquipmentV82::SupportDescription(this,S,Team);
    const TCHAR* Guide=FrontlinePlayerGuideV43::Support(S,GetFactionForTeam(Team));if(*Guide)return Guide;
    switch(S)
    {
        case EFrontlineSupport::CreepingBarrage:return TEXT("炮弹由标定地段逐段向敌后延伸。步兵须紧随烟尘，踏得过早也会撞上自己的炮火。");
        case EFrontlineSupport::AirRecon:return TEXT("Six timed shells burst over a committed patch of open ground. Effective against crowded infantry; trenches and earth shelter them. Cannot damage armour. Fuze drill adds a third three-shell salvo. Keep friendly troops clear.");
        case EFrontlineSupport::AirBombing:return TEXT("Twelve bombs fall in a committed line across the selected ground. The airmen cannot chase a moving target once the run is ordered. Keep your own men clear.");
        case EFrontlineSupport::Artillery:return TEXT("炮位接令后分轮覆盖指定地段。落弹分散，进攻的自己人也必须等炮火停下。");
        case EFrontlineSupport::Smoke:return TEXT("Lay smoke between your men and an enemy gun. Use the screen to cross open ground, change trenches, or deny observers a clear shot at your armour. Both armies lose sight through it; shells still pass through.");
        case EFrontlineSupport::Mortar:return TEXT("壕炮要先传令、装定，再送来数轮落弹。适合打散停留的枪位，不适合追逐奔跑的散兵。");
        case EFrontlineSupport::Gas:return TEXT("气弹在指定地段散落，低处的毒云随风偏移。它会阻断观察、压住枪位，久留有致死危险；风不分敌我。");
        case EFrontlineSupport::AntiTank:return TEXT("A ready field gun answers quickly with up to six aimed rounds against visible armour in the selected sector. It holds fire without an observer or a clear gun line. Tracks may be broken; armour that leaves the sector is not pursued.");
        case EFrontlineSupport::AirStrike:return TEXT("Two low passes rake narrow, overlapping lanes through the selected ground. Best against exposed infantry and advancing columns. Earth cover still protects men, the rounds cannot penetrate heavy armour, and the pilot does not follow targets after the order.");
        default:return TEXT("");
    }
}
int32 AWarfareFrontlineGameMode::SupportUnlockMission(EFrontlineSupport S,int32 Team) const
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUEquipmentV82::FirstMissionForSupport(this,S,Team);
    return FrontlinePlayerGuideV43::SupportArrival(S);
}
bool AWarfareFrontlineGameMode::IsSupportUnlocked(EFrontlineSupport S,int32 Team) const
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUEquipmentV82::SupportAvailable(this,S,Team);
    if((Team!=0 && Team!=1) || uint8(S)>=uint8(EFrontlineSupport::Count) || S==EFrontlineSupport::Rally)return false;
    if(FrontlineUltimateArchiveV8215::FullArmory(this,Team))return true;
    const auto* Mission=FrontlineCampaignV32::CurrentOperation(this);
    if(Mission&&(Mission->DisabledSupportMask&(1u<<uint8(S))))return false;
    const auto* Run=GetCurrentCampaign();if(!Run)return true;
    const int32 Stage=FrontlineUnlocksV53::Stage(this,Team);
    return Stage>=SupportUnlockMission(S,Team);
}
FString AWarfareFrontlineGameMode::SupportUnlockReason(EFrontlineSupport S,int32 Team) const
{
    if(IsSupportUnlocked(S,Team))return FString();
    const auto* Mission=FrontlineCampaignV32::CurrentOperation(this);
    if(uint8(S)<uint8(EFrontlineSupport::Count)&&Mission&&(Mission->DisabledSupportMask&(1u<<uint8(S))))return Mission->RestrictionText;
    const int32 Stage=SupportUnlockMission(S,Team);const auto& Missions=GetCampaignMissions();
    return Missions.IsValidIndex(Stage)?FString::Printf(TEXT("将在「%s」接通这项支援。"),Missions[Stage].Title):TEXT("这项支援尚未配属。");
}
float AWarfareFrontlineGameMode::SupportCooldown(EFrontlineSupport S,int32 Team) const
{
    if(Team!=0 && Team!=1)return MAX_flt;
    switch(S)
    {
        case EFrontlineSupport::CreepingBarrage:case EFrontlineSupport::AirRecon:case EFrontlineSupport::AirBombing:return FrontlineSupportV32::Cooldown(this,S,Team);
        case EFrontlineSupport::Artillery:return ArtilleryCooldown(Team);
        case EFrontlineSupport::Smoke:return SmokeCooldown(Team);
        case EFrontlineSupport::Mortar:return MortarCooldown(Team);
        case EFrontlineSupport::Rally:return RallyCooldown(Team);
        case EFrontlineSupport::Gas:case EFrontlineSupport::AntiTank:case EFrontlineSupport::AirStrike:
            return FMath::Max(0.f,ExtendedSupportReadyAt[Team][int32(S)-int32(EFrontlineSupport::Gas)]-Elapsed);
        default:return MAX_flt;
    }
}
bool AWarfareFrontlineGameMode::RequestSupport(EFrontlineSupport S,const FVector& Target,int32 Team)
{
    if(S>=EFrontlineSupport::CreepingBarrage&&S<=EFrontlineSupport::AirBombing)return FrontlineSupportV32::Request(this,S,Target,Team);
    if(S==EFrontlineSupport::Artillery)return RequestArtillery(Target,Team);
    if(S==EFrontlineSupport::Smoke)return RequestSmoke(Target,Team);
    if(S==EFrontlineSupport::Mortar)return RequestMortar(Target,Team);
    if(S==EFrontlineSupport::Rally)return RequestRally(Target,Team);
    const FString Reason=SupportBlockReason(S,Team);
    if(!Reason.IsEmpty()){if(Team==0)LastMessage=Reason;return false;}
    if(Target.ContainsNaN() || FMath::Abs(Target.X)>FrontlineBattlefield::SupportHalfLength || FMath::Abs(Target.Y)>FrontlineBattlefield::HalfFrontage)return false;
    if(Team==1 && !AiAreaSafe(this,S,Target,Team))return false;
    auto* Mission=GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
    if(!Mission || !Mission->Configure(this,S,Target,Team)){if(Mission)Mission->Destroy();return false;}
    const int32 Index=int32(S)-int32(EFrontlineSupport::Gas);
    const auto* OperationV32=GetEncounterCampaignMission();
    const float RailLiaison=(S==EFrontlineSupport::AntiTank&&HasTechnology(22,Team)&&OperationV32&&OperationV32->bArmoredTrain)?.82f:1.f;
    ExtendedSupportReadyAt[Team][Index]=Elapsed+(WarfareFRRUCampaignV82::Active(this)?WarfareFRRUEquipmentV82::SupportCooldown(this,S,Team):FrontlineGameplayV43::PreparationSeconds(SupportCooldownSeconds(S)*RailLiaison,Team,GetCampaignDifficulty()));++ExtendedSupportCommands[Index];
    FrontlineReportV839::SupportDelivered(this,Team,int32(S));
    if(Team==0)LastMessage=FString::Printf(TEXT("%s · 命令已传往后方"),GetSupportName(S,Team));
    return true;
}

AFrontlineSupportMissionV28::AFrontlineSupportMissionV28()
{
    PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Support mission")));
    Support=EFrontlineSupport::Gas;
}
bool AFrontlineSupportMissionV28::Configure(AWarfareFrontlineGameMode* B,EFrontlineSupport S,const FVector& P,int32 InTeam)
{
    if(!B || (InTeam!=0 && InTeam!=1) || S<EFrontlineSupport::Gas || S>EFrontlineSupport::AirStrike || !InsideSupportField(P))return false;
    // Configure is also called directly by reviews. Apply the same FR/RU
    // airframe exclusion before clocks, RNG or visible aircraft can commit.
    if(S==EFrontlineSupport::AirStrike&&WarfareFRRUCampaignV82::Active(B)
        &&!WarfareFRRUEquipmentV82::SupportAircraftYearAllowedV87(WarfareFRRUCampaignV82::Nation(B,InTeam),S,WarfareFRRUCampaignV82::CalendarDate(B)))return false;
    Battle=B;Support=S;Target=P;Team=InTeam;RequestedAt=B->Elapsed;LastTime=B->Elapsed;Encounter=B->GetEncounterId();
    GunOriginV8215=S==EFrontlineSupport::AntiTank?AntitankOrigin(B,Team):FVector::ZeroVector;
    Random.Initialize(FrontlineDifficultyV30::SupportSeed(B,Team,281917+B->ExtendedSupportCommands[int32(S)-int32(EFrontlineSupport::Gas)]*137+Team*911+int32(S)*73));
    const auto Response=B->FireSupportDelayRange(S,Target,Team);
    Arrival=Random.FRandRange(Response.X,Response.Y)-(Support==EFrontlineSupport::AntiTank?.42f:Support==EFrontlineSupport::AirStrike?1.435f:0.f);
    NextAction=Arrival;StrafeSlotsV8220[0]=StrafeSlotsV8220[1]=0;
    auto Add=[&](UStaticMesh* Mesh,UMaterialInterface* Material)
    {
        auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(Mesh);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetVisibility(false);C->SetCastShadow(S==EFrontlineSupport::AirStrike);C->SetReceivesDecals(false);
        if(Material){auto* MID=UMaterialInstanceDynamic::Create(Material,this);C->SetMaterial(0,MID);Materials.Add(MID);}
        C->RegisterComponent();Visuals.Add(C);
    };
    if(S==EFrontlineSupport::Gas)
    {
        auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
        auto* Mat=FrontlineParticulateV84412::Material(GetWorld(),2);
        if(!Mat)Mat=FrontlineSmokeVolumeV35::GasMaterialV8215();
        if(!Plane || !Mat)return false;
        for(int32 I=0;I<FrontlineSmokeVolumeV35::CloudsPerMission;++I)Add(Plane,Mat);
    }
    else if(S==EFrontlineSupport::AirStrike)
    {
        auto* Plane=FrontlineSupportArtV36::Aircraft(B->GetFactionForTeam(Team)==EFrontlineFaction::German);
        if(!Plane){UE_LOG(LogTemp,Error,TEXT("V36 aircraft asset missing"));return false;}Add(Plane,nullptr);
    }
    if(Support==EFrontlineSupport::Gas)
        for(int32 Group=0;Group<6;++Group)
            ScheduleFrontlineSupportArrivalV30(B,GasReleasePoint(Target,Group),Team,RequestedAt+Arrival+Group*1.7f,false);
    SetActorLocation(Target);return true;
}
void AFrontlineSupportMissionV28::Tick(float DT)
{
    Super::Tick(DT);
    if(!Battle || Encounter!=Battle->GetEncounterId() || Battle->Elapsed<RequestedAt){Destroy();return;}
    if(!Battle->CanResolveCommittedSupportV8217()){Destroy();return;}
    const float Clock=Battle->CommittedSupportTimeV8217();
    if(Clock<=LastTime)return;const float ObservedSeconds=Clock-LastTime;LastTime=Clock;
    const float Time=Clock-RequestedAt;
    if(Support==EFrontlineSupport::Gas)EmitGas(Time);
    else if(Support==EFrontlineSupport::AntiTank)FireAntitank(Time);
    else {Strafe(Time,ObservedSeconds);ResolveStrafeRounds();}
}
void AFrontlineSupportMissionV28::EmitGas(float Time)
{
    if(Time<Arrival)return;
    const float Age=Time-Arrival;
    if(!Warned){Warned=true;WarnFrontlineSoldiers(GetWorld(),EFrontlineVoiceEvent::GasAlarm,Target,2000.f);}
    const auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
    const FVector ViewForward=(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0)).Vector();
    for(int32 Group=0;Group<6;++Group)if(Age>=Group*1.7f && !(GasBurstMask&(1<<Group)))
    {
        GasBurstMask|=uint8(1<<Group);const FVector P=GasReleasePoint(Target,Group);
        FrontlineReportV839::SupportRounds(Battle,Team,int32(Support));
        // A low bursting charge releases gas; it does not call explosive damage
        // as well as the separately resolved exposure below.
        if(auto* Dust=GetWorld()->SpawnActor<AWarfareFrontlineImpact>(P,FRotator::ZeroRotator))Dust->ConfigureBulletStrike();
        if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->PlayImpact(true,P);
    }
    for(int32 I=0;I<Visuals.Num();++I)
    {
        const float Delay=I*1.7f,T=Age-Delay;
        if(T<0)continue;
        const float Spread=FMath::Clamp(T/2.f,0.f,1.f);
        const float Replenish=.10f*FMath::Clamp(1-FMath::Abs(T-5.1f)/2.f,0.f,1.f);
        // Each cube is one persistent released cloud. The retired twelve-card
        // pair layout incorrectly hid every other one after the first 1.9 s.
        const float Opacity=FMath::Clamp(T/1.4f,0.f,1.f)*FMath::Clamp((33-Age)/4.f,0.f,1.f)*(.51f+Replenish*.82f);
        const FVector Pos=GasReleasePoint(Target,I)+FVector((I%3-1)*95.f*Spread+Age*24.f,
            (I>=3?155.f:-155.f)*Spread+Age*3.f,0);
        const float Width=220+950*Spread+FMath::Sqrt(T)*26;
        const bool Layered=Materials[I]&&(Materials[I]->GetBaseMaterial()->GetName()==TEXT("M_GasVolume_V84413")||Materials[I]->GetBaseMaterial()->GetName()==TEXT("M_GasVolume_V85"));
        const FVector Dimensions(Width,Width*(.91f+.035f*(I%3)),Width*((Layered?.79f:.67f)+.035f*(I%3)));
        const FVector Centre(Pos.X,Pos.Y,AWarfareFrontlineGround::HeightAt(Pos.X,Pos.Y)+Dimensions.Z*(Layered?.38f:.35f));
        Materials[I]->SetScalarParameterValue(TEXT("Seed"),I*2.399f+Target.X*.00017f);
        FrontlineParticulateV84412::Update(Visuals[I],Materials[I],Centre,Dimensions,ViewForward,Opacity*(Layered?.87f:1.f),T,2);
        Materials[I]->SetScalarParameterValue(TEXT("VolumeProfile"),1.f);
        Materials[I]->SetVectorParameterValue(TEXT("DustColor"),FLinearColor(.17f,.165f,.085f));
    }
    if(Time>=NextAction && Age<29 && Battle->CanResolveCommittedSupportV8217())
    {
        NextAction=Time+FrontlineGasV35::ExposureIntervalSeconds;++RoundsFired;
        const FVector Centre=Target+FVector(Age*24.f,Age*3.f,0);
        FFrontlineSmokeZone Zone;Zone.Team=Team;Zone.bGas=true;Zone.bCoverCallMade=true;Zone.Centre=Centre;Zone.Radius=950;
        Zone.StartedAt=Battle->CommittedSupportTimeV8217();Zone.ExpiresAt=Zone.StartedAt+3.f;Battle->SmokeZones.Add(Zone);
        // V40: soldier Tick integrates this cloud's exposure continuously.
        // There is no second random death roll per emitter or overlapping round.
    }
    if(Age>33)Destroy();
}
void AFrontlineSupportMissionV28::FireAntitank(float Time)
{
    if(RoundsFired>=6 || Time>Arrival+24.f){Destroy();return;}
    if(Time<NextAction)return;
    NextAction=Time+.5f;
    const FVector Origin=GunOriginV8215;
    // The actual ordered area is retained; no tank-tracking aim correction.
    FVector Aim=Target+FVector(Random.FRandRange(-150,150),Random.FRandRange(-100,100),0);
    if(!InsideSupportField(Aim))return;
    Aim.Z=AWarfareFrontlineGround::HeightAt(Aim.X,Aim.Y)+105.f;
    FHitResult Hit;if(GetWorld()->LineTraceSingleByChannel(Hit,Origin,Aim,ECC_Visibility,SupportTrace(Battle)))Aim=Hit.ImpactPoint;
    if(!InsideSupportField(Aim))return;
    if(Team==1 && !AiAreaSafe(Battle,Support,Aim,Team))return;
    NextAction=Time+3.3f;
    FFrontlineStrike Strike;Strike.SourceV65=FFrontlineKillSourceV65::FireSupport(Team,int32(Support));Strike.Team=Team;Strike.Target=Aim;Strike.Origin=Origin;
    Strike.LaunchedAt=Battle->CommittedSupportTimeV8217();Strike.ImpactAt=Strike.LaunchedAt+.42f;Strike.Radius=300;
    Strike.bDirectFire=true;Strike.bMortar=true;Strike.ArmorPower=260;Strike.bIncomingSoundPlayed=true;Strike.RoundIndex=RoundsFired++;
    Strike.Projectile=FrontlineSupportArtV36::SpawnShell(Battle,Strike);
    Battle->Strikes.Add(Strike);
    if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->PlayTankCannon(Origin);
}
void AFrontlineSupportMissionV28::Strafe(float Time,float ObservationSeconds)
{
    // The aeroplane genuinely crosses the scene twice; no bomb or artillery
    // explosion is substituted for low-calibre aircraft machine-gun rounds.
    const float Age=Time-Arrival;
    if(Age< -3.f)return;
    if(!Warned){Warned=true;WarnFrontlineSoldiers(GetWorld(),EFrontlineVoiceEvent::AircraftInbound,Target,3600.f,1-Team);}
    const FAirPass Pass=AirPass(Age,Target.X,Battle->GetTeamDirection(Team));
    const float PassAge=Pass.Age;const int32 Direction=Pass.Direction;const bool Visible=Pass.Visible;
    auto Flight=FrontlineSupportArtV40::AttackFlight(Target,Direction,PassAge,Pass.Index);
    if(Battle->Phase==EFrontlinePhase::Fighting&&Visible&&!bDrivenOffV59&&FrontlineFieldSitesV59::EngageAircraft(Battle,Team,Flight.Position,GetUniqueID(),ObservationSeconds>0.f?ObservationSeconds:GetWorld()->GetDeltaSeconds()))
    {bDrivenOffV59=true;DrivenOffAtV59=Time;}
    if(bDrivenOffV59)
    {
        const float Escape=FMath::Clamp(Time-DrivenOffAtV59,0.f,2.f);Flight.Position.Z+=Escape*450.f;Flight.Rotation.Roll+=Escape*14.f;
        if(Pass.Index>0&&Visuals.Num())Visuals[0]->SetVisibility(false);
    }
    if(Visuals.Num())
    {
        Visuals[0]->SetVisibility(Visible&&(!bDrivenOffV59||Pass.Index==0));
        Visuals[0]->SetWorldLocation(Flight.Position);
        Visuals[0]->SetWorldRotation(Flight.Rotation);
        if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
            Audio->UpdateAircraftAudio(int32(GetUniqueID()),Visuals[0]->GetComponentLocation(),1.f,Visible&&(!bDrivenOffV59||Pass.Index==0),Flight.Velocity);
    }
    // There are 32 committed .05-game-second opportunities on each pass. Sample
    // the real airframe at the scheduled discharge, not once at the latest
    // rendered pose: .2/.4-second steps must not silently erase most ammunition.
    for(int32 PassIndex=0;PassIndex<2;++PassIndex)
    while(StrafeSlotsV8220[PassIndex]<32)
    {
        const float ShotTime=Arrival+PassIndex*12.f+1.15f+StrafeSlotsV8220[PassIndex]*.05f;
        if(ShotTime>Time+.00001f)break;
        ++StrafeSlotsV8220[PassIndex];
        // A driven-off aircraft retains rounds already discharged earlier in
        // this game step, while all later slots stay cancelled.
        if(bDrivenOffV59&&ShotTime>=DrivenOffAtV59)continue;
        const int32 ShotDirection=Battle->GetTeamDirection(Team)*(PassIndex? -1:1);
        const auto ShotFlight=FrontlineSupportArtV40::AttackFlight(Target,ShotDirection,ShotTime-Arrival-PassIndex*12.f,PassIndex);
        const FTransform Airframe(ShotFlight.Rotation,ShotFlight.Position);
        const FVector Muzzle=Airframe.TransformPosition(FVector(287,-31,49));
        FRotator ShotRotation=ShotFlight.Rotation;ShotRotation.Pitch+=Random.FRandRange(-.40f,.40f);ShotRotation.Yaw+=Random.FRandRange(-7.5f,7.5f);
        FVector Ground;float Duration=0;
        if(!FrontlineSupportArtV40::ForwardGroundShot(Muzzle,ShotRotation.Vector(),Ground,Duration))continue;
        FHitResult Obstacle;
        if(GetWorld()->LineTraceSingleByChannel(Obstacle,Muzzle,Ground,ECC_Visibility,SupportTrace(Battle)))
        {Ground=Obstacle.ImpactPoint;Duration=FMath::Max(.001f,float(FVector::Distance(Muzzle,Ground))/76000.f);}
        // Off-map shots are omitted, never clamped into a lethal pile at the edge.
        if(!InsideSupportField(Ground) || (Team==1 && !AiAreaSafe(Battle,Support,Target,Team)))continue;
        ++RoundsFired;
        FrontlineReportV839::SupportRounds(Battle,Team,int32(Support));
        // Short moving, world-space tracer segments arrive before directional
        // earth strikes. The old whole-path HUD line resembled a laser beam.
        // Arrival owns the ground strike independently of the optional tracer
        // actor. Its 32-actor visual cap must not erase shots into empty earth.
        const float LaunchedAt=RequestedAt+ShotTime;
        FrontlineSupportArtV36::StrafeRoundAtV8220(Battle,Muzzle,Ground,LaunchedAt,LaunchedAt+Duration);
        FStrafeImpactV37 Round;Round.Origin=Muzzle;Round.Point=Ground;Round.ImpactAt=LaunchedAt+Duration;Round.LaunchedAtV8220=LaunchedAt;Round.Seed=int32(Random.GetUnsignedInt());PendingStrafe.Add(Round);
        if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->PlayWeapon(true,Battle->GetFactionForTeam(Team)==EFrontlineFaction::German,Muzzle,90000+Team);
    }
    if(Age>20&&PendingStrafe.IsEmpty())Destroy();
}
void AFrontlineSupportMissionV28::ResolveStrafeRounds()
{
    // Damage follows the committed tracer arrival, even if the visual budget is
    // exhausted. A later order or a moving target cannot alter the stored point.
    // Coarse and fine steps resolve the same chronological contacts. A reversed
    // batch would otherwise change which victims consume each round's rolls.
    PendingStrafe.Sort([](const FStrafeImpactV37& A,const FStrafeImpactV37& B){return A.ImpactAt<B.ImpactAt;});
    for(int32 I=0;I<PendingStrafe.Num();)
    {
        const auto Round=PendingStrafe[I];if(Battle->CommittedSupportTimeV8217()<Round.ImpactAt)break;
        FRandomStream ImpactRandom(Round.Seed);const FVector Ground=Round.Point;
        if(auto* Strike=GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Ground,FRotator::ZeroRotator))
            Strike->ConfigureStrafeStrike((Ground-Round.Origin).GetSafeNormal());
        for(AWarfareFrontlineSoldier* U:Battle->Units)if(U&&U->IsActiveCombatant()&&FMath::Square((U->Feet().X-Ground.X)/250.f)+FMath::Square((U->Feet().Y-Ground.Y)/520.f)<1.f)
        {
            const float BeatenDistance=FMath::Sqrt(FMath::Square((U->Feet().X-Ground.X)/250.f)+FMath::Square((U->Feet().Y-Ground.Y)/520.f));
            const float Falloff=FMath::Lerp(1.f,.45f,BeatenDistance);
            const bool Exposed=SupportLineClear(Battle,U,Round.Origin,U->Feet()+FVector(0,0,125));
            const float Chance=Exposed?FrontlineWeapons::FirearmHitChance((Battle->CoverFor(U)>0?.29f:.61f)*Falloff*Battle->IncomingBulletHitChanceScale(U),true):0.f;
            U->ReceiveHit(ImpactRandom.FRand()<Chance?1.f:0.f,.33f*Battle->IncomingBulletPressureScale(U),Team,Ground-U->Feet(),EFrontlineDamageKind::Bullet,.5f,FFrontlineKillSourceV65::FireSupport(Team,int32(EFrontlineSupport::AirStrike)));
        }
        if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(GetWorld()))
            for(AFrontlineTank* Tank:Vehicles->GetVehicles())if(Tank&&Tank->IsOperational()&&FVector::Dist2D(Ground,Tank->GetActorLocation())<330)
                Vehicles->ApplySmallArmsHit(Tank,Ground+FVector(0,0,110),true,Team);
        PendingStrafe.RemoveAt(I);
    }
}

struct FFrontlineSupportV40Review
{
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check)
    {
        FString UserDir;if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV40Review"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
        const float Clock=B->Elapsed;const auto Phase=B->Phase;const int32 Units=B->Units.Num(),Strikes=B->Strikes.Num();
        TSet<AActor*> Existing;for(TActorIterator<AFrontlineOrdnanceV36> It(B->GetWorld());It;++It)Existing.Add(*It);
        auto* Air=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
        ON_SCOPE_EXIT{if(IsValid(Air))Air->Destroy();for(TActorIterator<AFrontlineOrdnanceV36> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();B->Elapsed=Clock;B->Phase=Phase;};
        B->Phase=EFrontlinePhase::Fighting;
        const bool Ready=Air&&Air->Configure(B,EFrontlineSupport::AirStrike,FVector(0,0,AWarfareFrontlineGround::HeightAt(0,0)),0);
        Check(TEXT("V40 actual air mission retains aircraft asset"),Ready);if(!Ready)return;
        bool Forward=true,Deferred=true,Independent=true;
        for(int32 Pass=0;Pass<2;++Pass)for(int32 I=0;I<=104;++I)
        {
            const float Time=Air->Arrival+Pass*12+1.35f+I/90.f;B->Elapsed=Air->RequestedAt+Time;
            const int32 Prior=Air->RoundsFired;Air->Strafe(Time);
            if(Air->RoundsFired<=Prior)continue;
            const auto& Round=Air->PendingStrafe.Last();const auto* Plane=Air->Visuals.Num()?Air->Visuals[0].Get():nullptr;
            const float Local=Round.LaunchedAtV8220-Air->RequestedAt-Air->Arrival-Pass*12.f;
            const auto Discharge=FrontlineSupportArtV40::AttackFlight(Air->Target,B->GetTeamDirection(Air->Team)*(Pass?-1:1),Local,Pass);
            Forward&=Plane&&Round.Origin.Equals(FTransform(Discharge.Rotation,Discharge.Position).TransformPosition(FVector(287,-31,49)),.05f)&&FVector::DotProduct((Round.Point-Round.Origin).GetSafeNormal(),Discharge.Rotation.Vector())>.98f;
            Deferred&=Round.ImpactAt>Round.LaunchedAtV8220&&Round.ImpactAt-Round.LaunchedAtV8220<.20f;
            const FVector Committed=Round.Point;const int32 Count=Air->PendingStrafe.Num();Air->Strafe(Time);
            Independent&=Count==Air->PendingStrafe.Num()&&Air->PendingStrafe.Last().Point.Equals(Committed,.001f);
        }
        Check(TEXT("V40 actual strafing uses visible muzzle at discharge and forward direction"),Forward&&Air->RoundsFired>=40&&Air->RoundsFired<=64);
        Check(TEXT("V40 actual strafing defers immutable hits and rejects duplicate same-time fire"),Deferred&&Independent&&Air->PendingStrafe.Num()==Air->RoundsFired);
        Check(TEXT("V40 air visual check does not resolve damage or add support strikes"),B->Units.Num()==Units&&B->Strikes.Num()==Strikes);
    }
};
void RunFrontlineSupportChecksV40(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check)
{FFrontlineSupportV40Review::Run(B,Check);}

struct FFrontlineRulesV8215Review
{
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B);
        Check(TEXT("v8215_combat_rules_require_disposable_nosave_profile"),Safe);if(!Safe)return;
        const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldSmoke=B->SmokeZones;const auto OldStrikes=B->Strikes;
        const auto OldPhase=B->Phase;const float OldTime=B->Elapsed,OldOutro=B->BattleOutroElapsed;
        const int32 OldWinner=B->BattleOutroWinner,OldShots=B->ShotsFired;const auto OldMessage=B->LastMessage;
        const auto OldRecords0=B->CombatRecordV65[0],OldRecords1=B->CombatRecordV65[1];
        const int32 OldCas0=B->Casualties[0],OldCas1=B->Casualties[1];
        const float OldDamage0=B->DamageDealt[0],OldDamage1=B->DamageDealt[1],OldMorale0=B->Morale[0],OldMorale1=B->Morale[1],OldFriendly=B->FriendlyFireDamage,OldPeak=B->PeakSuppression;
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        Check(TEXT("v8215_rules_have_actual_vehicle_subsystem"),Fleet!=nullptr);if(!Fleet)return;
        const auto OldVehicles=Fleet->Vehicles;const int32 OldSerial=Fleet->NextSerial;
        TSet<TWeakObjectPtr<AActor>> PriorActors;for(TActorIterator<AActor> I(B->GetWorld());I;++I)PriorActors.Add(*I);
        ON_SCOPE_EXIT
        {
            TArray<AActor*> Added;for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(!PriorActors.Contains(*I))Added.Add(*I);
            for(auto* A:Added)if(IsValid(A))A->Destroy();
            Fleet->Vehicles=OldVehicles;Fleet->NextSerial=OldSerial;
            B->Units=OldUnits;B->Squads=OldSquads;B->SmokeZones=OldSmoke;B->Strikes=OldStrikes;B->Phase=OldPhase;B->Elapsed=OldTime;
            B->BattleOutroElapsed=OldOutro;B->BattleOutroWinner=OldWinner;B->ShotsFired=OldShots;B->LastMessage=OldMessage;
            B->CombatRecordV65[0]=OldRecords0;B->CombatRecordV65[1]=OldRecords1;B->Casualties[0]=OldCas0;B->Casualties[1]=OldCas1;
            B->DamageDealt[0]=OldDamage0;B->DamageDealt[1]=OldDamage1;B->Morale[0]=OldMorale0;B->Morale[1]=OldMorale1;
            B->FriendlyFireDamage=OldFriendly;B->PeakSuppression=OldPeak;
        };
        FrontlineSpecialistsV41::RunChecksV75(B,Check); // Actual deferred infantry/AP paths, state restored by their review.
        B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();B->Strikes.Empty();Fleet->Vehicles.Empty();
        B->Elapsed=100.f;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
        {
            TSet<TWeakObjectPtr<AActor>> ExistingImpacts;for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)ExistingImpacts.Add(*I);
            auto* EmptyStrafe=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
            if(EmptyStrafe)
            {
                EmptyStrafe->Battle=B;EmptyStrafe->Team=0;
                const FVector Ground(0,900,AWarfareFrontlineGround::HeightAt(0,900));
                AFrontlineSupportMissionV28::FStrafeImpactV37 R;R.Origin=Ground+FVector(-850,0,1100);R.Point=Ground;R.ImpactAt=B->Elapsed+.05f;R.Seed=821601;
                EmptyStrafe->PendingStrafe.Add(R);const int32 Before=B->Casualties[0]+B->Casualties[1];
                EmptyStrafe->ResolveStrafeRounds();const bool Deferred=EmptyStrafe->PendingStrafe.Num()==1;
                B->Elapsed+=.051f;EmptyStrafe->ResolveStrafeRounds();bool Strike=false;
                for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)if(!ExistingImpacts.Contains(*I))
                    Strike|=I->GetActorLocation().Equals(Ground,.01f)&&I->GetBlastProfileIndex()==3&&I->GetBallisticFragmentCount()>=9;
                Check(TEXT("v8216_empty_ground_strafe_arrival_has_directional_debris_without_any_tracer_actor_or_enemy"),Deferred&&Strike
                    &&EmptyStrafe->PendingStrafe.IsEmpty()&&B->Casualties[0]+B->Casualties[1]==Before);
            }
        }
        bool SamePace=true;
        for(bool Run:{false,true})for(float Remaining:{25.f,600.f,2400.f})
            SamePace&=FMath::IsNearlyEqual(FrontlineMovementV28::TravelSpeed(Remaining,0.f,Run),FrontlineMovementV28::TravelSpeed(Remaining,1.f,Run));
        Check(TEXT("v8215_pressure_never_multiplies_translation_speed"),SamePace);
        auto Make=[&](int32 Team,EFrontlineRole Role,const FVector& Feet)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            U->Initialize(B,821500+B->Units.Num(),Team,Role,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;
            U->Suppression=0.f;U->SetActorLocation(Feet+FVector(0,0,88));B->Units.Add(U);return U;
        };
        const int32 Dir=B->GetTeamDirection(0);const float Home=B->Trenches[B->GetHomeTrenchIndex(0)].X;
        const float TargetX=Home+Dir*1800.f;
        const FVector Target(TargetX,-300,AWarfareFrontlineGround::HeightAt(TargetX,-300));
        const auto Kind=B->GetFactionForTeam(1)==EFrontlineFaction::German?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale;
        auto* Armour=Fleet->Spawn(Kind,1,true);auto* Observer=Make(0,EFrontlineRole::Rifle,Target-FVector(Dir*650,0,0));
        Check(TEXT("v8215_actual_antitank_fixture_assets"),Armour&&Armour->AreAssetsReady()&&Observer);if(!Armour||!Observer)return;
        Armour->SetActorLocation(Target);Armour->HeadingYawV37=B->GetTeamDirection(1)>0?0.f:180.f;Armour->FitGroundV35(Target,0);Armour->bEntered=true;
        Fleet->Vehicles={Armour};
        Check(TEXT("v8215_antitank_actual_650cm_friendly_observer_does_not_block_fire"),
            B->IsFireSupportAreaSafe(Armour->GetActorLocation(),EFrontlineSupport::AntiTank,0)
            &&AiAreaSafe(B,EFrontlineSupport::AntiTank,Armour->GetActorLocation(),0));
        auto* Gun=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
        const bool Configured=Gun&&Gun->Configure(B,EFrontlineSupport::AntiTank,Armour->GetActorLocation(),0);
        Check(TEXT("v8215_antitank_order_has_fixed_forward_gun_and_live_observer"),Configured&&TankObserved(B,Armour,0)
            &&(Gun->GunOriginV8215.X-Home)*Dir>=450.f&&GunCanSee(B,Armour,0,&Gun->GunOriginV8215));
        if(Configured)
        {
            const float Hull=Armour->HullIntegrity;
            B->Elapsed=Gun->RequestedAt+Gun->Arrival+.01f;Gun->FireAntitank(Gun->Arrival+.01f);
            Check(TEXT("v8215_antitank_support_commits_physical_round_without_instant_damage"),Gun->RoundsFired==1&&B->Strikes.Num()==1
                &&B->Strikes.Last().bDirectFire&&B->Strikes.Last().ImpactAt>B->Elapsed&&B->Strikes.Last().Projectile.IsValid()&&Armour->HullIntegrity==Hull);
            const FVector Fixed=Gun->GunOriginV8215;
            const auto Units=B->Units;B->Units.Empty();Gun->FireAntitank(Gun->Arrival+4.f);B->Units=Units;
            Check(TEXT("v8217_antitank_support_fires_selected_ground_without_observer"),Gun->RoundsFired==2&&Gun->GunOriginV8215.Equals(Fixed));
        }
        // A real mount-ready contact must produce fire while the vehicle holds;
        // a hull-eye contact with an obstructed muzzle must instead approach.
        Observer->SetActorLocation(Target-FVector(Dir*1600.f,0,0)+FVector(0,0,88));
        const int32 MGStart=Armour->MachinegunShots;const FVector MGStartAt=Armour->GetActorLocation();
        for(float& Clock:Armour->GunClock)Clock=0.f;
        for(float& Clock:Armour->CannonClock)Clock=100.f; // Keep the infantry alive for the mounted-MG check.
        bool HadMountedSight=false;FVector MountedPoint;
        for(int32 I=0;I<Armour->GunMounts.Num();++I)
            HadMountedSight|=Armour->InfantryFiringSolutionV828(Observer,I,false,MountedPoint);
        for(int32 I=0;I<80&&Observer->IsAlive()&&Armour->MachinegunShots==MGStart;++I)
        {B->Elapsed+=.1f;Armour->Simulate(.1f);}
        const bool Fired=Armour->MachinegunShots>MGStart;
        Check(TEXT("v8215_tank_infantry_contact_never_halts_without_a_fire_or_approach"),
            Fired||FVector::Dist2D(MGStartAt,Armour->GetActorLocation())>25.f);
        if(HadMountedSight)Check(TEXT("v8215_actual_mounted_mg_fires_when_infantry_sight_is_clear"),Fired);
        Armour->SetActorLocation(Target);Armour->HeadingYawV37=B->GetTeamDirection(1)>0?0.f:180.f;Armour->FitGroundV35(Target,0);
        Armour->HullIntegrity=350.f;Armour->TrackIntegrity[0]=Armour->TrackIntegrity[1]=180.f;
        const FVector Contact=Armour->GetActorTransform().TransformPosition(FVector(0,-190,35));
        Fleet->ApplyGrenadeBlast(Contact,500.f,0,FFrontlineKillSourceV65::Soldier(Observer));
        Check(TEXT("v8215_grenade_contact_damages_track_and_small_hull_amount"),FMath::IsNearlyEqual(Armour->HullIntegrity,338.f,.01f)
            &&FMath::IsNearlyEqual(Armour->TrackIntegrity[0],138.f,.01f)&&Armour->TrackIntegrity[1]==180.f);
        Armour->HullIntegrity=350.f;Armour->TrackIntegrity[0]=Armour->TrackIntegrity[1]=180.f;
        Fleet->ApplyBlast(Contact,900.f,440.f,0,FFrontlineKillSourceV65::Soldier(Observer));
        Check(TEXT("v8215_he_shell_transfers_substantial_contact_hull_damage"),FMath::IsNearlyEqual(Armour->HullIntegrity,350.f-440.f*.44f,.1f)&&Armour->IsOperational());
        Armour->SetActorEnableCollision(false);Fleet->Vehicles.Empty();B->Units.Empty();
        FVector Centre((Home+B->Trenches[B->GetHomeTrenchIndex(0)+Dir].X)*.5f,750,0);
        Centre.Z=AWarfareFrontlineGround::HeightAt(Centre.X,Centre.Y);
        auto* Victim=Make(1,EFrontlineRole::Rifle,Centre+FVector(0,400,0));
        if(Victim)
        {
            auto* Air=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
            Air->Battle=B;Air->Team=0;Air->Support=EFrontlineSupport::AirStrike;
            int32 Seed=1;while(Seed<10000&&FRandomStream(Seed).FRand()>=.005f)++Seed;
            AFrontlineSupportMissionV28::FStrafeImpactV37 Shot;Shot.Origin=Centre+FVector(-Dir*1500,0,1700);Shot.Point=Centre;
            Shot.ImpactAt=B->Elapsed;Shot.Seed=Seed;Air->PendingStrafe.Add(Shot);Air->ResolveStrafeRounds();
            Check(TEXT("v8215_strafe_lateral_beaten_strip_resolves_actual_deferred_infantry_hit"),!Victim->IsAlive()&&Air->PendingStrafe.IsEmpty());
        }
    }
};
void RunFrontlineSupportRulesChecksV8215(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineRulesV8215Review::Run(B,Check);}

void TickFrontlineExtendedSupportAI(AWarfareFrontlineGameMode* B)
{
    if(B&&WarfareFRRUCampaignV82::Active(B))return;
    if(!B || B->Phase!=EFrontlinePhase::Fighting)return;
    // Experienced UK/DE companies use the shared observed fire plan in
    // ThinkEnemy. This legacy chooser must not spend its pending ammunition.
    if(B->GetCampaignDifficulty()!=EFrontlineDifficulty::Rookie)return;
    const auto& Doctrine=FrontlineDifficultyV30::Doctrine(B->GetCampaignDifficulty());
    auto& Tactics=FrontlineDifficultyV30::State(B);
    if(B->Elapsed<B->ExtendedSupportThinkAt)return;
    B->ExtendedSupportThinkAt=B->Elapsed+Doctrine.ExtendedAssessment;
    if(B->Elapsed-Tactics.LastSupportAt<Doctrine.FireCoordinationGap)return;
    TArray<AWarfareFrontlineSoldier*> Seen;
    for(AWarfareFrontlineSoldier* Enemy:B->Units)if(Enemy && FrontlineDifficultyV30::CanObserve(B,Enemy))Seen.Add(Enemy);
    FrontlineDifficultyV30::Observe(B,Seen);
    // A field gun may engage only armour an actual observer can currently see.
    if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
        for(AFrontlineTank* T:Vehicles->GetVehicles())if(TankObserved(B,T,1) && GunCanSee(B,T,1)
            && B->SupportBlockReason(EFrontlineSupport::AntiTank,1).IsEmpty())
        {
            if(B->RequestSupport(EFrontlineSupport::AntiTank,T->GetActorLocation(),1))
            {Tactics.LastSupportAt=B->Elapsed;++Tactics.PlannedFireMissions;return;}
        }
    float BestScore=Doctrine.MinimumTargetValue+1.f;FVector BestPoint=FVector::ZeroVector;
    EFrontlineSupport BestType=EFrontlineSupport::Gas;bool Found=false;
    for(const auto& Candidate:Tactics.Contacts)
    {
        // Slow gas is reserved for a held position. Aircraft sweep a visible
        // advancing concentration. Neither follows a live unit after the order.
        const EFrontlineSupport Type=Candidate.Moving?EFrontlineSupport::AirStrike:EFrontlineSupport::Gas;
        if(!B->SupportBlockReason(Type,1).IsEmpty() || !AiAreaSafe(B,Type,Candidate.Position,1))continue;
        float Score=FrontlineDifficultyV30::TargetScore(Candidate,Tactics.Contacts,B->Elapsed,Doctrine);
        if(Type==EFrontlineSupport::Gas)Score*=Candidate.Entrenched?1.25f:.65f;
        else
        {
            Score=0;
            for(const auto& Contact:Tactics.Contacts)
                if(Contact.Moving && FMath::Abs(Contact.Position.X-Candidate.Position.X)<1300 && FMath::Abs(Contact.Position.Y-Candidate.Position.Y)<460)
                    Score+=FrontlineDifficultyV30::Confidence(Contact,B->Elapsed,Doctrine);
        }
        if(Score>BestScore){BestScore=Score;BestPoint=Candidate.Position;BestType=Type;Found=true;}
    }
    if(Found && B->RequestSupport(BestType,BestPoint,1))
    {Tactics.LastSupportAt=B->Elapsed;++Tactics.PlannedFireMissions;}
}

void RunFrontlineSupportChecksV37(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check)
{
    FString UserDir;if(!B||!FParse::Param(FCommandLine::Get(),TEXT("WFV37Review"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty()){Check(TEXT("v37_support_mission_fixture_requires_isolation"),false);return;}
    const auto OldUnits=B->Units;const auto OldZones=B->SmokeZones;const float OldTime=B->Elapsed;const auto OldPhase=B->Phase;
    TSet<AActor*> Existing;for(TActorIterator<AFrontlineSupportArrivalV30> It(B->GetWorld());It;++It)Existing.Add(*It);
    for(TActorIterator<AFrontlineOrdnanceV36> It(B->GetWorld());It;++It)Existing.Add(*It);
    for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)Existing.Add(*It);
    TArray<AActor*> Owned;B->Units.Empty();B->SmokeZones.Empty();B->Phase=EFrontlinePhase::Fighting;
    ON_SCOPE_EXIT
    {
        B->Units=OldUnits;B->SmokeZones=OldZones;B->Elapsed=OldTime;B->Phase=OldPhase;
        for(auto* A:Owned)if(IsValid(A))A->Destroy();
        for(TActorIterator<AFrontlineSupportArrivalV30> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
        for(TActorIterator<AFrontlineOrdnanceV36> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
        for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
    };
    auto Make=[&](EFrontlineSupport Type)
    {
        auto* M=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();Owned.Add(M);
        if(M&&!M->Configure(B,Type,FVector::ZeroVector,0)){M->Destroy();return static_cast<AFrontlineSupportMissionV28*>(nullptr);}return M;
    };
    auto* Gas=Make(EFrontlineSupport::Gas);auto* Gun=Make(EFrontlineSupport::AntiTank);auto* Air=Make(EFrontlineSupport::AirStrike);
    Check(TEXT("revised_support_missions_bind_existing_accepted_assets"),Gas&&Gun&&Air);
    if(Gas)
    {
        Gas->EmitGas(Gas->Arrival);Gas->EmitGas(Gas->Arrival+8.51f);const int32 Before=Gas->RoundsFired;
        Gas->EmitGas(Gas->Arrival+8.51f);
        Check(TEXT("gas_has_six_real_releases_without_duplicate_exposure"),Gas->GasBurstMask==63&&Gas->RoundsFired==Before&&Gas->Visuals.Num()==FrontlineSmokeVolumeV35::CloudsPerMission);
    }
    if(Gun)
    {
        Check(TEXT("antitank_first_ballistic_arrival_matches_fast_response"),Gun->Arrival+.42f>=5&&Gun->Arrival+.42f<=7);
        Gun->FireAntitank(Gun->Arrival);Check(TEXT("v8217_rapid_antitank_ground_order_does_not_require_armour"),Gun->RoundsFired==1);
        Gun->RoundsFired=6;Gun->FireAntitank(Gun->Arrival+19);Check(TEXT("antitank_ammunition_is_bounded_at_six_rounds"),Gun->IsActorBeingDestroyed());
    }
    if(Air)
    {
        bool WithinLane=true,Deferred=true;
        for(int32 Pass=0;Pass<2;++Pass)for(int32 Frame=0;Frame<=103;++Frame)
        {
            const float Time=Air->Arrival+Pass*12.f+1.35f+Frame/90.f;B->Elapsed=Air->RequestedAt+Time;
            const int32 Before=Air->RoundsFired;Air->Strafe(Time);
            if(Air->RoundsFired>Before)
            {
                const auto& Round=Air->PendingStrafe.Last();WithinLane&=FMath::Abs(Round.Point.X)<1800&&FMath::Abs(Round.Point.Y)<550;
                Deferred&=Round.ImpactAt>Round.LaunchedAtV8220&&Round.ImpactAt-Round.LaunchedAtV8220<.2f;
                const int32 After=Air->RoundsFired;Air->Strafe(Time);Deferred&=Air->RoundsFired==After;
            }
        }
        Check(TEXT("two_strafe_passes_deliver_more_rounds_in_coherent_narrow_lanes"),WithinLane&&Air->RoundsFired>=40&&Air->RoundsFired<=64);
        Check(TEXT("strafe_rounds_defer_damage_until_committed_tracer_arrival"),Deferred&&Air->PendingStrafe.Num()==Air->RoundsFired);
        // No terrain/armour fixture receives test damage. The queue's due-time
        // branch is exercised with one deliberately empty off-field impact.
        Air->PendingStrafe.Reset();AFrontlineSupportMissionV28::FStrafeImpactV37 Empty;
        Empty.Point=FVector(1.e7,1.e7,0);Empty.ImpactAt=B->Elapsed+.085f;Air->PendingStrafe.Add(Empty);
        Air->ResolveStrafeRounds();const bool NotEarly=Air->PendingStrafe.Num()==1;B->Elapsed+=.086f;Air->ResolveStrafeRounds();
        Check(TEXT("pending_strafe_queue_resolves_once_at_arrival"),NotEarly&&Air->PendingStrafe.IsEmpty());
    }
}

TSharedRef<FJsonObject> RunFrontlineV28SupportChecks(AWarfareFrontlineGameMode* B)
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
    auto Check=[&](const TCHAR* Name,bool Passed){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),Name);R->SetBoolField(TEXT("passed"),Passed);Rows.Add(MakeShared<FJsonValueObject>(R));};
    ON_SCOPE_EXIT{int32 Passed=0;for(const auto& R:Rows)Passed+=R->AsObject()->GetBoolField(TEXT("passed"))?1:0;Report->SetArrayField(TEXT("checks"),Rows);Report->SetNumberField(TEXT("passed"),Passed);Report->SetNumberField(TEXT("total"),Rows.Num());};
    FString UserDir;
    if(!B || !FParse::Param(FCommandLine::Get(),TEXT("WFV28Review")) || !FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) || UserDir.IsEmpty())
    {Check(TEXT("isolated_support_review_required"),false);return Report;}
    const auto A=AirPass(1.9f,0,1),C=AirPass(13.9f,0,1),D=AirPass(1.9f,0,-1);
    Check(TEXT("two_air_passes_fire_in_opposite_directions"),A.Firing&&C.Firing&&A.Direction==-C.Direction&&A.Index==0&&C.Index==1&&D.Direction==-A.Direction);
    Check(TEXT("aircraft_turnaround_and_end_are_not_extra_passes"),!AirPass(8,0,1).Firing&&!AirPass(21,0,1).Visible&&!AirPass(26,0,1).Firing);
    Check(TEXT("off_map_rounds_rejected_without_boundary_piling"),InsideSupportField(FVector(FrontlineBattlefield::SupportHalfLength,650,0))&&!InsideSupportField(FVector(FrontlineBattlefield::SupportHalfLength+1,0,0))&&!InsideSupportField(FVector(0,651,0)));
    bool GasInside=true;for(int32 I=0;I<3;++I)GasInside&=InsideSupportField(GasReleasePoint(FVector(FrontlineBattlefield::SupportHalfLength,650,0),I));
    Check(TEXT("gas_release_points_remain_inside_battlefield"),GasInside);

    // Synchronous opt-in fixture: no world ticks occur while units are excluded
    // from the damage loop. No health, campaign state or actor transform is changed.
    const auto SavedUnits=B->Units;const auto SavedSmoke=B->SmokeZones;const auto SavedTracers=B->Tracers;const auto SavedStrikes=B->Strikes;
    const EFrontlinePhase SavedPhase=B->Phase;B->Units.Empty();B->Phase=EFrontlinePhase::Fighting;
    TArray<AActor*> Owned;TSet<AActor*> ExistingImpacts;
    for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)ExistingImpacts.Add(*It);
    ON_SCOPE_EXIT{
        B->Units=SavedUnits;B->SmokeZones=SavedSmoke;B->Tracers=SavedTracers;B->Strikes=SavedStrikes;B->Phase=SavedPhase;
        for(AActor* Actor:Owned)if(IsValid(Actor))Actor->Destroy();
        for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)if(!ExistingImpacts.Contains(*It))It->Destroy();
    };
    AActor* Blocker=B->GetWorld()->SpawnActor<AActor>();Owned.Add(Blocker);
    if(Blocker)
    {
        auto* Box=NewObject<UBoxComponent>(Blocker);Blocker->SetRootComponent(Box);Box->SetBoxExtent(FVector(120));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        Box->RegisterComponent();Blocker->SetActorLocation(FVector(0,0,10000));
        const bool Blocked=!SupportLineClear(B,nullptr,FVector(-500,0,10000),FVector(500,0,10000));
        Blocker->SetActorLocation(FVector(0,1000,10000));
        Check(TEXT("actual_visibility_trace_blocks_and_clears_with_geometry"),Blocked&&SupportLineClear(B,nullptr,FVector(-500,0,10000),FVector(500,0,10000)));
    }
    else Check(TEXT("actual_visibility_trace_blocks_and_clears_with_geometry"),false);
    auto Make=[&](EFrontlineSupport S){auto* M=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();Owned.Add(M);if(M&&!M->Configure(B,S,FVector::ZeroVector,0)){M->Destroy();return static_cast<AFrontlineSupportMissionV28*>(nullptr);}return M;};
    auto* Gas=Make(EFrontlineSupport::Gas);auto* Air=Make(EFrontlineSupport::AirStrike);auto* Gun=Make(EFrontlineSupport::AntiTank);
    Check(TEXT("gas_and_air_assets_configure"),Gas&&Air&&Gun);
    if(Gas)
    {
        Gas->EmitGas(Gas->Arrival);Gas->EmitGas(Gas->Arrival);Gas->EmitGas(Gas->Arrival+1.71f);Gas->EmitGas(Gas->Arrival+3.41f);
        const int32 Pulses=Gas->RoundsFired,Zones=B->SmokeZones.Num();Gas->EmitGas(Gas->Arrival+3.41f);
        Check(TEXT("three_gas_releases_and_no_duplicate_exposure_on_reentry"),Gas->GasBurstMask==7&&Pulses==2&&Gas->RoundsFired==Pulses&&B->SmokeZones.Num()==Zones);
    }
    if(Air)
    {
        Air->Strafe(Air->Arrival+1.9f);Air->Strafe(Air->Arrival+1.9f);const int32 First=Air->RoundsFired;
        Air->Strafe(Air->Arrival+13.9f);
        Check(TEXT("one_actual_round_per_due_time_in_both_air_passes"),First==1&&Air->RoundsFired==2);
        const int32 Shots=Air->RoundsFired;Air->LastTime=B->Elapsed;Air->Tick(1.f);
        Check(TEXT("unchanged_battle_clock_never_advances_support"),Air->RoundsFired==Shots);
    }
    if(Gun)
    {
        const int32 Before=B->Strikes.Num();Gun->FireAntitank(Gun->Arrival);
        Check(TEXT("v8217_no_observer_keeps_committed_antitank_ground_round"),Gun->RoundsFired==1&&B->Strikes.Num()==Before+1);
        Gun->FireAntitank(Gun->Arrival+25);
        Check(TEXT("unobserved_antitank_mission_expires_bounded"),Gun->IsActorBeingDestroyed());
    }
    if(Gas){Gas->Encounter=FGuid::NewGuid();Gas->Tick(0.f);Check(TEXT("old_encounter_mission_is_destroyed"),Gas->IsActorBeingDestroyed());}
    {
        bool FiredPendingShell=false;
        auto* VehicleSystem=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        AWarfareFrontlineSoldier* Observer=nullptr;
        for(AWarfareFrontlineSoldier* U:SavedUnits)if(U && U->Team==0 && U->IsActiveCombatant()){Observer=U;break;}
        if(VehicleSystem && Observer)
        {
            const auto SavedVehicles=VehicleSystem->Vehicles;const int32 SavedSerial=VehicleSystem->NextSerial;
            const FTransform ObserverTransform=Observer->GetActorTransform();const float BeforeClock=B->Elapsed;
            const auto BeforeZones=B->SmokeZones;const auto BeforeUnits=B->Units;
            const EFrontlineVehicle Kind=B->GetFactionForTeam(1)==EFrontlineFaction::German?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale;
            AFrontlineTank* Armour=VehicleSystem->Spawn(Kind,1,true);
            ON_SCOPE_EXIT{
                if(IsValid(Armour))Armour->Destroy();VehicleSystem->Vehicles=SavedVehicles;VehicleSystem->NextSerial=SavedSerial;
                Observer->SetActorTransform(ObserverTransform);B->Elapsed=BeforeClock;B->SmokeZones=BeforeZones;B->Units=BeforeUnits;
            };
            if(Armour)
            {
                VehicleSystem->Vehicles={Armour};B->SmokeZones.Empty();
                Armour->SetActorLocation(FVector(0,0,AWarfareFrontlineGround::HeightAt(0,0)));Armour->Simulate(.001f);
                const float ObserverX=-B->GetTeamDirection(0)*800.f;
                Observer->SetActorLocation(FVector(ObserverX,-250,AWarfareFrontlineGround::HeightAt(ObserverX,-250)+88));B->Units={Observer};
                auto* Positive=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();Owned.Add(Positive);
                const bool Configured=Positive && Positive->Configure(B,EFrontlineSupport::AntiTank,Armour->GetActorLocation(),0);
                const bool Observed=TankObserved(B,Armour,0),GunLine=GunCanSee(B,Armour,0);
                const float Hull=Armour->GetHullIntegrity(),Health=Observer->Health;const int32 BeforeCount=B->Strikes.Num();
                if(Configured){B->Elapsed=Positive->RequestedAt+Positive->Arrival+.01f;Positive->Tick(.01f);}
                if(B->Strikes.Num()==BeforeCount+1)
                {
                    const FFrontlineStrike& S=B->Strikes.Last();
                    if(S.Projectile.IsValid())Owned.Add(S.Projectile.Get());
                    const auto* Visual=S.Projectile.IsValid()?S.Projectile->FindComponentByClass<UStaticMeshComponent>():nullptr;
                    FiredPendingShell=Configured&&Observed&&GunLine&&Positive->RoundsFired==1&&S.bDirectFire&&!S.bResolved
                        &&S.ImpactAt>B->Elapsed&&Visual&&Visual->GetStaticMesh()&&Visual->IsVisible()
                        &&Armour->GetHullIntegrity()==Hull&&Observer->Health==Health;
                }
                auto Detail=MakeShared<FJsonObject>();const FVector From=AntitankOrigin(B,0),To=Armour->AimPoint();float MinClearance=MAX_flt;
                for(int32 I=0;I<=200;++I){const FVector P=FMath::Lerp(From,To,I/200.f);MinClearance=FMath::Min(MinClearance,float(P.Z-AWarfareFrontlineGround::HeightAt(P.X,P.Y)));}
                Detail->SetBoolField(TEXT("observer_has_actual_sight"),Observed);Detail->SetBoolField(TEXT("gun_has_actual_geometry_sight"),GunLine);
                Detail->SetNumberField(TEXT("gun_origin_z_cm"),From.Z);Detail->SetNumberField(TEXT("sampled_terrain_clearance_cm"),MinClearance);
                Report->SetObjectField(TEXT("antitank_positive"),Detail);
            }
        }
        Check(TEXT("observed_armour_produces_visible_direct_shell_without_instant_damage"),FiredPendingShell);
    }
    Report->SetStringField(TEXT("scope"),TEXT("Synchronous isolated production trajectories, geometry trace and event reentry; normal field visuals and observed armour engagement are separate."));
    return Report;
}

#include "WarfareFrontlineAirSupportChecksV8220.inl"
#include "WarfareFrontlineSupportReviewV836.inl"
