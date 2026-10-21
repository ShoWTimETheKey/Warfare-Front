#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineAudio.h"
class UWorld;

// A real nearby witness issues a warning, never an off-map command source.
void WarnFrontlineSoldiers(UWorld* World,EFrontlineVoiceEvent Event,const FVector& Threat,float Radius,int32 WitnessTeam=INDEX_NONE);
void StartFrontlineV20Review(AWarfareFrontlineController* Controller);
