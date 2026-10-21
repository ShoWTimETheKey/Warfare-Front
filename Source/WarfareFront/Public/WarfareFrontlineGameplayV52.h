#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
struct FFrontlineCampaignProgress;
struct FFrontlineTechnologyNode;
enum class EFrontlineRole : uint8;
namespace FrontlineGameplayV52
{
    bool SelectableRole(EFrontlineRole Role);
    EFrontlineRole CanonicalRole(EFrontlineRole Role);
    void NormalizeLegacyLoadout(FFrontlineCampaignProgress& Run);
    bool MobileAvailable(const AWarfareFrontlineGameMode* Battle,int32 Team=0);
    EFrontlineRole RecruitVariant(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Team=0);
    bool SetMobileGun(AWarfareFrontlineGameMode* Battle,bool Enabled);
    bool IsCommandLeader(const AWarfareFrontlineSoldier* Unit);
    const AWarfareFrontlineSoldier* CommandLeader(const AWarfareFrontlineSoldier* Unit);
    float CommandAccuracyScale(const AWarfareFrontlineSoldier* Unit);
    bool ObservationTrained(const AWarfareFrontlineSoldier* Unit);
    float PioneerContactRange(const AWarfareFrontlineSoldier* Unit);
    void AppendTechnologyNodes(TArray<FFrontlineTechnologyNode>& Nodes,bool German);
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
