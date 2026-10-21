#pragma once
#include "CoreMinimal.h"

/** Contact provenance only; cosmetic audio never changes damage. */
enum class EFrontlineArmourContactSoundV839 : uint8 { Automatic, Rifle, Grenade, Cannon };
namespace FrontlineArmourAudioV839
{
    inline EFrontlineArmourContactSoundV839 Resolve(EFrontlineArmourContactSoundV839 Kind,bool Explosive)
    {return Kind==EFrontlineArmourContactSoundV839::Automatic?(Explosive?EFrontlineArmourContactSoundV839::Grenade:EFrontlineArmourContactSoundV839::Rifle):Kind;}
    inline const TCHAR* Family(EFrontlineArmourContactSoundV839 Kind)
    {return Kind==EFrontlineArmourContactSoundV839::Cannon?TEXT("Cannon"):Kind==EFrontlineArmourContactSoundV839::Grenade?TEXT("Grenade"):TEXT("Rifle");}
}
