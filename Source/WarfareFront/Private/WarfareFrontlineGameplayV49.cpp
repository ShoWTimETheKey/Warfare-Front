#include "WarfareFrontlineGameplayV49.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlinePacingV24.h"

float FrontlineGameplayV49::BreakthroughLine()
{return FrontlineBattlefield::GoalHalfLength+300.f;}

bool FrontlineGameplayV49::InEndSector(const AWarfareFrontlineGameMode* B,int32 Team,const FVector& P)
{
    if(!B||Team<0||Team>1||P.ContainsNaN())return false;
    const int32 Home=B->GetHomeTrenchIndex(1-Team);if(!B->Trenches.IsValidIndex(Home))return false;
    const float Dir=B->GetTeamDirection(Team),Along=P.X*Dir;
    // Use the live operation's last defensive line, on either side of the map.
    // This is a rear-sector objective, not a requirement to eliminate the map.
    return Along>=B->Trenches[Home].X*Dir-400.f&&Along<=FrontlinePacingV24::CombatHalfLength()+650.f
        &&FMath::Abs(P.Y)<=FrontlineBattlefield::HalfFrontage+200.f;
}

bool FrontlineGameplayV49::EnemyEndDefended(const AWarfareFrontlineGameMode* B,int32 Team)
{
    if(!B||Team<0||Team>1)return true;
    for(const AWarfareFrontlineSoldier* U:B->Units)
        if(IsValid(U)&&U->Team==1-Team&&U->IsActiveCombatant()&&InEndSector(B,Team,U->Feet()))return true;
    if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
        for(const AFrontlineTank* T:Fleet->GetVehicles())
            if(IsValid(T)&&T->Team==1-Team&&T->IsOperational()&&T->HasEnteredField()
                &&InEndSector(B,Team,T->GetActorLocation()))return true;
    return false;
}

bool FrontlineGameplayV49::ShareRearSector(const AWarfareFrontlineGameMode* Battle,int32 Team,const FVector& A,const FVector& B)
{
    return (InEndSector(Battle,Team,A)&&InEndSector(Battle,Team,B))
        ||(InEndSector(Battle,1-Team,A)&&InEndSector(Battle,1-Team,B));
}
