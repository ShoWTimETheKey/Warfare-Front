#include "WarfareFrontlineSupportArrivalV30.h"
#include "WarfareFrontlineReportV839.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineWaterV51.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

namespace
{
bool BindShell(UStaticMeshComponent* C)
{
    UStaticMesh* Shape=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V36/Meshes/SM_Shell_V36.SM_Shell_V36"));
    UMaterialInterface* Steel=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V36/Materials/M_Airframe_V36.M_Airframe_V36"));
    if(!Shape || !Steel)return false;
    C->SetStaticMesh(Shape);C->SetMaterial(0,Steel);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCastShadow(false);C->SetReceivesDecals(false);C->SetRelativeScale3D(FVector::OneVector);return true;
}
FVector FlightPosition(const FVector& Origin,const FVector& Target,float Duration,float T)
{
    const FVector Velocity=(Target-Origin)/Duration+FVector(0,0,490.f*Duration);
    return Origin+Velocity*T-FVector(0,0,490.f*T*T);
}
}
AFrontlineSupportArrivalV30::AFrontlineSupportArrivalV30()
{
    PrimaryActorTick.bCanEverTick=true;
    Shell=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Approaching ammunition"));SetRootComponent(Shell);
    Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);Shell->SetCastShadow(false);Shell->SetVisibility(false);
}
bool AFrontlineSupportArrivalV30::Configure(AWarfareFrontlineGameMode* B,const FVector& P,int32 Team,float At,bool Dust)
{
    if(!B || !BindShell(Shell))return false;
    Battle=B;Encounter=B->GetEncounterId();Target=P;ImpactAt=At;LaunchAt=At-1.4f;bImpactDust=Dust;TeamV839=Team;
    Origin=Target+FVector(-B->GetTeamDirection(Team)*1100,Team?90:-90,2400);
    SetActorLocation(Origin);return true;
}
void AFrontlineSupportArrivalV30::Tick(float DT)
{
    Super::Tick(DT);auto* B=Battle.Get();
    if(!B || B->GetEncounterId()!=Encounter || !B->CanResolveCommittedSupportV8217()){Destroy();return;}
    const float Clock=B->CommittedSupportTimeV8217();
    if(Clock<LaunchAt)return;
    if(!bVisualLaunched)
    {
        if(bImpactDust)FrontlineReportV839::SupportRounds(B,TeamV839,int32(EFrontlineSupport::Smoke));
        bVisualLaunched=true;auto* Visual=GetWorld()->SpawnActor<AFrontlineOrdnanceV36>();
        if(Visual){Visual->SetCommittedSupportV8217();if(!Visual->Configure(B,Origin,Target,LaunchAt,ImpactAt,false))Visual->Destroy();}
    }
    if(Clock>=ImpactAt)
    {
        if(bImpactDust)
        {
            if(auto* Puff=GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Target,FRotator::ZeroRotator))Puff->ConfigureBulletStrike();
            if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->PlayImpact(true,Target);
        }
        // Smoke owns its dust here; gas owns its dust in EmitGas. Both have one
        // physical arrival, so only this actor submits their small water pulse.
        // Destroy below prevents any persistent cloud Tick from resubmitting.
        UFrontlineWaterResponseV51::NotifyImpact(GetWorld(),Target,180.f,.16f);
        Destroy();return;
    }
    Shell->SetVisibility(false);const float Duration=ImpactAt-LaunchAt,T=Clock-LaunchAt;
    SetActorLocation(FlightPosition(Origin,Target,Duration,T));
    const FVector Velocity=(Target-Origin)/Duration+FVector(0,0,490.f*Duration-980.f*T);
    SetActorRotation(Velocity.Rotation());
}
void ScheduleFrontlineSupportArrivalV30(AWarfareFrontlineGameMode* B,const FVector& Target,int32 Team,float ImpactAt,bool Dust)
{
    if(!B)return;
    auto* Arrival=B->GetWorld()->SpawnActor<AFrontlineSupportArrivalV30>();
    if(Arrival && !Arrival->Configure(B,Target,Team,ImpactAt,Dust))Arrival->Destroy();
}
void EnsureFrontlineSupportProjectileV30(AWarfareFrontlineGameMode* B,FFrontlineStrike& S)
{
    if(!B || S.bGrenade || S.bResolved || S.Projectile.IsValid() || B->CommittedSupportTimeV8217()<S.LaunchedAt || B->CommittedSupportTimeV8217()>=S.ImpactAt)return;
    S.Projectile=FrontlineSupportArtV36::SpawnShell(B,S);
}
