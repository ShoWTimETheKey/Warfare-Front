#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class FJsonObject;
namespace FrontlineInfantryV45
{
    // Same authored triangle contacts as the displayed bank and casualty solver.
    float SurfaceHeight(AWarfareFrontlineGameMode* Battle,const FVector& Point);
    float BankIntrusion(const AWarfareFrontlineSoldier* Unit,const FVector& Feet);
    FVector TrenchSlot(const AWarfareFrontlineSoldier* Unit,int32 Trench,const FVector& Desired,bool FiringStep=false);
    bool HasValidatedFiringStepV831(const AWarfareFrontlineSoldier* Unit,const FVector& Destination);
    // Shared contact must still be observed by a living occupant of this same
    // trench; it grants a local placement search, never a shot through cover.
    AWarfareFrontlineSoldier* ObservedTrenchContactV844(const AWarfareFrontlineSoldier* Unit);
    void ObservedTrenchContactsV8444(const AWarfareFrontlineSoldier* Unit,TArray<AWarfareFrontlineSoldier*>& Contacts);
    void ReviewTrenchFiringSectorV844(AWarfareFrontlineSoldier* Unit);
    bool AllowsTrenchStep(const AWarfareFrontlineSoldier* Unit,const FVector& From,const FVector& To);
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    // Focused V46 production Tick / evaluated-pose fixture; restores the caller's squads.
    TSharedRef<FJsonObject> RunCrewChecksV46(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    TSharedRef<FJsonObject> RunAntiTankStanceChecksV48(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    // Production Tick approaches, not teleported firing positions. Disposable
    // profile only; restores the host world and never opens national campaigns.
    TSharedRef<FJsonObject> RunFireControlChecksV831(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    TSharedRef<FJsonObject> RunTrenchFireChecksV844(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
