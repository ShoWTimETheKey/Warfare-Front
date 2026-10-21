#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontline.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunArmourChecksV40()
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;bool Passed=true;
    auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);Checks.Add(MakeShared<FJsonValueObject>(J));Passed&=Good;};
    ON_SCOPE_EXIT{Report->SetArrayField(TEXT("checks"),Checks);Report->SetBoolField(TEXT("success"),Passed);};
    auto* B=Battle();FString UserDir;
    if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV40Review"))||
        !FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
    {Check(TEXT("isolated_armour_review_required"),false);return Report;}
    const auto SavedVehicles=Vehicles;const auto Encounter=LastEncounter;
    const float Time=B->Elapsed,EnemyTime=NextEnemyAt;const auto Phase=B->Phase;const FString Message=B->LastMessage;
    const int32 Serial=NextSerial,Count0=DeploymentCountV37[0],Count1=DeploymentCountV37[1];
    float Ready[2][int32(EFrontlineVehicle::Count)];for(int32 T=0;T<2;++T)for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)Ready[T][I]=ReadyAt[T][I];
    const float Cycle[2]={ArmourCycleSecondsV40[0],ArmourCycleSecondsV40[1]};
    Vehicles.Empty();LastEncounter=B->GetEncounterId();B->Phase=EFrontlinePhase::Fighting;
    auto ClearFixtures=[&](){for(AFrontlineTank* T:Vehicles)if(IsValid(T))T->Destroy();Vehicles.Empty();};
    ON_SCOPE_EXIT
    {
        ClearFixtures();Vehicles=SavedVehicles;LastEncounter=Encounter;B->Elapsed=Time;B->Phase=Phase;B->LastMessage=Message;
        NextEnemyAt=EnemyTime;NextSerial=Serial;DeploymentCountV37[0]=Count0;DeploymentCountV37[1]=Count1;
        for(int32 T=0;T<2;++T){ArmourCycleSecondsV40[T]=Cycle[T];for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)ReadyAt[T][I]=Ready[T][I];}
    };
    for(int32 Team=0;Team<2;++Team)
    {
        const auto Roster=GetRoster(Team);
        Check(TEXT("separate_model_allocations_")+FString::FromInt(Team),Roster.Num()==3&&UnlockMission(Roster[0].Type,Team)<UnlockMission(Roster[1].Type,Team));
        if(Roster.Num()!=3)continue;
        for(int32 First=0;First<Roster.Num();++First)
        {
            ClearFixtures();ResetReadinessForDebug(0);ResetReadinessForDebug(1);B->Elapsed=130;
            const auto A=Roster[First].Type,C=Roster[(First+1)%Roster.Num()].Type;const FString Key=FString::Printf(TEXT("army%d_model%d_"),Team,First);
            Check(Key+TEXT("both_models_available_in_late_operation"),IsUnlocked(A,Team)&&IsUnlocked(C,Team));
            const bool Recruited=Recruit(A,Team);Check(Key+TEXT("actual_first_recruit"),Recruited);if(!Recruited)continue;
            const int32 Count=Vehicles.Num();const float ReadyTime=ReadyAt[Team][0];
            Check(Key+TEXT("other_model_cannot_bypass_shared_queue"),!Recruit(C,Team)&&Vehicles.Num()==Count);
            Check(Key+TEXT("cooldown_masks_match_at_dispatch"),FMath::IsNearlyEqual(CooldownFraction(A,Team),1.f,1.e-4f)&&FMath::IsNearlyEqual(CooldownFraction(C,Team),1.f,1.e-4f));
            Check(Key+TEXT("opposing_army_has_independent_queue"),CooldownFraction(GetRoster(1-Team)[0].Type,1-Team)==0.f);
            B->Elapsed=ReadyTime-ArmourCycleSecondsV40[Team]*.5f;
            Check(Key+TEXT("both_masks_halfway_at_same_time"),FMath::IsNearlyEqual(CooldownFraction(A,Team),.5f,1.e-4f)&&FMath::IsNearlyEqual(CooldownFraction(C,Team),.5f,1.e-4f));
            B->Elapsed=ReadyTime-.01f;Check(Key+TEXT("no_early_switch_recruitment"),!RecruitBlockReason(C,Team).IsEmpty());
            B->Elapsed=ReadyTime;Check(Key+TEXT("different_model_recruits_after_queue_finishes"),Recruit(C,Team)&&Vehicles.Num()==Count+1);
            Check(Key+TEXT("next_vehicle_restarts_both_masks"),FMath::IsNearlyEqual(CooldownFraction(A,Team),1.f,1.e-4f)&&FMath::IsNearlyEqual(CooldownFraction(C,Team),1.f,1.e-4f));
        }
    }
    return Report;
}
