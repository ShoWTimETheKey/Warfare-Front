#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineProgression.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class FJsonObject;
namespace FrontlineDifficultyV30
{
    struct FDoctrine
    {
        float RecruitScale=1,DecisionScale=1,WaveRestScale=1,AssaultIntervalScale=1;
        float AssessMin=6,AssessMax=12,ReactionMin=2.8f,ReactionMax=5.8f;
        float MemorySeconds=4,RegistrationError=115,MinimumTargetValue=3.25f;
        float ExtendedAssessment=9,FireCoordinationGap=16;
        float SmokePressure=.38f;
    };
    struct FContact
    {
        TWeakObjectPtr<AWarfareFrontlineSoldier> Identity;
        FVector Position=FVector::ZeroVector,ObservedVelocity=FVector::ZeroVector;
        float LastSeen=0,Pressure=0;
        bool Moving=false,Machinegun=false,Leader=false,Entrenched=false,Fired=false,Rifle=false;
    };
    struct FState
    {
        FGuid Encounter;
        TArray<FContact> Contacts;
        float LastSupportAt=-1000,LastContactUpdate=-1000;
        int32 Observations=0,PlannedFireMissions=0,ExpiredContactCount=0,SupportSeedSerial=0;
    };
    const FDoctrine& Doctrine(EFrontlineDifficulty Difficulty);
    const TCHAR* Label(EFrontlineDifficulty Difficulty);
    const TCHAR* Description(EFrontlineDifficulty Difficulty);
    bool ReadArchiveDifficulty(const FJsonObject& Json,EFrontlineDifficulty& Difficulty);
    bool CanObserve(const AWarfareFrontlineGameMode* Battle,const AWarfareFrontlineSoldier* Unit);
    int32 SupportSeed(AWarfareFrontlineGameMode* Battle,int32 Team,int32 PlayerSeed);
    FState& State(AWarfareFrontlineGameMode* Battle);
    void Reset(AWarfareFrontlineGameMode* Battle);
    void Observe(AWarfareFrontlineGameMode* Battle,const TArray<AWarfareFrontlineSoldier*>& Seen);
    float Confidence(const FContact& Contact,float Now,const FDoctrine& Doctrine);
    float RememberedStrength(const TArray<FContact>& Contacts,float Now,const FDoctrine& Doctrine,float TargetX,float Radius=1100.f);
    bool CanAssault(float Friendly,float ObservedOpposition,float RememberedOpposition,float AveragePressure,bool EnemyGun,bool Counterattack);
    float TargetScore(const FContact& Candidate,const TArray<FContact>& Contacts,float Now,const FDoctrine& Doctrine);
    FVector RegisteredPoint(const FContact& Contact,float Now,const FDoctrine& Doctrine,FRandomStream& Random);
}
TSharedRef<FJsonObject> RunFrontlineV30TacticsChecks(AWarfareFrontlineGameMode* Battle);
