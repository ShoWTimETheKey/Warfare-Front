#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class FJsonObject;
namespace FrontlineFrontsV76
{
struct FReadout{float Advance[2]={0,0},AdvanceDisplay[2]={0,0},Morale[2]={1,1},MoraleDisplay[2]={1,1},MoraleGhost[2]={1,1};bool MoraleGhostActive[2]={false,false};};
float AdvanceFraction(const AWarfareFrontlineGameMode* Battle,int32 Team);
FReadout Read(AWarfareFrontlineGameMode* Battle,float RealSeconds);
// Does not sample or advance the presentation interpolation.
bool Peek(const AWarfareFrontlineGameMode* Battle,FReadout& Out);
void Reset(AWarfareFrontlineGameMode* Battle);
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check,TSharedPtr<FJsonObject>* Diagnostics=nullptr);
}
