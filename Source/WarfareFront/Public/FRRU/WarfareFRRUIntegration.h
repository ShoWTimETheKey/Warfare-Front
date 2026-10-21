#pragma once
#include "CoreMinimal.h"
class FJsonObject;

// Nation is a registry identity. Only Side is permitted to index team arrays.
enum class EFRRUNationV81 : uint8 {British=0,German=1,French=2,Russian=3,AustroHungarian=4,Invalid=255};
enum class EFRRUTechModeV81 : uint8 {TacticalV2=2};
struct FFRRUEncounterDescriptorV81
{
    FString StableMissionId,SceneId;
    uint32 SceneRevision=0;
    int32 CalendarDate=0,PlayerSide=INDEX_NONE;
    bool bEasternTheatre=false;
    EFRRUNationV81 Nations[2]={EFRRUNationV81::Invalid,EFRRUNationV81::Invalid};
    int32 Directions[2]={0,0};
    EFRRUTechModeV81 TechnologyMode=EFRRUTechModeV81::TacticalV2;
};
namespace WarfareFRRUPilotV81
{
    WARFAREFRONT_API bool IsGameplayEnabled();
    WARFAREFRONT_API bool ResolveEncounter(const FString& MissionId,int32 PlayerSide,int32 PlayerDirection,FFRRUEncounterDescriptorV81& Output,FString& Error);
    // Pure compatibility fixtures; never uses or writes player saves/actors.
    WARFAREFRONT_API TSharedRef<FJsonObject> RunCompatibilityChecks();
}
