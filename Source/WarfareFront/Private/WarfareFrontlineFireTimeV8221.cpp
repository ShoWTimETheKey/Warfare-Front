#include "WarfareFrontlineFireTimeV8221.h"
#include "WarfareFrontlineFireV33.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineGround.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include <limits>

namespace
{
struct FFireSampleV8221
{
    bool Ready=false,Retired=false,InvalidDeltaStable=false;
    int32 ActiveAtFourTenths=0;
    TArray<FTransform> Flame,Smoke;
};
bool SnapshotFireV8221(AFrontlineFuelFireV33* Fire,TArray<FTransform>& Flame,TArray<FTransform>& Smoke)
{
    TArray<UInstancedStaticMeshComponent*> Components;Fire->GetComponents(Components);
    for(auto* C:Components)
    {
        TArray<FTransform>* Target=C->GetName()==TEXT("AdvectedFuelBatch")?&Flame:
            C->GetName()==TEXT("FuelSootBatch")?&Smoke:nullptr;
        if(!Target)continue;
        Target->Reset();Target->Reserve(C->GetInstanceCount());
        for(int32 I=0;I<C->GetInstanceCount();++I)
        {
            FTransform Pose;if(!C->GetInstanceTransform(I,Pose,true)||Pose.ContainsNaN())return false;
            Target->Add(Pose);
        }
    }
    return Flame.Num()==AFrontlineFuelFireV33::FlameBudget&&Smoke.Num()==AFrontlineFuelFireV33::SmokeBudget;
}
bool SameFireV8221(const TArray<FTransform>& A,const TArray<FTransform>& B)
{
    if(A.Num()!=B.Num())return false;
    for(int32 I=0;I<A.Num();++I)if(!A[I].Equals(B[I],.25f))return false;
    return true;
}
FFireSampleV8221 FireSampleV8221(AWarfareFrontlineGameMode* B,float Step)
{
    FFireSampleV8221 Result;auto* Fire=B->GetWorld()->SpawnActor<AFrontlineFuelFireV33>();
    ON_SCOPE_EXIT{if(IsValid(Fire))Fire->Destroy();};
    if(!Fire||!Fire->IsReady())return Result;
    Fire->SetActorHiddenInGame(true);
    const FVector At(0,0,AWarfareFrontlineGround::HeightAt(0,0)+50.f);
    // Fresh production actors begin with the same production stream seed.
    // No private pool, substitute age equation or global service is modified.
    Fire->FuelBurst(At,12.f);
    const bool BurstRegistered=Fire->ActiveCount()==37;
    Fire->EmitBurningWreck(At+FVector(150,0,0),.05f,1.f,0.f,true,1.f);
    const bool VolumeRegistered=Fire->VisualStateV8214().VolumeSootV8216>0;
    const int32 FirstSteps=FMath::RoundToInt(.4f/Step),TotalSteps=FMath::RoundToInt(5.2f/Step);
    for(int32 I=0;I<FirstSteps;++I)Fire->Tick(Step);
    Result.ActiveAtFourTenths=Fire->ActiveCount();
    Result.Ready=BurstRegistered&&VolumeRegistered&&Result.ActiveAtFourTenths>0
        &&SnapshotFireV8221(Fire,Result.Flame,Result.Smoke);
    TArray<FTransform> AfterFlame,AfterSmoke;
    Fire->Tick(-.4f);Fire->Tick(std::numeric_limits<float>::quiet_NaN());Fire->Tick(0.f);
    Result.InvalidDeltaStable=Fire->ActiveCount()==Result.ActiveAtFourTenths
        &&SnapshotFireV8221(Fire,AfterFlame,AfterSmoke)
        &&SameFireV8221(Result.Flame,AfterFlame)&&SameFireV8221(Result.Smoke,AfterSmoke);
    for(int32 I=FirstSteps;I<TotalSteps;++I)Fire->Tick(Step);
    const auto End=Fire->VisualStateV8214();
    // All unchanged burst/wreck particle envelopes end before 5.2 game seconds;
    // the real source-fed soot envelope is two seconds without a new feed.
    Result.Retired=Fire->ActiveCount()==0&&End.PoolUsed==0&&End.VolumeSootV8216==0
        &&!Fire->IsActorTickEnabled()&&SnapshotFireV8221(Fire,AfterFlame,AfterSmoke);
    return Result;
}
}

void RunFireTimeChecksV8221(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString Dir;
    if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV8221Review"))
        ||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir))return;
    Check(TEXT("v8221_fire_time_has_real_review_camera"),B->GetWorld()->GetFirstPlayerController()!=nullptr);
    if(!B->GetWorld()->GetFirstPlayerController())return;
    const auto Fine=FireSampleV8221(B,.05f),Medium=FireSampleV8221(B,.2f),Coarse=FireSampleV8221(B,.4f);
    const bool Ready=Fine.Ready&&Medium.Ready&&Coarse.Ready;
    Check(TEXT("v8221_fire_time_uses_actual_ready_fuel_and_soot_assets"),Ready);
    Check(TEXT("v8221_fire_time_actual_point_four_motion_matches_bounded_substeps"),Ready
        &&Fine.ActiveAtFourTenths==Medium.ActiveAtFourTenths&&Fine.ActiveAtFourTenths==Coarse.ActiveAtFourTenths
        &&SameFireV8221(Fine.Flame,Medium.Flame)&&SameFireV8221(Fine.Flame,Coarse.Flame)
        &&SameFireV8221(Fine.Smoke,Medium.Smoke)&&SameFireV8221(Fine.Smoke,Coarse.Smoke));
    Check(TEXT("v8221_fire_time_all_three_steps_retire_at_same_game_budget"),Ready
        &&Fine.Retired&&Medium.Retired&&Coarse.Retired);
    Check(TEXT("v8221_fire_time_zero_negative_nan_do_not_advance_actual_pool"),Ready
        &&Fine.InvalidDeltaStable&&Medium.InvalidDeltaStable&&Coarse.InvalidDeltaStable);
}

