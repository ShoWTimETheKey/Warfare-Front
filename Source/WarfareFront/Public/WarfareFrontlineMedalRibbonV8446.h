#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineMedalsV8215.h"

/** Display-only projection. A catalogue class does not grant an award and a
 * ribbon-only device is not necessarily the attachment on the full medal. */
namespace FrontlineMedalRibbonV8446
{
enum class EContext:uint8 {SelectedClass,EarnedRack};
enum class EDevice:uint8 {Bow,Rosette,RosetteBacking,MiniatureCross,OakLeaves,Star,Palm,Swords,Crown,Clasp};
enum class EMetal:uint8 {Unspecified,Silver,Gold,Bronze,RedTextile,SilverAndGold};
struct FDevice
{
    EDevice Kind=EDevice::Rosette;
    EMetal Metal=EMetal::Unspecified;
    int32 Count=1;
};
struct FPresentation
{
    /** The class whose service textile is represented, or zero if no
     * corresponding breast ribbon is supported by the catalogue.
     * EarnedRack may retain a previously earned class. */
    int32 Grade=0;
    TArray<FDevice> Devices;
    bool HasRibbon() const {return Grade>0;}
};

/** Pure: all account information is supplied by the caller. SelectedClass
 * chooses the inspected class; EarnedRack chooses the highest earned class.
 * Recorded repeat/citation devices apply only to that exact recorded award.
 * Class-prescribed devices can be previewed before the class is earned. */
FPresentation Presentation(int32 Index,int32 Grade,EContext Context,
    const FrontlineMedalsV8215::FAward& RecordedAward=FrontlineMedalsV8215::FAward{});
void RunChecks(TFunctionRef<void(const FString&,bool)> Check);
}
