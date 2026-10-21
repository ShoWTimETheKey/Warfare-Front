#include "WarfareFrontline.h"
#include "WarfareFrontlineReportV839.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineBalanceV50.h"
#include "WarfareFrontlineGameplayV51.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontlineSupportV32.h"
#include "WarfareFrontlineSupportArrivalV30.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SceneInterface.h"

namespace
{
bool IsFrontlineTeam(int32 Team) { return Team==0 || Team==1; }
bool ValidSupportPoint(const FVector& Point)
{
    return !Point.ContainsNaN() && FMath::Abs(Point.X)<=FrontlineBattlefield::SupportHalfLength && FMath::Abs(Point.Y)<=FrontlineBattlefield::HalfFrontage;
}
}

AWarfareFrontlineSmoke::AWarfareFrontlineSmoke()
{
    PrimaryActorTick.bCanEverTick=true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Smoke")));
    for (int32 I=0;I<FrontlineSmokeVolumeV35::CloudsPerMission;++I)
    {
        UStaticMeshComponent* Puff=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Smoke%d"),I));
        Puff->SetupAttachment(RootComponent); Puff->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Puff->SetCastShadow(false); Puff->SetReceivesDecals(false); Puff->SetVisibility(false); Puffs.Add(Puff);
    }
}
void AWarfareFrontlineSmoke::BeginPlay()
{
    Super::BeginPlay();
    UStaticMesh* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* SmokeMaterials[2]={
        FrontlineSmokeVolumeV35::MaterialV40(),FrontlineSmokeVolumeV35::MaterialV40()};
    if (!Plane || !SmokeMaterials[0] || !SmokeMaterials[1])
    {
        UE_LOG(LogTemp,Error,TEXT("Frontline smoke visual assets missing; smoke request cannot commit.")); Destroy(); return;
    }
    UMaterialInterface* Material=SmokeMaterials[0];
    if (Material->GetBlendMode()!=BLEND_Translucent || SmokeMaterials[1]->GetBlendMode()!=BLEND_Translucent)
    {
        UE_LOG(LogTemp,Error,TEXT("Frontline smoke requires translucent dust material, got blend %d: %s"),int32(Material->GetBlendMode()),*Material->GetPathName());
        Destroy(); return;
    }
    for (int32 I=0;I<Puffs.Num();++I)
    {
        Puffs[I]->SetStaticMesh(Plane);
        Material=SmokeMaterials[I%2];
        UMaterialInstanceDynamic* M=UMaterialInstanceDynamic::Create(Material,this);
        M->SetScalarParameterValue(TEXT("Seed"),I*2.399f+GetActorLocation().X*.00017f);
        M->SetScalarParameterValue(TEXT("Density"),0);
        M->SetVectorParameterValue(TEXT("DustColor"),FLinearColor(.135f,.145f,.150f));
        Puffs[I]->SetMaterial(0,M); Materials.Add(M);
        if (Puffs[I]->GetMaterial(0)!=M || M->GetBlendMode()!=BLEND_Translucent)
        {
            UE_LOG(LogTemp,Error,TEXT("Frontline smoke material binding failed on puff %d."),I); Destroy(); return;
        }
        if (I==0)
        {
            const FLinearColor BoundColor=M->K2_GetVectorParameterValue(TEXT("DustColor"));
            const FMaterialResource* Resource=GetWorld()->Scene?Material->GetMaterial()->GetMaterialResource(GetWorld()->Scene->GetShaderPlatform()):nullptr;
            UE_LOG(LogTemp,Display,TEXT("Frontline smoke material bound: %s; parent %s; blend %d; plane slots %d; density %.3f; color %.3f %.3f %.3f; shader valid %d complete %d"),
                *Puffs[I]->GetMaterial(0)->GetPathName(),*Material->GetPathName(),int32(M->GetBlendMode()),Plane->GetStaticMaterials().Num(),
                M->K2_GetScalarParameterValue(TEXT("Density")),BoundColor.R,BoundColor.G,BoundColor.B,
                Resource && Resource->HasValidGameThreadShaderMap(),Resource && Resource->IsGameThreadShaderMapComplete());
        }
    }
    bVisualReady=true; SetLifeSpan(FrontlineFireSupportV26::SmokeDuration);
}
void AWarfareFrontlineSmoke::Tick(float DT)
{
    Super::Tick(DT);
    const auto* Battle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    Age=Battle&&Battle->Phase==EFrontlinePhase::Fighting?Battle->Elapsed-DeployAt:Age+DT;
    if(Age<0)return;
    const float Fade=FMath::Clamp(Age/.7f,0.f,1.f)*FMath::Clamp((FrontlineFireSupportV26::SmokeDuration-Age)/2.8f,0.f,1.f);
    const APlayerCameraManager* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
    const FVector ViewForward=(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0)).Vector();
    for (int32 I=0;I<Puffs.Num() && I<Materials.Num();++I)
    {
        // Three persistent rolls, each with a brief paired release puff. Six
        // long-lived, mostly coincident integrals washed out their own shade.
        const float Seed=I*2.399f+GetActorLocation().X*.00017f;
        const float Delay=(I/2)*FrontlineFireSupportV26::SmokeBurstSpacing+(I%2)*.15f,T=FMath::Max(0.f,Age-Delay);
        const float Spread=FMath::Clamp(T/1.5f,0.f,1.f);
        const bool ReleasePuff=(I%2)!=0;
        const float ReleaseFade=ReleasePuff?FMath::Clamp((1.85f-T)/.65f,0.f,1.f):1.f;
        if(ReleaseFade<=0){Puffs[I]->SetVisibility(false);continue;}
        const float Width=ReleasePuff?180.f+210.f*FMath::Clamp(T/.6f,0.f,1.f):
            (200.f+810.f*Spread+FMath::Sqrt(T)*25.f)*(1+.07f*FMath::Sin(Seed));
        const FVector Dimensions=ReleasePuff?FVector(Width,Width*.8f,Width*.72f):
            FVector(Width,Width*(.92f+.035f*(I%3)),Width*(.86f+.07f*(I%3)));
        FVector Release=GetActorLocation()+FVector((I/2-1)*470.f+(I%2?35.f:-35.f)*Spread,
            ((I/2)%2?-140.f:140.f)+(I%2?90.f:-90.f)*Spread,0);
        Release.X=FrontlineFireSupportV26::ReflectInside(Release.X,FrontlineBattlefield::SupportHalfLength);
        Release.Y=FrontlineFireSupportV26::ReflectInside(Release.Y,FrontlineBattlefield::HalfFrontage);
        Release.Z=AWarfareFrontlineGround::HeightAt(Release.X,Release.Y);
        Release+=FVector(FMath::Sin(Seed)*T*2.f,FMath::Cos(Seed)*T*1.4f,Dimensions.Z*(.40f+.025f*FMath::Sin(Seed)));
        const float Replenish=FMath::Clamp(1-FMath::Abs(T-5.4f)/2.f,0.f,1.f)*.10f;
        const float Density=Fade*FMath::Clamp(T/.7f,0.f,1.f)*(I%2?.82f:.72f)*(1+Replenish)*ReleaseFade;
        FrontlineSmokeVolumeV35::Update(Puffs[I],Materials[I],Release,Dimensions,ViewForward,Density,T);
    }
}

