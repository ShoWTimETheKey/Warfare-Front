#pragma once

#include "CoreMinimal.h"

enum class EFrontlineMusicCueV31 : uint8 { Vigil, Contact, Barrage, Victory, Defeat };

/** Only observable battlefield facts enter the director; no recruit or support commands. */
struct FFrontlineMusicObservationV31
{
    uint8 Phase=0; // Briefing, Fighting, Victory, Defeat, matching EFrontlinePhase.
    float ShotsPerSecond=0.f, BlastsPerSecond=0.f, CasualtiesPerSecond=0.f;
    float EngagedDensity=0.f, SuppressedFraction=0.f, MoralePressure=0.f;
};

/** Same scheduler used by the actual audio subsystem and focused deterministic checks. */
struct FFrontlineMusicDirectorV31
{
    // V828: 120 BPM / four beats, one common bar across every national cue.
    static constexpr float LoopSeconds=64.f, PhraseSeconds=8.f, CrossfadeSeconds=3.5f;
    EFrontlineMusicCueV31 Current=EFrontlineMusicCueV31::Vigil;
    EFrontlineMusicCueV31 Previous=EFrontlineMusicCueV31::Vigil;
    EFrontlineMusicCueV31 Candidate=EFrontlineMusicCueV31::Vigil;
    float Tension=0.f, CandidateAge=0.f, HoldAge=0.f, FadeAge=CrossfadeSeconds;
    double Clock=0.;
    uint32 Generation=0;
    uint8 LastPhase=0;
    void Advance(float DeltaSeconds,const FFrontlineMusicObservationV31& Observation);
    float CurrentWeight() const;
    float PreviousWeight() const;
    bool IsTransitioning() const { return FadeAge<CrossfadeSeconds; }
    static float Score(const FFrontlineMusicObservationV31& Observation);
    static EFrontlineMusicCueV31 Choose(float Score,EFrontlineMusicCueV31 Current,uint8 Phase);
    static const TCHAR* AssetName(EFrontlineMusicCueV31 Cue);
    static FName NationalAssetNameV828(EFrontlineMusicCueV31 Cue,int32 Nation);
    static bool IsResult(EFrontlineMusicCueV31 Cue) { return Cue==EFrontlineMusicCueV31::Victory || Cue==EFrontlineMusicCueV31::Defeat; }
};

void RunFrontlineMusicV31PolicyChecks(TFunctionRef<void(const FString&,bool)> Check);
void RunFrontlineMusicV828PolicyChecks(TFunctionRef<void(const FString&,bool)> Check);
