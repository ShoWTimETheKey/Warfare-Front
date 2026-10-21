#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class FJsonObject;

// Fixed plans contain no unit pointer or future target observation. The seconds
// and centimetres are compressed gameplay values, not real ballistic tables.
namespace FrontlineFireSupportV26
{
    constexpr int32 Salvos=4,NormalRoundsPerSalvo=3;
    constexpr float ResponseScaleV37=.88f,AirRegistrationScatterV37=.75f;
    constexpr float SmokeDelayMin=5.f*ResponseScaleV37,SmokeDelayMax=7.f*ResponseScaleV37,SmokeDuration=24.f,SmokeBurstSpacing=1.8f;
    constexpr int32 SmokeRoundsV37=6;
    constexpr float SmokeRadius=1050.f,MortarBlastRadius=365.f,FieldGunBlastRadius=600.f;
    struct FPlannedRound
    {
        FVector2D Offset=FVector2D::ZeroVector;
        float ImpactDelay=0,TerminalFlight=0,WarningLead=0;
        int32 Salvo=0,Round=0;
    };
    inline FVector2D ScatterExtent(bool Mortar,bool Surveyed=false)
    {return Mortar?FVector2D(1000,650)*(Surveyed?.6:1.):FVector2D(1050,690);}
    inline FVector2D DelayRange(bool Mortar,bool Observed=false)
    {return (Mortar?FVector2D(14,18):FVector2D(12,14))*ResponseScaleV37-FVector2D(Observed?2:0);}
    inline FVector2D Disk(FRandomStream& Random)
    {
        const float Angle=Random.FRandRange(0,2*PI),Radius=FMath::Sqrt(Random.FRand());
        return FVector2D(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius);
    }
    inline TArray<FPlannedRound> Plan(bool Mortar,int32 Seed,bool Observed=false,bool Surveyed=false,bool SmokeCoordinated=false,bool AirRegistered=false,float ResponseRelief=0.f,bool GroundObserved=false)
    {
        FRandomStream Random(Seed);TArray<FPlannedRound> Result;Result.Reserve(Salvos*NormalRoundsPerSalvo+(SmokeCoordinated?1:0));
        const auto Range=DelayRange(Mortar,Observed||AirRegistered),Extent=ScatterExtent(Mortar,Surveyed)*(AirRegistered?AirRegistrationScatterV37:1.f);
        float SalvoAt=FMath::Max(3.5f,Random.FRandRange(Range.X,Range.Y)-FMath::Clamp(ResponseRelief,0.f,2.f));
        const FVector2D RegistrationError=Disk(Random)*.16;
        for(int32 Salvo=0;Salvo<Salvos;++Salvo)
        {
            const auto SalvoError=Disk(Random)*.22;float RoundAt=SalvoAt;
            const int32 Rounds=NormalRoundsPerSalvo+(SmokeCoordinated && Salvo==Salvos-1?1:0);
            for(int32 Round=0;Round<Rounds;++Round)
            {
                FPlannedRound Shot;const auto UnitOffset=RegistrationError+SalvoError+Disk(Random)*.62;
                Shot.Offset=FVector2D(UnitOffset.X*Extent.X,UnitOffset.Y*Extent.Y)*(GroundObserved&&Salvo==0?.70f:1.f);Shot.ImpactDelay=FMath::Max(3.5f,RoundAt-(GroundObserved&&Salvo==0?2.5f:0.f));
                Shot.TerminalFlight=Mortar?3.f:2.4f;Shot.WarningLead=Mortar?1.2f:1.7f;Shot.Salvo=Salvo;Shot.Round=Round;
                Result.Add(Shot);RoundAt+=Random.FRandRange(Mortar?.65f:.48f,Mortar?1.05f:.85f);
            }
            SalvoAt+=Random.FRandRange(Mortar?5.4f:5.f,Mortar?6.2f:5.8f);
        }
        return Result;
    }
    inline float ReflectInside(float Value,float HalfSize)
    {
        // Mirror a sample that crosses a battlefield boundary; clamping piles
        // many otherwise different impacts onto an unnaturally exact edge.
        if(Value>HalfSize)return 2*HalfSize-Value;
        if(Value<-HalfSize)return -2*HalfSize-Value;
        return Value;
    }
}

// Root's isolated -WFV26Review calls this synchronous, focused production-API
// fixture; it does not advance the world, open windows or write player saves.
TSharedRef<FJsonObject> RunFrontlineV26SupportChecks(AWarfareFrontlineGameMode* Battle);