float AWarfareFrontlineGameMode::RoleCost(EFrontlineRole InRole)
{
    return 0.f;
}
float AWarfareFrontlineGameMode::SupportCost(EFrontlineSupport Support)
{
    return 0.f;
}
float AWarfareFrontlineGameMode::SupportCooldownSeconds(EFrontlineSupport Support)
{
    switch (Support)
    {
        case EFrontlineSupport::Artillery:return 4000.f/35.f;
        case EFrontlineSupport::Smoke:return 30.f;
        case EFrontlineSupport::Mortar:return 45.f;
        case EFrontlineSupport::Rally:return 20.f;
        case EFrontlineSupport::Gas:return 110.f;
        case EFrontlineSupport::AntiTank:return 71.25f;
        case EFrontlineSupport::AirStrike:return 130.f;
        case EFrontlineSupport::CreepingBarrage:return 155.f;
        case EFrontlineSupport::AirRecon:return 88.f;
        case EFrontlineSupport::AirBombing:return 175.f;
        default:return MAX_flt;
    }
}
float AWarfareFrontlineGameMode::Income(int32 Team) const
{
    return 0.f;
}
float AWarfareFrontlineGameMode::CommandIncome(int32 Team) const
{
    return 0.f;
}
float AWarfareFrontlineGameMode::SmokeCooldown(int32 Team) const
{ return IsFrontlineTeam(Team)?FMath::Max(0.f,SmokeReadyAt[Team]-Elapsed):MAX_flt; }
float AWarfareFrontlineGameMode::RallyCooldown(int32 Team) const
{ return IsFrontlineTeam(Team)?FMath::Max(0.f,RallyReadyAt[Team]-Elapsed):MAX_flt; }
float AWarfareFrontlineGameMode::MortarCooldown(int32 Team) const
{ return IsFrontlineTeam(Team)?FMath::Max(0.f,MortarReadyAt[Team]-Elapsed):MAX_flt; }

