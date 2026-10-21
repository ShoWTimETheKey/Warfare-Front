#include "WarfareFrontlineGameplayV48.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlinePacingV24.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include <limits>

TSharedRef<FJsonObject> FrontlineGameplayV48::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto J=MakeShared<FJsonObject>();bool Good=true;
    auto Test=[&](const TCHAR* Name,bool Pass){Check(Name,Pass);Good&=Pass;};
    FString Dir;
    const bool Allowed=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV48Review"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty();
    Test(TEXT("v48_damage_requires_isolated_volatile_profile"),Allowed);
    if(!Allowed){J->SetBoolField(TEXT("success"),false);return J;}
    const auto Units=B->Units;const auto Squads=B->Squads;const auto Phase=B->Phase;
    const int32 C0=B->Casualties[0],C1=B->Casualties[1];const float D0=B->DamageDealt[0],D1=B->DamageDealt[1];
    const float M0=B->Morale[0],M1=B->Morale[1],FF=B->FriendlyFireDamage,Peak=B->PeakSuppression;
    const FString Message=B->LastMessage;TArray<AWarfareFrontlineSoldier*> Created;
    B->Units.Empty();B->Squads.Empty();B->Phase=EFrontlinePhase::Fighting;
    ON_SCOPE_EXIT
    {
        for(auto* U:Created)if(IsValid(U))U->Destroy();B->Units=Units;B->Squads=Squads;B->Phase=Phase;
        B->Casualties[0]=C0;B->Casualties[1]=C1;B->DamageDealt[0]=D0;B->DamageDealt[1]=D1;
        B->Morale[0]=M0;B->Morale[1]=M1;B->FriendlyFireDamage=FF;B->PeakSuppression=Peak;B->LastMessage=Message;
    };
    auto Make=[&](EFrontlineRole Role,int32 Member=0,int32 Team=0)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        Created.Add(U);U->Initialize(B,74800+Created.Num(),Team,Role,Member);U->SetActorTickEnabled(false);
        U->bFieldEntered=true;U->Health=100;U->Suppression=0;
        U->SetActorLocation(FVector(50000+Created.Num()*8000,0,12000));B->Units.Add(U);return U;
    };
    bool BothSides=true;
    for(int32 Team=0;Team<2;++Team)
    {
        auto* U=Make(EFrontlineRole::Flamethrower,0,Team);if(!U){BothSides=false;continue;}
        const int32 Before=B->Casualties[Team];const float Damage=B->DamageDealt[1-Team];
        for(float Invalid:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
            U->ReceiveHit(Invalid,.01f,1-Team,FVector::ForwardVector,EFrontlineDamageKind::Bullet);
        BothSides&=U->Health==100&&!U->bFlameBulletProtectionSpentV48;
        U->ReceiveHit(1,.2f,1-Team,FVector::ForwardVector,EFrontlineDamageKind::Bullet);
        BothSides&=U->Health==100&&U->bFlameBulletProtectionSpentV48&&U->Suppression>=.2f
            &&B->Casualties[Team]==Before&&B->DamageDealt[1-Team]==Damage;
        U->Suppression=0; // Cover/rally cannot regenerate the spent protection.
        U->ReceiveHit(1,0,1-Team,FVector::ForwardVector,EFrontlineDamageKind::Bullet);
        BothSides&=!U->IsAlive()&&B->Casualties[Team]==Before+1&&B->DamageDealt[1-Team]==Damage+100;
        U->ReceiveHit(1,0,1-Team,FVector::ForwardVector,EFrontlineDamageKind::Bullet);
        BothSides&=B->Casualties[Team]==Before+1;
    }
    Test(TEXT("v48_both_flame_operators_survive_first_landed_round_only_binary_strength_and_one_casualty"),BothSides);
    bool OtherHits=true;
    for(auto Kind:{EFrontlineDamageKind::Grenade,EFrontlineDamageKind::Artillery,EFrontlineDamageKind::Gas,EFrontlineDamageKind::Fire})
    {
        auto* U=Make(EFrontlineRole::Flamethrower);if(!U){OtherHits=false;continue;}
        U->ReceiveHit(1,0,1,FVector::ForwardVector,Kind);OtherHits&=!U->IsAlive();
    }
    Test(TEXT("v48_explosion_fire_and_gas_bypass_firearm_protection"),OtherHits);
    auto* Mixed=Make(EFrontlineRole::Flamethrower);
    if(Mixed){Mixed->ReceiveHit(1,0,1,FVector::ForwardVector,EFrontlineDamageKind::Bullet);Mixed->ReceiveHit(1,0,1,FVector::ForwardVector,EFrontlineDamageKind::Artillery);}
    Test(TEXT("v48_explosion_after_first_round_remains_fatal"),Mixed&&!Mixed->IsAlive());
    bool Escorts=true;
    for(auto Role:{EFrontlineRole::Flamethrower,EFrontlineRole::Rifle})
    {
        auto* U=Make(Role,1);if(!U){Escorts=false;continue;}
        U->ReceiveHit(1,0,1,FVector::ForwardVector,EFrontlineDamageKind::Bullet);Escorts&=!U->IsAlive();
    }
    Test(TEXT("v48_riflemen_and_flame_rifle_escorts_retain_one_round_lethality"),Escorts);
    auto* Fresh=Make(EFrontlineRole::Flamethrower);
    if(Fresh){Fresh->bFlameBulletProtectionSpentV48=true;Fresh->Initialize(B,74899,0,EFrontlineRole::Flamethrower,0);}
    Test(TEXT("v48_initialization_resets_once_only_operator_protection"),Fresh&&!Fresh->bFlameBulletProtectionSpentV48);
    const float AT=B->RoleRecruitSeconds(EFrontlineRole::AntiTankRifle),Flame=B->RoleRecruitSeconds(EFrontlineRole::Flamethrower);
    Test(TEXT("v48_anti_tank_preparation_is_30_percent_shorter"),FMath::IsNearlyEqual(AT/(32.f*FrontlinePacingV24::BaseRecruitScale),.70f,.00001f));
    Test(TEXT("v48_flame_preparation_is_15_percent_shorter"),FMath::IsNearlyEqual(Flame/(29.f*FrontlinePacingV24::BaseRecruitScale),.85f,.00001f));
    bool Modifiers=true;
    for(int32 Team=0;Team<2;++Team)Modifiers&=FMath::IsNearlyEqual(B->EffectiveRecruitSeconds(EFrontlineRole::AntiTankRifle,Team)/AT,
        B->EffectiveRecruitSeconds(EFrontlineRole::Flamethrower,Team)/Flame,.00001f);
    Test(TEXT("v48_existing_faction_research_and_shared_preparation_modifiers_preserved"),Modifiers);
    J->SetNumberField(TEXT("anti_tank_base_seconds"),AT);J->SetNumberField(TEXT("flame_base_seconds"),Flame);
    J->SetBoolField(TEXT("success"),Good);return J;
}
