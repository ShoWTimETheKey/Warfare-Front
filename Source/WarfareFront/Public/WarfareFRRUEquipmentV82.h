#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineWeaponProfile.h"

class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
enum class EFrontlineRole : uint8;
enum class EFrontlineSupport : uint8;
enum class EFRRUNationV81 : uint8;

struct FFRRUEquipmentV82
{
    FString WeaponId;
    int32 Capacity=0;
    float ReloadSeconds=0.f;
    bool Automatic=false;
    FrontlineWeapons::FProfile Profile={};
};

// Descriptor resolution is not asset/deployment acceptance. An unavailable
// national role or dated weapon never falls through to a UK/DE service rifle.
namespace WarfareFRRUEquipmentV82
{
    WARFAREFRONT_API const FFRRUEquipmentV82* Resolve(const AWarfareFrontlineSoldier* Soldier);
    WARFAREFRONT_API bool RoleAvailable(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Side);
    WARFAREFRONT_API int32 RoleSize(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Side);
    WARFAREFRONT_API float ReadinessSeconds(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Side);
    WARFAREFRONT_API const TCHAR* RoleName(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Side);
    WARFAREFRONT_API FString RoleDescription(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Side);
    WARFAREFRONT_API int32 FirstMissionForRole(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Side);
    WARFAREFRONT_API bool ResolveSupport(EFrontlineSupport HostSupport,int32& CatalogSlot);
    // FR/RU-only negative year filter for the retained Halberstadt CL.II.
    // True preserves all other qualifications; it is not a first-service date.
    WARFAREFRONT_API bool SupportAircraftYearAllowedV87(EFRRUNationV81 Nation,EFrontlineSupport Support,int32 CalendarDate);
    WARFAREFRONT_API bool SupportAvailable(const AWarfareFrontlineGameMode* Battle,EFrontlineSupport Support,int32 Side);
    WARFAREFRONT_API int32 FirstMissionForSupport(const AWarfareFrontlineGameMode* Battle,EFrontlineSupport Support,int32 Side);
    WARFAREFRONT_API float SupportCooldown(const AWarfareFrontlineGameMode* Battle,EFrontlineSupport Support,int32 Side);
    WARFAREFRONT_API float SupportArrival(const AWarfareFrontlineGameMode* Battle,EFrontlineSupport Support,int32 Side);
    WARFAREFRONT_API const TCHAR* SupportName(const AWarfareFrontlineGameMode* Battle,EFrontlineSupport Support,int32 Side);
    WARFAREFRONT_API const TCHAR* SupportDescription(const AWarfareFrontlineGameMode* Battle,EFrontlineSupport Support,int32 Side);
}