FVector2D AWarfareFrontlineGameMode::FireSupportScatterExtent(EFrontlineSupport Support,int32 Team) const
{
    if(Support==EFrontlineSupport::CreepingBarrage)return FVector2D(3160,720);
    if(Support==EFrontlineSupport::AirRecon)return FVector2D(1100,600);
    if(Support==EFrontlineSupport::AirBombing)return FVector2D(2060,770);
    if(Support==EFrontlineSupport::Gas)return FVector2D(950,720);
    if(Support==EFrontlineSupport::AntiTank)return FVector2D(1350,1350);
    if(Support==EFrontlineSupport::AirStrike)return FVector2D(2100,1050);
    if(Support==EFrontlineSupport::Smoke)return FVector2D(FrontlineFireSupportV26::SmokeRadius);
    return FrontlineFireSupportV26::ScatterExtent(Support==EFrontlineSupport::Mortar,
        HasFactionTechnology(EFrontlineFaction::German,10,Team));
}
FVector2D AWarfareFrontlineGameMode::FireSupportDelayRange(EFrontlineSupport Support,const FVector& Target,int32 Team) const
{
    if(WarfareFRRUCampaignV82::Active(this))
    {const float Arrival=WarfareFRRUEquipmentV82::SupportArrival(this,Support,Team);return FVector2D(Arrival,Arrival);}
    const float Pace=FrontlineFireSupportV26::ResponseScaleV37;
    const float Registration=FrontlineSupportV32::HasRegistrationV37(this,Team,Target)?2.f:0.f;
    const float Telephone=FrontlineCampaignV41::TelephoneResponseRelief(this,Team);
    if(Support==EFrontlineSupport::CreepingBarrage)return FVector2D(18,22)*Pace-FVector2D(Registration+Telephone);
    if(Support==EFrontlineSupport::AirRecon)return FVector2D(11,14)*Pace-FVector2D(Telephone);
    if(Support==EFrontlineSupport::AirBombing)return FVector2D(27,32)*Pace-FVector2D(Registration);
    if(Support==EFrontlineSupport::Gas)return FVector2D(18,22)*Pace-FVector2D(Telephone);
    if(Support==EFrontlineSupport::AntiTank)return FVector2D(5,7);
    if(Support==EFrontlineSupport::AirStrike)return FVector2D(20,25)*Pace;
    if(Support==EFrontlineSupport::Smoke)return FVector2D(FrontlineFireSupportV26::SmokeDelayMin,FrontlineFireSupportV26::SmokeDelayMax)-FVector2D(Telephone);
    return FrontlineFireSupportV26::DelayRange(Support==EFrontlineSupport::Mortar,
        FrontlineSupportV32::HasRegistrationV37(this,Team,Target)||(HasFactionTechnology(EFrontlineFaction::British,10,Team) && HasForwardObserver(Target,Team)))-FVector2D(Telephone+(FrontlineGameplayV51::HasObservation(this,Team,Target)?2.5f:0.f));
}
bool AWarfareFrontlineGameMode::IsFireSupportAreaSafe(const FVector& Target,EFrontlineSupport Support,int32 Team) const
{
    if(!IsFrontlineTeam(Team)||Target.ContainsNaN())return false;
    if(Support==EFrontlineSupport::Smoke||Support==EFrontlineSupport::Rally)return true;
    const auto Extent=FireSupportScatterExtent(Support,Team);
    // AT mission selection covers a broad search sector. The gun's impact is
    // local; applying the sector as shell dispersion blocked its own observer.
    const float Blast=Support==EFrontlineSupport::AntiTank?300.f:
        Support==EFrontlineSupport::Mortar?FrontlineFireSupportV26::MortarBlastRadius:
        Support==EFrontlineSupport::AirRecon?1200.f:Support==EFrontlineSupport::AirBombing?820.f:
        Support==EFrontlineSupport::CreepingBarrage?780.f:
        Support==EFrontlineSupport::Gas||Support==EFrontlineSupport::AirStrike?0.f:FrontlineFireSupportV26::FieldGunBlastRadius;
    // Request-time conservative footprint. Troops may subsequently enter the
    // ordered area; the mission is never secretly moved or homed upon them.
    for(const AWarfareFrontlineSoldier* U:Units)if(U && U->Team==Team && U->IsActiveCombatant())
    {
        const auto Delta=U->Feet()-Target;
        if(Support==EFrontlineSupport::AntiTank)
        {if(FMath::Abs(Delta.X)<=500.f&&FMath::Abs(Delta.Y)<=500.f)return false;continue;}
        if(Support==EFrontlineSupport::CreepingBarrage)
        {
            const float Along=Delta.X*GetTeamDirection(Team);
            if(Along>=-220.f-Blast-160.f&&Along<=Extent.X+Blast+160.f&&FMath::Abs(Delta.Y)<=Extent.Y+Blast+160.f)return false;
            continue;
        }
        if(FMath::Abs(Delta.X)<=Extent.X+Blast+160 && FMath::Abs(Delta.Y)<=Extent.Y+Blast+160)return false;
    }
    return true;
}

