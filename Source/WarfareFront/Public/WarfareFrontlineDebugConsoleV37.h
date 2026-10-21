#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class AWarfareFrontlineController;
class AHUD;
class UCanvas;

namespace FrontlineDebugV37
{
void Initialize(AWarfareFrontlineController* Controller);
void Shutdown(AWarfareFrontlineController* Controller);
bool IsOpen(const AWarfareFrontlineController* Controller);
void Draw(AHUD* HUD,UCanvas* Canvas);
void Tick(AWarfareFrontlineGameMode* Battle);
bool HasCheatState(const AWarfareFrontlineGameMode* Battle);
bool UnlimitedResearch(const AWarfareFrontlineGameMode* Battle);
bool Execute(AWarfareFrontlineGameMode* Battle,const FString& Command,FString& Response);
// Isolated review access: never invoked by ordinary pages or user-profile tests.
void SetReviewOpen(AWarfareFrontlineController* Controller,bool Open);
bool SubmitReviewCommand(AWarfareFrontlineController* Controller,const FString& Command);
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
void RunSpeedChecksV836(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
