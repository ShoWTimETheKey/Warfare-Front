#include "WarfareFrontlineMedalMotionV8216.h"
#include "Engine/World.h"
namespace FrontlineMedalMotionV8216
{
namespace { struct FHistory {uint64 Serial=0;TArray<FShock,TInlineAllocator<16>> Events;};TMap<TWeakObjectPtr<UWorld>,FHistory> Histories; }
void NotifyBlast(UWorld* World,const FVector& At,float Strength)
{
 if(!World||!FMath::IsFinite(Strength)||Strength<=0||At.ContainsNaN())return;
 auto& H=Histories.FindOrAdd(World);if(H.Events.Num()==16)H.Events.RemoveAt(0);
 H.Events.Add({++H.Serial,At,FMath::Clamp(Strength,0.f,2.f)});
 if(Histories.Num()>4)for(auto It=Histories.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
}
void ReadSince(UWorld* World,uint64& Serial,TArray<FShock,TInlineAllocator<16>>& Out)
{
 Out.Reset();if(auto* H=Histories.Find(World)){for(const auto& E:H->Events)if(E.Serial>Serial)Out.Add(E);Serial=H->Serial;}else Serial=0;
}
void FPendulum::Step(float Delta,float Acceleration,float Impulse)
{
 Delta=FMath::Clamp(Delta,0.f,.1f);if(Delta<=0)return;
 Velocity+=FMath::Clamp(Impulse,-32.f,32.f);
 const int32 Steps=FMath::Max(1,FMath::CeilToInt(Delta*120.f));const float D=Delta/Steps;
 for(int32 I=0;I<Steps;++I){Velocity+=(-62.f*Angle-7.5f*Velocity-FMath::Clamp(Acceleration*.0012f,-8.f,8.f))*D;Angle+=Velocity*D;}
 Angle=FMath::Clamp(Angle,-11.f,11.f);if(FMath::Abs(Angle)<.0001f&&FMath::Abs(Velocity)<.0001f)Angle=Velocity=0;
}
void RunChecks(TFunctionRef<void(const FString&,bool)> C)
{
 FPendulum Rest;for(int32 I=0;I<600;++I)Rest.Step(1.f/60.f,0,0);C(TEXT("v8216_medal_rest_is_stationary"),Rest.Angle==0&&Rest.Velocity==0);
 FPendulum A,B;A.Step(1.f/60.f,0,12);B.Step(1.f/120.f,0,12);B.Step(1.f/120.f,0,0);
 C(TEXT("v8216_medal_fixed_substeps_match_60_120fps"),FMath::IsNearlyEqual(A.Angle,B.Angle,.0001f));
 bool Signed=A.Angle>0;for(int32 I=0;I<600;++I)A.Step(1.f/60.f,0,0);
 C(TEXT("v8216_medal_impulse_damps_to_rest"),Signed&&FMath::Abs(A.Angle)<.001f&&FMath::Abs(A.Velocity)<.001f);
 FPendulum Limit;for(int32 I=0;I<600;++I)Limit.Step(.1f,100000,100);C(TEXT("v8216_medal_stress_is_finite_and_bounded"),FMath::IsFinite(Limit.Angle)&&FMath::IsFinite(Limit.Velocity)&&FMath::Abs(Limit.Angle)<=11);
}
}