FString AWarfareFrontlineGameMode::SupportBlockReason(EFrontlineSupport Support,int32 Team) const
{
    if (!IsFrontlineTeam(Team) || uint8(Support)>=uint8(EFrontlineSupport::Count)) return TEXT("支援不可用。");
    if (Support==EFrontlineSupport::Rally&&!WarfareFRRUCampaignV82::Active(this)) return TEXT("由前线军士就地指挥。");
    if(!IsSupportUnlocked(Support,Team))return SupportUnlockReason(Support,Team);
    if(const FString Loadout=FrontlineCampaignV32::SupportBlockReason(this,Support,Team);!Loadout.IsEmpty())return Loadout;
    if (!AssetFailure.IsEmpty()) return AssetFailure;
    if (Phase!=EFrontlinePhase::Fighting) return TEXT("请先开始战斗。");
    if (UGameplayStatics::IsGamePaused(this)) return TEXT("暂停中，请先继续。");
    const float Cooldown=SupportCooldown(Support,Team);
    if (Cooldown>.01f) return TEXT("该支援仍在整备。");
    return FString();
}
FString AWarfareFrontlineGameMode::SmokeBlockReason(int32 Team) const { return SupportBlockReason(EFrontlineSupport::Smoke,Team); }
FString AWarfareFrontlineGameMode::RallyBlockReason(int32 Team) const { return SupportBlockReason(EFrontlineSupport::Rally,Team); }
FString AWarfareFrontlineGameMode::MortarBlockReason(int32 Team) const { return SupportBlockReason(EFrontlineSupport::Mortar,Team); }

float AWarfareFrontlineGameMode::DirectFireMultiplier(const FVector& From,const FVector& To) const
{
    const FVector2D A(From.X,From.Y),B(To.X,To.Y),Segment=B-A;
    const double LengthSquared=Segment.SizeSquared();
    float Transmission=1.f;
    for (const FFrontlineSmokeZone& Zone:SmokeZones) if (Elapsed>=Zone.StartedAt && Elapsed<Zone.ExpiresAt)
    {
        const FVector2D Centre(Zone.Centre.X,Zone.Centre.Y);
        const double T=LengthSquared>UE_SMALL_NUMBER?FMath::Clamp(FVector2D::DotProduct(Centre-A,Segment)/LengthSquared,0.0,1.0):0;
        if (FVector2D::DistSquared(Centre,A+Segment*T)<=FMath::Square(double(Zone.Radius)))
            Transmission=FMath::Min(Transmission,Zone.bGas?.40f:.22f);
    }
    return Transmission;
}

