#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineBattlefield.h"
#include "Engine/World.h"

namespace FrontlineGrenade
{
float FlightSeconds(float RangeCm)
{
    return MinFlightSeconds+FlightRangeSeconds*FMath::Clamp(RangeCm/TrajectoryReferenceRangeCm,0.f,MaxRangeCm/TrajectoryReferenceRangeCm);
}

float HitChanceFalloff(float DistanceCm,float RadiusCm)
{
    // A close open-ground burst remains dangerous. Fragment density drops
    // towards the perimeter, reducing the chance of a fragment reaching a man.
    const float T=FMath::Clamp((DistanceCm-FullExposureRadiusCm)/FMath::Max(1.f,RadiusCm-FullExposureRadiusCm),0.f,1.f);
    return FMath::Square(1.f-T);
}

FVector EstimateAim(const AWarfareFrontlineSoldier* Shooter,const AWarfareFrontlineSoldier* Target)
{
    const AWarfareFrontlineGameMode* Battle=Shooter->Battle.Get();
    const FVector Observed=Target->Feet();
    const FVector From=Shooter->Feet();
    const float Range=FVector::Dist2D(From,Observed);
    FVector Along=(Observed-From).GetSafeNormal2D();
    if(Along.IsNearlyZero())Along=FVector(Battle->GetTeamDirection(Shooter->Team),0,0);
    const FVector Across(-Along.Y,Along.X,0);
    const float RangeFraction=FMath::Clamp(Range/TrajectoryReferenceRangeCm,0.f,MaxRangeCm/TrajectoryReferenceRangeCm);
    const float Pressure=FMath::Clamp(Shooter->Suppression,0.f,1.f);
    const float TargetMovement=FMath::Clamp(Target->GetMotionSpeed()/390.f,0.f,1.f);
    const float ThrowerMovement=FMath::Clamp(Shooter->GetMotionSpeed()/390.f,0.f,1.f);
    const bool Obscured=!Battle->HasSight(Shooter,Target)
        || Battle->IsInFriendlySmoke(Observed,0) || Battle->IsInFriendlySmoke(Observed,1);
    const float Cover=FMath::Clamp(Battle->CoverFor(Target),0.f,1.f);
    const float Scatter=AimScatterMultiplier*FMath::Clamp(BaseScatterCm+RangeScatterCm*FMath::Square(RangeFraction)
        +PressureScatterCm*Pressure+TargetMovementScatterCm*TargetMovement+ThrowerMovementScatterCm*ThrowerMovement
        +ObscuredScatterCm*float(Obscured)+CoverScatterCm*Cover,BaseScatterCm,MaxScatterCm);

    // Dedicated personal/throw seed: aiming does not consume the animation,
    // bullet-accuracy or artillery random streams. Uniform area, bounded ellipse.
    const uint32 Seed=uint32(Shooter->SquadId+1)*92821u ^ uint32(Shooter->MemberIndex+1)*68917u
        ^ uint32(Shooter->Team+1)*283923u ^ uint32(Shooter->GrenadeThrowsStarted+1)*15485863u ^ 191717u;
    FRandomStream Dispersion{int32(Seed)};
    const float Angle=Dispersion.FRand()*2.f*PI;
    const float Radius=FMath::Sqrt(Dispersion.FRand())*Scatter;
    const float LeadSeconds=Dispersion.FRandRange(.12f,.28f)*(Obscured?.35f:1.f)*(1.f-.5f*Pressure);
    // Only a short estimate from motion visible during preparation, never a
    // perfect intercept using the whole animation/flight time or future orders.
    const FVector Lead=Target->GetActorForwardVector().GetSafeNormal2D()*FMath::Min(Target->GetMotionSpeed(),390.f)*LeadSeconds;
    FVector Aim=Observed+Lead+Along*(FMath::Cos(Angle)*Radius)+Across*(FMath::Sin(Angle)*Radius*CrossRangeScale);
    Aim.X=FMath::Clamp(Aim.X,-double(FrontlineBattlefield::SupportHalfLength),double(FrontlineBattlefield::SupportHalfLength));
    Aim.Y=FMath::Clamp(Aim.Y,-double(FrontlineBattlefield::HalfFrontage),double(FrontlineBattlefield::HalfFrontage));
    Aim.Z=AWarfareFrontlineGround::HeightAt(Aim.X,Aim.Y);
    if(Shooter->GrenadeThrowsStarted<3)
        UE_LOG(LogTemp,Display,TEXT("WF_V17_GRENADE_PREP squad=%d member=%d throw=%d observed=%s fixed_aim=%s range_cm=%.1f scatter_cm=%.1f suppression=%.2f moving_target=%.2f obscured=%d seed=%u"),
            Shooter->SquadId,Shooter->MemberIndex,Shooter->GrenadeThrowsStarted+1,*Observed.ToString(),*Aim.ToString(),Range,Scatter,Pressure,TargetMovement,int32(Obscured),Seed);
    return Aim;
}

float BlastExposure(const AWarfareFrontlineGameMode* Battle,const FVector& Position,const AWarfareFrontlineSoldier* Unit)
{
    // Two bounded terrain/prop visibility rays; bodies cannot provide a free
    // blast shield. Both armies and friendly fire receive the same attenuation.
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FrontlineGrenadeBlast),true);
    for(const AWarfareFrontlineSoldier* Other:Battle->Units)if(Other)Params.AddIgnoredActor(Other);
    const FVector Start=Position+FVector(0,0,18);
    int32 Clear=0;
    for(float Height:{45.f,115.f})
    {
        FHitResult Hit;
        if(!Battle->GetWorld()->LineTraceSingleByChannel(Hit,Start,Unit->Feet()+FVector(0,0,Height),ECC_Visibility,Params))++Clear;
    }
    return Clear==2?1.f:(Clear==1?.60f:.18f);
}
}
