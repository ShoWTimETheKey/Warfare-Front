#pragma once
#include "WarfareFrontline.h"
#include "WarfareFrontlineVehiclesV28.h"

// Narrow actor identity access shared by the fatal-hit route and disposable review.
struct FFrontlineArmourKillTallyV8220Review
{
    static AWarfareFrontlineGameMode* Owner(const AFrontlineTank* Tank)
    {return Tank?Tank->Battle.Get():nullptr;}
    static void Enter(AFrontlineTank* Tank)
    {if(Tank)Tank->bEntered=true;}
};
