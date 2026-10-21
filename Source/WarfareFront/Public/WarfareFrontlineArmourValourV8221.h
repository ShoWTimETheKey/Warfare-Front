#pragma once
#include "CoreMinimal.h"
#include "FRRU/WarfareFRRUIntegration.h"

/** Presentation of the original infantry-armour deed record. It never grants
 * or changes the historical decorations in FrontlineMedalsV8215. */
namespace FrontlineArmourValourV8221
{
enum class EDevice : uint8 {None,SilverRosettes,IronCrossPin,CitationDevices,GeorgeClass};
struct FDisplay
{
    EFRRUNationV81 Nation=EFRRUNationV81::Invalid;
    int32 Total=0,Distinctions=0,NextAt=1;
    EDevice Device=EDevice::None;
    bool Available=false;
};
/** Pure, bounded projection. Count thresholds are gameplay qualifications. */
FDisplay Display(EFRRUNationV81 Nation,int32 ConfirmedKills);
FString BannerTitle(bool Chinese);
FString RibbonLabel(const FDisplay& Display,bool Chinese);
FString Description(const FDisplay& Display,bool Chinese);
FString BadgePath(EFRRUNationV81 Nation);
FString RibbonPath(EFRRUNationV81 Nation);
/** Opaque authored weave, excluding alpha canvas. Four nations share 4:1. */
FIntRect RibbonTextileCutV837(EFRRUNationV81 Nation);
FIntPoint RibbonSourceSizeV837(EFRRUNationV81 Nation);
TArray<FString> AssetPaths();
void RunChecks(TFunctionRef<void(const FString&,bool)> Check);
}
