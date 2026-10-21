#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
namespace FrontlineGameplayV49
{
float BreakthroughLine();
bool InEndSector(const AWarfareFrontlineGameMode* Battle,int32 Team,const FVector& Position);
bool ShareRearSector(const AWarfareFrontlineGameMode* Battle,int32 Team,const FVector& A,const FVector& B);
bool EnemyEndDefended(const AWarfareFrontlineGameMode* Battle,int32 Team);
}
