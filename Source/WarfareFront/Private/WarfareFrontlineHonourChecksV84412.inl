void RunChallengeChecksV84412(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    // Keep the older focused entry point current rather than testing superseded
    // 18-kill / two-vehicle conditions after the catalogue has changed.
    RunChallengeChecksV84415(B,Check);
}
