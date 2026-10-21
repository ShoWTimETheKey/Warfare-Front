#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineSpecialistsV41.h"

bool FrontlineUseLegacyCombatV835();
bool FrontlineUseLegacyBalanceV8447();

namespace FrontlineWeapons
{
namespace
{
// Id, fire/contact cm, hit probability weight/pressure/splash/radius, accuracy near/floor/falloff,
// aimed-cycle floor, automatic round interval, burst size, burst recovery.
// V26 preserves the old expected lethality budget (former damage / 100) as
// probability. Every successful roll kills; failed rolls never accumulate.
constexpr FProfile Flame={TEXT("PressurisedFlameLance"),1450,1100,1,.8f,.4f,250,.95f,.90f,10000,4.55f,0,0,0,0,0};
constexpr FProfile RifleGrenade={TEXT("SMLE_No23_RifleGrenade"),3600,3100,.25f,.18f,.05f,220,.78f,.27f,15000,7.f,0,0,0,0,0};
constexpr FProfile AntiTank={TEXT("Tankgewehr_M1918"),7000,5600,.48f,.24f,.03f,180,.88f,.36f,24000,6.4f,0,0,0,0,0};
constexpr FProfile BritishRifle={TEXT("SMLE"),4100,2500,.25f,.12f,.025f,180,.76f,.16f,6500,3.05f,0,0,0,0,0};
constexpr FProfile GermanRifle={TEXT("Gewehr98"),4300,2600,.25f,.12f,.025f,180,.76f,.16f,6500,3.30f,0,0,0,0,0};
// V46 favours accurate established positions over long-range map denial.
// Apply only to actual gun carriers; assistants retain their service rifles.
constexpr FProfile BritishGun={TEXT("Lewis"),5450*.85f,4700*.85f,.072f*1.15f*1.25f,.55f,.16f,540,.74f,.29f,18000,0,.100f,8,11,2.6f,3.8f};
constexpr FProfile GermanGun={TEXT("MG0815"),5800*.85f,4850*.85f,.084f*1.15f*1.25f,.60f,.175f,560,.74f,.29f,18000,0,.120f,10,14,3.0f,4.3f};
constexpr FProfile BritishMobileGun={TEXT("Lewis_AssaultSection"),3500,2650,.072f*1.15f*1.25f,.43f,.12f,420,.76f,.28f,15000,0,.100f,6,9,2.2f,3.2f};
constexpr FProfile GermanMobileGun={TEXT("MG0815_BegleitSection"),3750,2850,.084f*1.15f*1.25f,.48f,.14f,450,.74f,.27f,15000,0,.120f,8,11,2.6f,3.7f};
constexpr FProfile BritishMarksman={TEXT("MarksmanSMLE"),6400,4500,.70f,.17f,.025f,180,.91f,.53f,28000,4.8f,0,0,0,0,0};
// A three-man Jäger patrol replaces the isolated German marksman slot. Veteran
// Gewehr98 fire is strongest at working rifle range, without a sniper's reach.
constexpr FProfile GermanJaeger={TEXT("Jaeger_Gewehr98"),5500,3900,.58f,.24f,.060f,280,.93f,.43f,21000,3.15f,0,0,0,0,0};
constexpr FProfile BritishAssault={TEXT("MillsSMLE"),3400,1155,.25f,.12f,.025f,180,.76f,.16f,6500,3.05f,0,0,0,0,0};
constexpr FProfile GermanAssault={TEXT("AssaultGewehr98"),3500,1155,.25f,.12f,.025f,180,.76f,.16f,6500,3.30f,0,0,0,0,0};
constexpr FProfile BritishCommand={TEXT("CommandSMLE"),4100,2100,.25f,.12f,.025f,180,.76f,.16f,6500,3.05f,0,0,0,0,0};
constexpr FProfile GermanCommand={TEXT("CommandGewehr98"),4300,2200,.25f,.12f,.025f,180,.76f,.16f,6500,3.30f,0,0,0,0,0};
constexpr FProfile BritishSniper={TEXT("ScopedSMLE"),8500,5700,.90f,.21f,.025f,180,.95f,.66f,38000,5.4f,0,0,0,0,0};
// MP 18: approximately 400 rounds/minute within short bursts; its pistol
// cartridge has a shorter useful game range than either service rifle.
constexpr FProfile GermanStormLegacyV8446={TEXT("MP18"),2800,1386,.14f,.23f,.075f,230,.77f,.23f,8500,0,.150f,4,7,.95f,1.55f};
constexpr FProfile GermanStorm={TEXT("MP18"),2800,1386,.16f,.23f,.075f,230,.81f,.23f,10000,0,.150f,4,7,.85f,1.40f};
constexpr FProfile BritishOfficer={TEXT("WebleyMkVI"),1900,1650,.30f,.15f,.015f,120,.82f,.35f,10000,1.5f,0,0,0,0,0};
constexpr FProfile GermanOfficer={TEXT("P08"),2100,1250,.23f,.14f,.015f,120,.84f,.35f,11000,1.05f,0,0,0,0,0};
}

float FirearmHitChance(float BaseProbability,bool bAutomatic)
{
    if(!FMath::IsFinite(BaseProbability))return 0.f;
    return FMath::Clamp(BaseProbability*(bAutomatic?1.35f:1.20f),0.f,1.f);
}

float DirectFireRangeDistanceCm(const AWarfareFrontlineSoldier* Soldier,const FVector& Point)
{
    if(!Soldier||Point.ContainsNaN()||Soldier->Feet().ContainsNaN())return MAX_flt;
    // The battlefield's lateral ranks share a longitudinal engagement limit.
    // World traces and accuracy still use their actual physical endpoints and
    // distance; a trench wall, hill or smoke bank remains authoritative.
    // Jets and launched specialist ammunition retain their physical reach.
    if(FrontlineUseLegacyCombatV835()||WarfareFRRUCampaignV82::Active(Soldier->Battle)
        ||FrontlineFieldV32::IsFlameOperator(Soldier)||FrontlineSpecialistsV41::IsSpecialist(Soldier))
        return FVector::Dist2D(Soldier->Feet(),Point);
    return FMath::Abs(Soldier->Feet().X-Point.X);
}

const FProfile& ProfileFor(const AWarfareFrontlineSoldier* Soldier)
{
    if(Soldier&&WarfareFRRUCampaignV82::Active(Soldier->Battle))
    {
        if(const auto* Equipment=WarfareFRRUEquipmentV82::Resolve(Soldier))return Equipment->Profile;
        static constexpr FProfile Unavailable={TEXT("UnavailableNationalWeapon"),0,0,0,0,0,0,0,0,1,MAX_flt,0,0,0,0,0};
        return Unavailable;
    }
    if(FrontlineFieldV32::IsFlameOperator(Soldier))return Flame;
    if(FrontlineSpecialistsV41::IsRifleGrenadier(Soldier))return RifleGrenade;
    if(FrontlineSpecialistsV41::IsAntiTankGunner(Soldier))return AntiTank;
    const bool British=!Soldier || Soldier->GetFaction()==EFrontlineFaction::British;
    if (Soldier && Soldier->IsCommissionedOfficer()) return British?BritishOfficer:GermanOfficer;
    if (Soldier && Soldier->IsStormtrooper()) return FrontlineUseLegacyBalanceV8447()?GermanStormLegacyV8446:GermanStorm;
    if (Soldier && Soldier->IsScopedMarksman()) return BritishSniper;
    if (Soldier && Soldier->IsMachineGunner()) return Soldier->Role==EFrontlineRole::MobileGun?(British?BritishMobileGun:GermanMobileGun):(British?BritishGun:GermanGun);
    if (Soldier && Soldier->Role==EFrontlineRole::Marksman && !British)return GermanJaeger;
    if (Soldier && Soldier->Role==EFrontlineRole::Marksman && Soldier->MemberIndex==0) return BritishMarksman;
    if (Soldier && Soldier->Role==EFrontlineRole::Mills) return British?BritishAssault:GermanAssault;
    if (Soldier && Soldier->Role==EFrontlineRole::Officer) return British?BritishCommand:GermanCommand;
    // Role Lewis includes two rifle-armed ammunition bearers; never grant them MG fire.
    return British?BritishRifle:GermanRifle;
}

float AdvanceContactRangeFor(const AWarfareFrontlineSoldier* Soldier)
{
    if(Soldier&&WarfareFRRUCampaignV82::Active(Soldier->Battle))
    {
        // The rifle remains the national weapon, but a hand-bomb section closes
        // to the same assault contact radius before stopping to fight.
        if(Soldier->Role==EFrontlineRole::Mills&&!Soldier->IsStormtrooper())
            return FMath::Min(ProfileFor(Soldier).AdvanceContactCm,BritishAssault.AdvanceContactCm);
        if(Soldier->Role==EFrontlineRole::Lewis||Soldier->Role==EFrontlineRole::MobileGun)
            if(const auto* Section=Soldier->Battle->FindSquad(Soldier->SquadId))for(const auto& Member:Section->Members)
                if(const auto* Gun=Member.Get();Gun&&Gun->IsAlive()&&Gun->IsMachineGunner())return ProfileFor(Gun).AdvanceContactCm;
        return ProfileFor(Soldier).AdvanceContactCm;
    }
    if(Soldier&&Soldier->Role==EFrontlineRole::Signals)return 4400.f;
    if(Soldier&&Soldier->Role==EFrontlineRole::Marksman&&Soldier->MemberIndex!=0&&Soldier->Battle)
        if(const auto* S=Soldier->Battle->FindSquad(Soldier->SquadId))for(const auto& W:S->Members)
            if(const auto* Lead=W.Get();Lead&&Lead->MemberIndex==0&&Lead->IsAlive())return ProfileFor(Lead).AdvanceContactCm;
    if(Soldier&&Soldier->Role==EFrontlineRole::Flamethrower)return 1100.f;
    // Close only for an uncleared obstacle on this man's immediate route.
    // Empty or cleared approaches retain the service-rifle contact screen.
    if(Soldier&&Soldier->Role==EFrontlineRole::Pioneer)return FrontlineGameplayV52::PioneerContactRange(Soldier);
    // Commanders do not lead with their sidearm's range. Rifle escorts establish
    // the ordinary screen and the leader halts two metres behind it. Discharge
    // still uses ProfileFor().FireRangeCm, so this grants no extra pistol reach.
    if(Soldier&&Soldier->Role==EFrontlineRole::Officer)
        return (Soldier->GetFaction()==EFrontlineFaction::British?BritishRifle:GermanRifle).AdvanceContactCm+(Soldier->IsSectionLeader()?200.f:0.f);
    if (Soldier && IsValid(Soldier->Battle.Get()) && (Soldier->Role==EFrontlineRole::Lewis||Soldier->Role==EFrontlineRole::MobileGun||Soldier->Role==EFrontlineRole::RifleGrenadier||Soldier->Role==EFrontlineRole::AntiTankRifle) && !Soldier->IsAutomaticWeapon()&&!FrontlineSpecialistsV41::IsSpecialist(Soldier))
        if (const FFrontlineSquad* Squad=Soldier->Battle->FindSquad(Soldier->SquadId))
            for (const auto& Member:Squad->Members)
                if (const AWarfareFrontlineSoldier* Other=Member.Get(); Other && Other->IsAlive() && Other->Team==Soldier->Team && (Other->IsAutomaticWeapon()||FrontlineSpecialistsV41::IsSpecialist(Other)))
                    return ProfileFor(Other).AdvanceContactCm;
    return ProfileFor(Soldier).AdvanceContactCm;
}
float AdvanceArmourContactRangeForV8310(const AWarfareFrontlineSoldier* Soldier)
{
    if(FrontlineSpecialistsV41::HasAntiTankRifleGrenadeV836(Soldier))return FrontlineSpecialistsV41::AntiTankRifleGrenadeContactCmV8310;
    if(Soldier&&Soldier->Role==EFrontlineRole::RifleGrenadier&&Soldier->Battle)
        if(const auto* Squad=Soldier->Battle->FindSquad(Soldier->SquadId))for(const auto& W:Squad->Members)
            if(const auto* Lead=W.Get();Lead&&Lead->IsActiveCombatant()&&Lead->Team==Soldier->Team&&FrontlineSpecialistsV41::HasAntiTankRifleGrenadeV836(Lead))
                return FrontlineSpecialistsV41::AntiTankRifleGrenadeContactCmV8310;
    return AdvanceContactRangeFor(Soldier);
}
}