bool AWarfareFrontlineGameMode::RequestSmoke(const FVector& Target,int32 Team)
{
    const FString Reason=SmokeBlockReason(Team); if (!Reason.IsEmpty()) { if (Team==0) LastMessage=Reason; return false; }
    if (!ValidSupportPoint(Target)) { if (Team==0) LastMessage=TEXT("请选择战线上的有效落点。"); return false; }
    FVector Position=Target; Position.Z=AWarfareFrontlineGround::HeightAt(Target.X,Target.Y);
    AWarfareFrontlineSmoke* Visual=GetWorld()->SpawnActor<AWarfareFrontlineSmoke>(Position,FRotator::ZeroRotator);
    if (!IsValid(Visual) || !Visual->bVisualReady)
    {
        if (IsValid(Visual)) Visual->Destroy();
        if (Team==0) LastMessage=TEXT("烟幕暂不可用，整备已保留。"); return false;
    }
    FRandomStream Arrival(FrontlineDifficultyV30::SupportSeed(this,Team,191726+SmokeCommands*719+Team*131));
    const FVector2D DelayRange=FireSupportDelayRange(EFrontlineSupport::Smoke,Position,Team);
    const float Delay=Arrival.FRandRange(DelayRange.X,DelayRange.Y);
    Visual->DeployAt=Elapsed+Delay;Visual->SetLifeSpan(Delay+FrontlineFireSupportV26::SmokeDuration+.2f);
    for(int32 Group=0;Group<FrontlineFireSupportV26::SmokeRoundsV37;++Group)
    {
        FVector Landing=Position+FVector((Group%3-1)*220.f+(Group>=3?55.f:0.f),(Group%3-1)*120.f+(Group>=3?-45.f:0.f),0);
        Landing.X=FrontlineFireSupportV26::ReflectInside(Landing.X,FrontlineBattlefield::SupportHalfLength);
        Landing.Y=FrontlineFireSupportV26::ReflectInside(Landing.Y,FrontlineBattlefield::HalfFrontage);
        Landing.Z=AWarfareFrontlineGround::HeightAt(Landing.X,Landing.Y);
        ScheduleFrontlineSupportArrivalV30(this,Landing,Team,Visual->DeployAt+Group*FrontlineFireSupportV26::SmokeBurstSpacing,true);
    }
    SmokeReadyAt[Team]=Elapsed+(WarfareFRRUCampaignV82::Active(this)?WarfareFRRUEquipmentV82::SupportCooldown(this,EFrontlineSupport::Smoke,Team):FrontlineGameplayV43::PreparationSeconds(SupportCooldownSeconds(EFrontlineSupport::Smoke),Team,GetCampaignDifficulty()));
    FFrontlineSmokeZone Zone; Zone.Team=Team; Zone.Centre=Position; Zone.Radius=FrontlineFireSupportV26::SmokeRadius; Zone.StartedAt=Elapsed+Delay;
    Zone.ExpiresAt=Zone.StartedAt+FrontlineFireSupportV26::SmokeDuration;
    SmokeZones.Add(Zone); ++SmokeCommands;
    FrontlineReportV839::SupportDelivered(this,Team,int32(EFrontlineSupport::Smoke));
    if (Team==0) LastMessage=TEXT("Smoke ordered. Two groups will renew the screen; advance behind it.");
    return true;
}

