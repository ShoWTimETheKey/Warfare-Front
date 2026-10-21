#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class AFrontlineTank;
class FJsonObject;
struct FFrontlineStrike;
namespace FrontlineFieldSitesV59
{
enum class EKind:uint8 { Dugout, Stores, AntiAircraft, Casemate, FieldWorks };
struct FView
{
    FVector Position=FVector::ZeroVector,MarkerPosition=FVector::ZeroVector;
    bool ShowOwnershipFlag=false;
    EKind Kind=EKind::Dugout;
    int32 Owner=INDEX_NONE,Garrison=0,CapturingTeam=INDEX_NONE;
    float Capture=0,Integrity=1;
    float Work=0;
    int32 Ammunition=0;
    bool RewardTaken=false,Operating=false,Contested=false;
};
struct FRepairViewV60
{
    TWeakObjectPtr<AFrontlineTank> Tank;
    FVector Position=FVector::ZeroVector;
    float Work=0;
    int32 Side=INDEX_NONE;
    bool Working=false;
};
void Reset(AWarfareFrontlineGameMode* B);
void Begin(AWarfareFrontlineGameMode* B);
void Tick(AWarfareFrontlineGameMode* B,float DT);
void OnBlast(AWarfareFrontlineGameMode* B,const FFrontlineStrike& Strike);
float BulletCover(const AWarfareFrontlineSoldier* Target);
float PreparedFrontCoverV60(const AWarfareFrontlineSoldier* Shooter,const AWarfareFrontlineSoldier* Target);
bool EngineerDestinationV60(const AWarfareFrontlineSoldier* Unit,FVector& Destination);
TArray<FRepairViewV60> RepairViewsV60(const AWarfareFrontlineGameMode* Battle);
FString RepairDescriptionV60(const FRepairViewV60& View);
bool EngageAircraft(AWarfareFrontlineGameMode* B,int32 AircraftTeam,const FVector& Position,uint32 AircraftId,float DT);
TArray<FView> Views(const AWarfareFrontlineGameMode* B);
FString Title(EKind Kind);
FString Description(const FView& View);
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
void RunAirCoverChecksV8219(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
// Disposable review only: a registered production gun, restored after Body.
bool WithAirCoverReviewV8220(AWarfareFrontlineGameMode* B,int32 GunTeam,TFunctionRef<void(const FVector&)> Body);
void RunChecksV60(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
void RunFlagChecksV64(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
TSharedRef<FJsonObject> DescribeVisualsV8449(AWarfareFrontlineGameMode* B);
void RunRepairChecksV63(AWarfareFrontlineGameMode* B,AFrontlineTank* Tank,AWarfareFrontlineSoldier* Engineer,AFrontlineTank* Threat,TFunctionRef<void(const FString&,bool)> Check);
}
