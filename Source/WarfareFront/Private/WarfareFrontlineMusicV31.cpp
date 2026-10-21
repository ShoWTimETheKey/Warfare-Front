#include "WarfareFrontlineMusicV31.h"

float FFrontlineMusicDirectorV31::Score(const FFrontlineMusicObservationV31& O)

{

    if(O.Phase!=1)return 0.f;

    auto Unit=[](float X){return FMath::IsFinite(X)?FMath::Clamp(X,0.f,1.f):0.f;};

    // Density, reports and fresh losses describe the present fighting. Morale

    // adds sustained peril without keeping an empty battlefield at full intensity.

    return Unit(.30f*Unit(O.ShotsPerSecond/10.f)+.24f*Unit(O.BlastsPerSecond/1.4f)

        +.20f*Unit(O.CasualtiesPerSecond/.8f)+.16f*Unit(O.EngagedDensity)

        +.06f*Unit(O.SuppressedFraction)+.04f*Unit(O.MoralePressure));

}



EFrontlineMusicCueV31 FFrontlineMusicDirectorV31::Choose(float Value,EFrontlineMusicCueV31 Cue,uint8 Phase)

{

    if(Phase==2)return EFrontlineMusicCueV31::Victory;

    if(Phase==3)return EFrontlineMusicCueV31::Defeat;

    if(Phase!=1)return EFrontlineMusicCueV31::Vigil;

    if(Cue==EFrontlineMusicCueV31::Barrage && Value>=.39f)return EFrontlineMusicCueV31::Barrage;

    if(Value>=.57f)return EFrontlineMusicCueV31::Barrage;

    if(Cue==EFrontlineMusicCueV31::Contact && Value>=.14f)return EFrontlineMusicCueV31::Contact;

    return Value>=.23f?EFrontlineMusicCueV31::Contact:EFrontlineMusicCueV31::Vigil;

}



void FFrontlineMusicDirectorV31::Advance(float DeltaSeconds,const FFrontlineMusicObservationV31& O)

{

    // Audio keeps playing during a render hitch. Do not lose wall-clock time

    // here, or the next cue would seek into a different harmonic phrase.

    const float DT=FMath::IsFinite(DeltaSeconds)?FMath::Clamp(DeltaSeconds,0.f,3600.f):0.f;

    const double Before=Clock;Clock+=DT;HoldAge+=DT;

    FadeAge=FMath::Min(CrossfadeSeconds,FadeAge+DT);

    const float Target=Score(O);

    const float Follow=1.f-FMath::Exp(-DT/(Target>Tension?2.5f:8.f));

    Tension=FMath::Lerp(Tension,Target,Follow);

    const bool PhaseChanged=O.Phase!=LastPhase;

    LastPhase=O.Phase;

    if(PhaseChanged && O.Phase!=1)Tension=0.f;

    const auto Wanted=Choose(Tension,Current,O.Phase);

    if(Wanted!=Candidate){Candidate=Wanted;CandidateAge=0.f;}

    else CandidateAge+=DT;

    const bool ResultOrMenu=O.Phase!=1;

    const float Cadence=ResultOrMenu?1.f:PhraseSeconds;

    const bool Boundary=FMath::FloorToInt(Before/Cadence)!=FMath::FloorToInt(Clock/Cadence);

    const bool Confirmed=ResultOrMenu || (CandidateAge>=3.f && HoldAge>=16.f);

    if(Wanted!=Current && Confirmed && Boundary && !IsTransitioning())

    {

        Previous=Current;Current=Wanted;HoldAge=0.f;FadeAge=0.f;++Generation;

    }

}



FName FFrontlineMusicDirectorV31::NationalAssetNameV828(EFrontlineMusicCueV31 Cue,int32 Nation)
{
    return FName(AssetName(Cue));
}

float FFrontlineMusicDirectorV31::CurrentWeight() const
{return FMath::Sin(FMath::Clamp(FadeAge/CrossfadeSeconds,0.f,1.f)*HALF_PI);}
float FFrontlineMusicDirectorV31::PreviousWeight() const
{return IsTransitioning()?FMath::Cos(FMath::Clamp(FadeAge/CrossfadeSeconds,0.f,1.f)*HALF_PI):0.f;}

const TCHAR* FFrontlineMusicDirectorV31::AssetName(EFrontlineMusicCueV31 Cue)
{
    switch(Cue)
    {
    case EFrontlineMusicCueV31::Contact:return TEXT("S_Music_Contact_V31");
    case EFrontlineMusicCueV31::Barrage:return TEXT("S_Music_Barrage_V31");
    case EFrontlineMusicCueV31::Victory:return TEXT("S_Music_Victory_V46");
    case EFrontlineMusicCueV31::Defeat:return TEXT("S_Music_Defeat_V31");
    default:return TEXT("S_Music_Vigil_V31");
    }
}