bool AWarfareFrontlineGameMode::RequestRally(const FVector& Target,int32 Team)
{
    const FString Reason=RallyBlockReason(Team); if (!Reason.IsEmpty()) { if (Team==0) LastMessage=Reason; return false; }
    if (!ValidSupportPoint(Target)) { if (Team==0) LastMessage=TEXT("请选择战线上的有效落点。"); return false; }
    TArray<AWarfareFrontlineSoldier*> Affected;
    for (AWarfareFrontlineSoldier* U:Units) if (U->Team==Team && U->IsAlive() && FVector::Dist2D(U->Feet(),Target)<=1400) Affected.Add(U);
    if (Affected.IsEmpty()) { if (Team==0) LastMessage=TEXT("附近没有可集结的友军。"); return false; }
    RallyReadyAt[Team]=Elapsed+(WarfareFRRUCampaignV82::Active(this)?WarfareFRRUEquipmentV82::SupportCooldown(this,EFrontlineSupport::Rally,Team):FrontlineGameplayV43::PreparationSeconds(SupportCooldownSeconds(EFrontlineSupport::Rally),Team,GetCampaignDifficulty()));
    for (AWarfareFrontlineSoldier* U:Affected)
    {
        RallySuppressionRemoved[Team]+=U->Suppression; U->Suppression=0; U->RallyUntil=Elapsed+14;
    }
    const float Before=Morale[Team];FrontlineMoraleV36::Rally(this,Team);
    RallyMoraleRestored[Team]+=Morale[Team]-Before; ++RallyCommands;
    FrontlineReportV839::SupportDelivered(this,Team,int32(EFrontlineSupport::Rally));
    if (Team==0)
    {
        LastMessage=TEXT("集结生效：解除压制、恢复士气，14 秒内加快推进与射击。");
    }
    return true;
}

bool AWarfareFrontlineGameMode::RequestMortar(const FVector& Target,int32 Team)
{
    const FString Reason=MortarBlockReason(Team);
    if (!Reason.IsEmpty()) { if (Team==0) LastMessage=Reason; return false; }
    if (!ValidSupportPoint(Target)) { if (Team==0) LastMessage=TEXT("请选择战线上的有效落点。"); return false; }
    if(Team==1 && !IsFireSupportAreaSafe(Target,EFrontlineSupport::Mortar,Team))return false;
    const bool Observed=HasFactionTechnology(EFrontlineFaction::British,10,Team) && HasForwardObserver(Target,Team);
    const bool AirRegistered=FrontlineSupportV32::HasRegistrationV37(this,Team,Target);
    const bool Coordinated=HasFactionTechnology(EFrontlineFaction::British,11,Team) && IsInFriendlySmoke(Target,Team);
    const auto Plan=FrontlineFireSupportV26::Plan(true,FrontlineDifficultyV30::SupportSeed(this,Team,191726+MortarCommands*917+Team*131),Observed,
        HasFactionTechnology(EFrontlineFaction::German,10,Team),Coordinated,AirRegistered,FrontlineCampaignV41::TelephoneResponseRelief(this,Team),FrontlineGameplayV51::HasObservation(this,Team,Target));
    for (const auto& Round:Plan)
    {
        FFrontlineStrike Strike; Strike.SourceV65=FFrontlineKillSourceV65::FireSupport(Team,int32(EFrontlineSupport::Mortar)); Strike.Team=Team; Strike.bMortar=true; Strike.Radius=FrontlineFireSupportV26::MortarBlastRadius;
        Strike.Target=Target+FVector(Round.Offset.X,Round.Offset.Y,0);
        Strike.Target.X=FrontlineFireSupportV26::ReflectInside(Strike.Target.X,FrontlineBattlefield::SupportHalfLength);
        Strike.Target.Y=FrontlineFireSupportV26::ReflectInside(Strike.Target.Y,FrontlineBattlefield::HalfFrontage);
        Strike.Target.Z=AWarfareFrontlineGround::HeightAt(Strike.Target.X,Strike.Target.Y);
        Strike.Origin=Strike.Target+FVector(-GetTeamDirection(Team)*650,0,1450);
        Strike.ImpactAt=Elapsed+Round.ImpactDelay+(WarfareFRRUCampaignV82::Active(this)?WarfareFRRUEquipmentV82::SupportArrival(this,EFrontlineSupport::Mortar,Team)-Plan[0].ImpactDelay:0.f);Strike.LaunchedAt=Strike.ImpactAt-Round.TerminalFlight;
        Strike.FireMissionId=200000+MortarCommands;Strike.SalvoIndex=Round.Salvo;Strike.RoundIndex=Round.Round;Strike.WarningLeadSeconds=Round.WarningLead;
        Strikes.Add(Strike);
    }
    MortarReadyAt[Team]=Elapsed+(WarfareFRRUCampaignV82::Active(this)?WarfareFRRUEquipmentV82::SupportCooldown(this,EFrontlineSupport::Mortar,Team):FrontlineGameplayV43::PreparationSeconds(SupportCooldownSeconds(EFrontlineSupport::Mortar),Team,GetCampaignDifficulty())); ++MortarCommands;
    FrontlineReportV839::SupportDelivered(this,Team,int32(EFrontlineSupport::Mortar));
    if (Team==0) LastMessage=AirRegistered?TEXT("Mortars registered by aircraft. Four groups will cover the photographed sector."):TEXT("Mortars ordered. Four groups will cover the selected ground.");
    return true;
}

