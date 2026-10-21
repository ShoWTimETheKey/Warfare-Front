#pragma once
#include "CoreMinimal.h"

// The same projection controls drawing, left-click regions and repeat orders.
namespace FrontlineConsoleLayoutV79
{
constexpr float InfantryX=24.f,ArmourX=728.f,CardPitch=136.f;
constexpr float CardWidth=128.f,CardHeight=180.f,CardTop=36.f;
inline bool ContainsCard(const FVector2D& Pointer,float GroupX,int32 Slot,float Dock)
{
    const FVector2D Origin(GroupX+Slot*CardPitch,Dock+CardTop);
    return Pointer.X>=Origin.X&&Pointer.X<Origin.X+CardWidth
        &&Pointer.Y>=Origin.Y&&Pointer.Y<Origin.Y+CardHeight;
}
}