void RunFrontlineMusicV31PolicyChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    using Cue=EFrontlineMusicCueV31;
    FFrontlineMusicDirectorV31 D;
    FFrontlineMusicObservationV31 Quiet;Quiet.Phase=1;
    auto Simulate=[&D](float Seconds,const FFrontlineMusicObservationV31& O)
    {for(int32 I=0;I<FMath::RoundToInt(Seconds*20);++I)D.Advance(.05f,O);};
    Simulate(20,Quiet);Check(TEXT("music_empty_battle_stays_restrained"),D.Current==Cue::Vigil);
    FFrontlineMusicObservationV31 Contact=Quiet;Contact.ShotsPerSecond=5;Contact.EngagedDensity=.75f;
    Simulate(30,Contact);Check(TEXT("music_actual_contact_changes_orchestration"),D.Current==Cue::Contact);
    FFrontlineMusicObservationV31 Heavy=Contact;Heavy.ShotsPerSecond=15;Heavy.BlastsPerSecond=2;
    Heavy.CasualtiesPerSecond=1.2;Heavy.SuppressedFraction=.65f;Heavy.MoralePressure=.7f;
    const uint32 Before=D.Generation;Simulate(1,Heavy);
    Check(TEXT("music_one_shock_does_not_restart_cue"),D.Generation==Before);
    Simulate(30,Heavy);Check(TEXT("music_sustained_barrage_changes_timbre_and_rhythm"),D.Current==Cue::Barrage);
    Simulate(5,Quiet);Check(TEXT("music_aftershock_hysteresis_prevents_chatter"),D.Current==Cue::Barrage);
    Simulate(65,Quiet);Check(TEXT("music_silence_eventually_restores_sparse_arrangement"),D.Current==Cue::Vigil);
    bool UnitPower=true,Continuous=true;float Prior=0;
    D.Previous=Cue::Barrage;D.Current=Cue::Victory;D.FadeAge=0;
    for(int32 I=0;I<=70;++I)
    {
        D.FadeAge=I*.05f;
        const float A=D.CurrentWeight(),B=D.PreviousWeight();
        UnitPower&=FMath::IsNearlyEqual(A*A+B*B,1.f,.0001f);
        Continuous&=FMath::Abs(A-Prior)<.06f;Prior=A;
    }
    Check(TEXT("music_crossfade_is_continuous_equal_power"),UnitPower&&Continuous);
    D=FFrontlineMusicDirectorV31();FFrontlineMusicObservationV31 Result;Result.Phase=2;Simulate(1.5,Result);
    Check(TEXT("music_victory_owns_distinct_cue_without_combat_hold"),D.Current==Cue::Victory&&D.IsTransitioning());
    Simulate(5,Result);Result.Phase=3;Simulate(1.5,Result);
    Check(TEXT("music_defeat_owns_distinct_cue"),D.Current==Cue::Defeat);
    Simulate(5,Result);Result.Phase=0;Simulate(1.5,Result);
    Check(TEXT("music_return_to_command_room_leaves_result_cue"),D.Current==Cue::Vigil);
    Check(TEXT("music_different_entry_exit_thresholds"),FFrontlineMusicDirectorV31::Choose(.18f,Cue::Contact,1)==Cue::Contact
        &&FFrontlineMusicDirectorV31::Choose(.18f,Cue::Vigil,1)==Cue::Vigil);
    Check(TEXT("music_finite_bounded_score"),FFrontlineMusicDirectorV31::Score(Heavy)<=1.f&&FFrontlineMusicDirectorV31::Score(Quiet)==0.f);
    D=FFrontlineMusicDirectorV31();D.Advance(.05f,Quiet);D.Advance(1.75f,Quiet);
    Check(TEXT("music_render_hitch_preserves_harmonic_clock"),FMath::Abs(D.Clock-1.8)<.0001);
}

void RunFrontlineMusicV828PolicyChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    RunFrontlineMusicV31PolicyChecks(Check);
    using Cue=EFrontlineMusicCueV31;
    FFrontlineMusicDirectorV31 D;
    FFrontlineMusicObservationV31 O;O.Phase=1;O.ShotsPerSecond=14;O.EngagedDensity=.8f;
    for(int32 I=0;I<640;++I)D.Advance(.05f,O);
    Check(TEXT("v8211_music_sustained_combat_reaches_contact_on_phrase_boundary"),D.Current==Cue::Contact);
    O.ShotsPerSecond=0;O.EngagedDensity=0;
    const uint32 Before=D.Generation;
    for(int32 I=0;I<60;++I)D.Advance(.05f,O);
    Check(TEXT("v828_music_short_reload_lull_does_not_change_arrangement"),D.Generation==Before);
    for(int32 I=0;I<800;++I)D.Advance(.05f,O);
    Check(TEXT("v828_music_sustained_lull_returns_to_vigil"),D.Current==Cue::Vigil);
    const uint32 QuietGeneration=D.Generation;O.BlastsPerSecond=20;
    for(int32 I=0;I<20;++I)D.Advance(.05f,O);
    Check(TEXT("v828_music_one_isolated_blast_is_not_full_barrage"),D.Generation==QuietGeneration);
    TSet<FName> Names;
    for(int32 Nation=0;Nation<4;++Nation)for(int32 I=0;I<3;++I)Names.Add(FFrontlineMusicDirectorV31::NationalAssetNameV828(Cue(I),Nation));
    Check(TEXT("v8211_music_nations_share_only_the_accepted_three_arrangements"),Names.Num()==3);
    Check(TEXT("v828_music_existing_outcome_score_preserved"),FFrontlineMusicDirectorV31::NationalAssetNameV828(Cue::Victory,3)==TEXT("S_Music_Victory_V46")
        &&FFrontlineMusicDirectorV31::NationalAssetNameV828(Cue::Defeat,2)==TEXT("S_Music_Defeat_V31"));
}