void AWarfareFrontlineGameMode::UpdateSectionOrders()
{
    if (Phase!=EFrontlinePhase::Fighting || UGameplayStatics::IsGamePaused(this)) return;
    // A section uses the smoke before it calls for covered movement. Empty smoke
    // placements no longer make a disembodied recruitment-style acknowledgement.
    for(FFrontlineSmokeZone& Zone:SmokeZones)
    {
        if(Zone.bCoverCallMade || Elapsed<Zone.StartedAt || Elapsed>=Zone.ExpiresAt || Elapsed<Zone.NextVoiceCheckAt)continue;
        Zone.NextVoiceCheckAt=Elapsed+1.f;
        for(AWarfareFrontlineSoldier* Unit:Units)
            if(Unit && Unit->IsAlive() && Unit->Team==Zone.Team && (Unit->bMoving || Unit->bEngaging)
                && FVector::Dist2D(Unit->Feet(),Zone.Centre)<Zone.Radius)
            {
                if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
                    Zone.bCoverCallMade=Audio->PlayVoice(EFrontlineVoiceEvent::SmokeCover,Unit->GetFaction()==EFrontlineFaction::German,Unit->Feet(),Unit->SquadId);
                break;
            }
    }
    for (AWarfareFrontlineSoldier* Leader:Units)
    {
        const bool Officer=Leader->IsCommissionedOfficer();
        const bool AssaultOfficer=Officer && Leader->GetFaction()==EFrontlineFaction::German;
        if (!Leader->IsActiveCombatant() || !Leader->IsSectionLeader() || (Leader->bMoving && !AssaultOfficer)
            || Leader->Suppression>.85f || Elapsed<Leader->CommandReadyAt
            || (Officer && Leader->WeaponPhase!=EFrontlineWeaponPhase::Ready)) continue;
        const float Interval=FrontlineBalanceV50::OrderInterval(Officer);
        const float OrderRelief=FrontlineBalanceV50::OrderRelief(Officer,AssaultOfficer);
        const float OrderRange=FrontlineBalanceV50::OrderRange(Officer,AssaultOfficer);
        const FFrontlineSquad* Section=FindSquad(Leader->SquadId);
        if (!Section) continue;
        bool Issued=false;
        for (AWarfareFrontlineSoldier* Unit:Units)
        {
            if (Unit==Leader || Unit->Team!=Leader->Team || !Unit->IsActiveCombatant() || Unit->Suppression<.18f
                || Elapsed-Unit->LastSectionOrderAt<Interval || FVector::Dist2D(Leader->Feet(),Unit->Feet())>OrderRange) continue;
            const FFrontlineSquad* Nearby=FindSquad(Unit->SquadId);
            if (!Nearby) continue;
            // Orders carry across adjacent sections and parapets. Each recipient
            // has its own cooldown, so several leaders cannot stack one rally.
            const float Relief=FMath::Min(OrderRelief,Unit->Suppression);
            Unit->Suppression-=Relief; Unit->LastSectionOrderAt=Elapsed;
            if(Officer&&Relief>0)FrontlineAchievementsV76::OfficerOrder(this,Leader,Unit->SquadId,Relief);
            LeaderSuppressionRemoved[Leader->Team]+=Relief; Issued=true;
        }
        if (Issued)
        {
            Leader->CommandReadyAt=Elapsed+Interval; ++LeaderOrders[Leader->Team];
            Leader->LastSectionOrderAt=Elapsed;
            if (Officer) Leader->BeginOfficerCommand();
            if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
                Audio->PlayVoice(Officer?EFrontlineVoiceEvent::OfficerCommand:EFrontlineVoiceEvent::Cover,
                    Leader->GetFaction()==EFrontlineFaction::German,Leader->Feet(),Leader->SquadId);
        }
    }
}
