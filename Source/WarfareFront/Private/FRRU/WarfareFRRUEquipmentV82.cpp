#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUTextV82.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGameplayV43.h"
#include "Native/Expansion.h"
#include "Native/UE/ExpansionHostBridge.h"
#include "Native/UE/ExpansionArchiveUE.h"

namespace WarfareFRRUEquipmentV82
{
bool SupportAircraftYearAllowedV87(EFRRUNationV81 Nation,EFrontlineSupport Support,int32 CalendarDate)
{
    // The existing German visible airframe is a Halberstadt CL.II, not a
    // qualified 1916 substitute. Polish Aviation Museum: Year 1917; 1916 was
    // design work. This excludes known earlier years and makes no claim that
    // service began on 1 January 1917, nor changes later qualification rules.
    // https://muzeumlotnictwa.pl/en/exhibits/halberstadt-cl-ii-2/
    return Nation!=EFRRUNationV81::German
        ||(Support!=EFrontlineSupport::AirStrike&&Support!=EFrontlineSupport::AirBombing)
        ||CalendarDate/10000>=1917;
}
namespace
{
wfexp::Faction National(const AWarfareFrontlineGameMode* Battle,int32 Side)
{return static_cast<wfexp::Faction>(uint8(WarfareFRRUCampaignV82::Nation(Battle,Side)));}
const wfexp::Role* RoleFor(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Side)
{
    if(!Battle||!WarfareFRRUCampaignV82::Active(Battle)||(Side!=0&&Side!=1))return nullptr;
    const auto* Mission=wfexp::FindMission(wfexpue::Narrow(WarfareFRRUCampaignV82::MissionId(Battle)));
    if(!Mission||(Side==0&&!WarfareFRRUCampaignV82::ArchiveFullArmoryV87(Battle,Side)&&!wfexp::RoleAllowed(*Mission,int32(Role))))return nullptr;
    return wfexp::RoleFor(National(Battle,Side),int32(Role),WarfareFRRUCampaignV82::CalendarDate(Battle));
}
const wfexp::Support* SupportFor(const AWarfareFrontlineGameMode* Battle,EFrontlineSupport Support,int32 Side)
{
    if(!Battle||!WarfareFRRUCampaignV82::Active(Battle)||(Side!=0&&Side!=1))return nullptr;
    int32 Slot=-1;if(!ResolveSupport(Support,Slot))return nullptr;
    const auto* Mission=wfexp::FindMission(wfexpue::Narrow(WarfareFRRUCampaignV82::MissionId(Battle)));
    if(!Mission||(Side==0&&!WarfareFRRUCampaignV82::ArchiveFullArmoryV87(Battle,Side)&&!wfexp::SupportAllowed(*Mission,Slot)))return nullptr;
    const int32 Date=WarfareFRRUCampaignV82::CalendarDate(Battle);
    if(!SupportAircraftYearAllowedV87(WarfareFRRUCampaignV82::Nation(Battle,Side),Support,Date))return nullptr;
    return wfexp::SupportFor(National(Battle,Side),Slot,Date);
}
// Allocated records never move: FProfile's WeaponId refers to its own FString.
TMap<const wfexp::Weapon*,TUniquePtr<FFRRUEquipmentV82>> Equipment;
TMap<const wfexp::Role*,FString> RoleLabels;
TMap<const wfexp::Support*,FString> SupportLabels;
TMap<const wfexp::Support*,FString> SupportProse;
}
const FFRRUEquipmentV82* Resolve(const AWarfareFrontlineSoldier* Soldier)
{
    if(!Soldier||!Soldier->Battle||!WarfareFRRUCampaignV82::Active(Soldier->Battle))return nullptr;
    const auto* Role=RoleFor(Soldier->Battle,Soldier->Role,Soldier->Team);if(!Role)return nullptr;
    const int32 Member=Soldier->IsMachineGunner()?0:Soldier->MemberIndex;
    const auto* Weapon=wfexp::WeaponFor(National(Soldier->Battle,Soldier->Team),int32(Soldier->Role),Member,WarfareFRRUCampaignV82::CalendarDate(Soldier->Battle));
    if(!Weapon)return nullptr;
    if(const auto* Found=Equipment.Find(Weapon))return Found->Get();
    auto New=MakeUnique<FFRRUEquipmentV82>();New->WeaponId=wfexpue::Wide(Weapon->id);New->Capacity=Weapon->capacity;New->ReloadSeconds=Weapon->reloadSeconds;
    New->Automatic=Weapon->stats.roundIntervalSeconds>0&&Weapon->stats.burstMax>0;
    const auto& S=Weapon->stats;
    New->Profile={*New->WeaponId,S.fireRangeCm,S.advanceContactCm,S.hitProbabilityScale,S.directPressure,S.adjacentPressure,S.adjacentRadiusCm,
        S.accuracyNear,S.accuracyFloor,S.accuracyFalloffCm,S.aimedCycleSeconds,S.roundIntervalSeconds,S.burstMin,S.burstMax,S.recoveryMin,S.recoveryMax};
    const auto* Result=New.Get();Equipment.Add(Weapon,MoveTemp(New));return Result;
}
bool RoleAvailable(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Side){return RoleFor(B,Role,Side)!=nullptr;}
int32 RoleSize(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Side){const auto* R=RoleFor(B,Role,Side);return R?R->size:0;}
float ReadinessSeconds(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Side){const auto* R=RoleFor(B,Role,Side);return R?R->readinessSeconds:MAX_flt;}
const TCHAR* RoleName(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Side)
{
    const auto* R=RoleFor(B,Role,Side);if(!R)return TEXT("UNAVAILABLE");
    // Unit names are consistently English; body/tooltip prose follows locale.
    auto& Label=RoleLabels.FindOrAdd(R);if(Label.IsEmpty())Label=wfexpue::Wide(R->name.en);return *Label;
}
FString RoleDescription(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Side)
{const auto* R=RoleFor(B,Role,Side);return R?WarfareFRRUTextV82::Resolve(wfexpue::Wide(R->id),TEXT("description"),wfexpue::Wide(R->description.en),wfexpue::Wide(R->description.zh)):FString();}
int32 FirstMissionForRole(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Side)
{
    if(!B||!WarfareFRRUCampaignV82::Active(B)||(Side!=0&&Side!=1))return MAX_int32;
    const auto Nation=National(B,Side);int32 Result=MAX_int32;
    for(const auto& M:wfexp::Missions())if(M.faction==Nation&&wfexp::RoleAllowed(M,int32(Role))&&wfexp::RoleFor(Nation,int32(Role),M.date))Result=FMath::Min(Result,M.order);
    return Result;
}
bool ResolveSupport(EFrontlineSupport HostSupport,int32& CatalogSlot)
{
    // Explicit semantics, including the retained legacy AirRecon ordinal whose
    // actual V64 consumer is timed shrapnel. It never authorizes a recon task.
    switch(HostSupport)
    {
    case EFrontlineSupport::Artillery:CatalogSlot=0;return true;
    case EFrontlineSupport::Smoke:CatalogSlot=1;return true;
    case EFrontlineSupport::Rally:CatalogSlot=2;return true;
    case EFrontlineSupport::Mortar:CatalogSlot=3;return true;
    case EFrontlineSupport::Gas:CatalogSlot=4;return true;
    case EFrontlineSupport::AntiTank:CatalogSlot=5;return true;
    case EFrontlineSupport::AirStrike:CatalogSlot=6;return true;
    case EFrontlineSupport::CreepingBarrage:CatalogSlot=7;return true;
    case EFrontlineSupport::AirRecon:CatalogSlot=8;return true;
    case EFrontlineSupport::AirBombing:CatalogSlot=9;return true;
    default:return false;
    }
}
bool SupportAvailable(const AWarfareFrontlineGameMode* B,EFrontlineSupport Support,int32 Side){return SupportFor(B,Support,Side)!=nullptr;}
int32 FirstMissionForSupport(const AWarfareFrontlineGameMode* B,EFrontlineSupport Support,int32 Side)
{
    if(!B||!WarfareFRRUCampaignV82::Active(B)||(Side!=0&&Side!=1))return MAX_int32;int32 Slot=-1;if(!ResolveSupport(Support,Slot))return MAX_int32;
    const auto Nation=National(B,Side);int32 Result=MAX_int32;
    for(const auto& M:wfexp::Missions())if(M.faction==Nation
        &&SupportAircraftYearAllowedV87(WarfareFRRUCampaignV82::Nation(B,Side),Support,M.date)
        &&wfexp::SupportAllowed(M,Slot)&&wfexp::SupportFor(Nation,Slot,M.date))Result=FMath::Min(Result,M.order);
    return Result;
}
float SupportCooldown(const AWarfareFrontlineGameMode* B,EFrontlineSupport Support,int32 Side)
{
    const auto* S=SupportFor(B,Support,Side);if(!S)return MAX_flt;
    return Side==1?FrontlineGameplayV43::PreparationSeconds(S->cooldownSeconds,Side,B->GetCampaignDifficulty()):S->cooldownSeconds;
}
float SupportArrival(const AWarfareFrontlineGameMode* B,EFrontlineSupport Support,int32 Side){const auto* S=SupportFor(B,Support,Side);return S?S->arrivalSeconds:MAX_flt;}
const TCHAR* SupportName(const AWarfareFrontlineGameMode* B,EFrontlineSupport Support,int32 Side)
{const auto* S=SupportFor(B,Support,Side);if(!S)return TEXT("UNAVAILABLE");auto& Label=SupportLabels.FindOrAdd(S);if(Label.IsEmpty())Label=wfexpue::Wide(S->name.en);return *Label;}
const TCHAR* SupportDescription(const AWarfareFrontlineGameMode* B,EFrontlineSupport Support,int32 Side)
{const auto* S=SupportFor(B,Support,Side);if(!S)return TEXT("");auto& Prose=SupportProse.FindOrAdd(S);Prose=WarfareFRRUTextV82::Resolve(wfexpue::Wide(S->id),TEXT("description"),wfexpue::Wide(S->description.en),wfexpue::Wide(S->description.zh));return *Prose;}
}
