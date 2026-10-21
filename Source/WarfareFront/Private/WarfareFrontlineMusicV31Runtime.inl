void UFrontlineAudioSubsystem::TickMusicV31(float DT)
{
    if(!GetWorld() || Components.Num()!=TotalSlots)return;
    const auto* Battle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    const double Now=GetWorld()->GetRealTimeSeconds();
    const uint8 AudiblePhase=Battle?(Battle->BattleOutroWinner>=0?uint8(Battle->BattleOutroWinner==0?2:3):uint8(Battle->Phase)):0;
    int32 WantedNation=Battle?int32(Battle->GetPlayerFaction()):0;
    if(Battle&&WarfareFRRUCampaignV82::Active(Battle))WantedNation=int32(WarfareFRRUCampaignV82::Nation(Battle,0));
    if(AudiblePhase==0 && MenuMusicNationV828!=INDEX_NONE)WantedNation=MenuMusicNationV828;
    WantedNation=FMath::Clamp(WantedNation,0,3);
    if(MusicNationV828==INDEX_NONE)
    {MusicNationV828=WantedNation;MusicPlayerNationV828[0]=MusicPlayerNationV828[1]=WantedNation;}
    MusicObservationV31.Phase=AudiblePhase;
    if(Now>=MusicNextObservationV31 && !GetWorld()->IsPaused())
    {
        const float Seconds=FMath::Max(.25f,float(Now-MusicLastObservationV31));
        MusicLastObservationV31=Now;MusicNextObservationV31=Now+.5;
        FFrontlineMusicObservationV31 O;O.Phase=AudiblePhase;
        if(Battle && Battle->Phase==EFrontlinePhase::Fighting)
        {
            const int32 Losses=Battle->Casualties[0]+Battle->Casualties[1];
            O.ShotsPerSecond=FMath::Max(0,Battle->ShotsFired-MusicLastShotsV31)/Seconds;
            O.BlastsPerSecond=FMath::Max(0,MusicBlastEventsV31-MusicLastBlastsV31)/Seconds;
            O.CasualtiesPerSecond=FMath::Max(0,Losses-MusicLastCasualtiesV31)/Seconds;
            MusicLastShotsV31=Battle->ShotsFired;MusicLastCasualtiesV31=Losses;
            int32 Alive=0,Engaged=0,Suppressed=0;
            for(const AWarfareFrontlineSoldier* Unit:Battle->Units)
                if(Unit && Unit->IsActiveCombatant())
                {++Alive;if(Unit->bEngaging)++Engaged;if(Unit->Suppression>.30f)++Suppressed;}
            O.EngagedDensity=FMath::Clamp(Engaged/24.f,0.f,1.f);
            O.SuppressedFraction=Alive?float(Suppressed)/Alive:0.f;
            O.MoralePressure=FMath::Clamp((50.f-FMath::Min(Battle->Morale[0],Battle->Morale[1]))/50.f,0.f,1.f);
        }
        else MusicLastShotsV31=MusicLastCasualtiesV31=0;
        MusicLastBlastsV31=MusicBlastEventsV31;
        MusicObservationV31=O;
    }
    MusicDirectorV31.Advance(DT,MusicObservationV31);
    // Nation previews never restart or layer another copy of the score.
    MusicNationV828=WantedNation;
    if(MusicAppliedGenerationV31!=MusicDirectorV31.Generation)
    {
        MusicActivePlayerV31=1-MusicActivePlayerV31;
        MusicPlayerCueV31[MusicActivePlayerV31]=MusicDirectorV31.Current;
        MusicPlayerNationV828[MusicActivePlayerV31]=MusicNationV828;
        MusicCueOriginV31[MusicActivePlayerV31]=FFrontlineMusicDirectorV31::IsResult(MusicDirectorV31.Current)?MusicDirectorV31.Clock:0.;
        Components[MusicSlot+MusicActivePlayerV31]->Stop();
        MusicAppliedGenerationV31=MusicDirectorV31.Generation;
        UE_LOG(LogTemp,Display,TEXT("V828_MUSIC_CUE cue=%s nation=%d tension=%.3f phrase=%.2f generation=%u"),
            *GetMusicAssetV828().ToString(),MusicNationV828,MusicDirectorV31.Tension,float(MusicDirectorV31.Clock),MusicDirectorV31.Generation);
    }
    for(int32 Player=0;Player<MusicPlayersV31;++Player)
    {
        const int32 Slot=MusicSlot+Player;
        auto* Component=Components[Slot].Get();auto& E=Emitters[Slot];
        const float Weight=Player==MusicActivePlayerV31?MusicDirectorV31.CurrentWeight():MusicDirectorV31.PreviousWeight();
        E.Bus=EFrontlineAudioBus::Music;E.bSpatial=false;E.Gain=1.8f*Weight;
        // No running decoder when explicitly muted. The continuous score clock
        // gives the correct phrase on resume, without restarting its first bar.
        if(bMuted || Volumes[0]<=0.f || Volumes[4]<=0.f || Weight<.0001f)
        {Component->Stop();UpdateEmitter(Slot);continue;}
        if(!Component->IsPlaying())
        {
            const FName Name=FFrontlineMusicDirectorV31::NationalAssetNameV828(MusicPlayerCueV31[Player],MusicPlayerNationV828[Player]);
            const auto* Wave=Sounds.Find(Name);
            if(!Wave || !*Wave)continue;
            Component->SetSound(*Wave);Component->SetPitchMultiplier(1.f);UpdateEmitter(Slot);
            const float Offset=float(FMath::Fmod(MusicDirectorV31.Clock-MusicCueOriginV31[Player],64.));
            Component->Play(FMath::Max(0.f,Offset));
        }
        UpdateEmitter(Slot);
    }
}

void UFrontlineAudioSubsystem::RunV31MusicChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Allowed=FParse::Param(FCommandLine::Get(),TEXT("WFV31Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV31AudioReview"));
    Check(TEXT("music_v31_isolated_review_only"),Allowed);if(!Allowed)return;
    LoadBank();RunFrontlineMusicV31PolicyChecks(Check);
    bool Loaded=true,Distinct=true;TSet<const USoundWave*> Waves;
    for(int32 I=0;I<5;++I)
    {
        const auto* Found=Sounds.Find(FName(FFrontlineMusicDirectorV31::AssetName(EFrontlineMusicCueV31(I))));
        const USoundWave* Wave=Found?Found->Get():nullptr;
        const bool Valid=Wave && Wave->NumChannels==2 && Wave->bLooping && FMath::Abs(Wave->Duration-64.f)<.005f;
        Loaded&=Valid;Distinct&=Wave&&!Waves.Contains(Wave);Waves.Add(Wave);
    }
    Check(TEXT("music_five_loaded_stereo_64_second_loops"),Loaded);
    Check(TEXT("music_five_independent_soundwave_arrangements"),Distinct);
    Check(TEXT("music_decoder_budget_is_two_separate_from_effects_and_voices"),MusicPlayersV31==2&&RainSlot-MusicSlot==2&&VoiceSlots==2);
    // Exercise the actual emitter path. Do not persist test values to user settings.
    const bool MutedBefore=bMuted;const float MasterBefore=Volumes[0],MusicBefore=Volumes[4],GainBefore=MusicGain;
    float EmitterBefore[2]={Emitters[MusicSlot].Gain,Emitters[MusicSlot+1].Gain};
    EFrontlineAudioBus BusBefore[2]={Emitters[MusicSlot].Bus,Emitters[MusicSlot+1].Bus};
    bool SpatialBefore[2]={Emitters[MusicSlot].bSpatial,Emitters[MusicSlot+1].bSpatial};
    Volumes[0]=.8f;Volumes[4]=.5f;MusicGain=.7f;bMuted=false;
    for(int32 I=0;I<2;++I){Emitters[MusicSlot+I].Bus=EFrontlineAudioBus::Music;Emitters[MusicSlot+I].bSpatial=false;Emitters[MusicSlot+I].Gain=1.f;UpdateEmitter(MusicSlot+I);}
    const float Expected=.8f*.5f*.7f;
    Check(TEXT("music_actual_players_honor_master_music_and_duck_gain"),FMath::IsNearlyEqual(Components[MusicSlot]->VolumeMultiplier,Expected,.0001f)
        &&FMath::IsNearlyEqual(Components[MusicSlot+1]->VolumeMultiplier,Expected,.0001f));
    bMuted=true;RefreshMix();
    Check(TEXT("music_master_mute_is_exact_zero_for_both_players"),Components[MusicSlot]->VolumeMultiplier==0.f&&Components[MusicSlot+1]->VolumeMultiplier==0.f);
    bMuted=false;Volumes[4]=0.f;RefreshMix();
    Check(TEXT("music_slider_zero_is_exact_zero_for_both_players"),Components[MusicSlot]->VolumeMultiplier==0.f&&Components[MusicSlot+1]->VolumeMultiplier==0.f);
    bMuted=MutedBefore;Volumes[0]=MasterBefore;Volumes[4]=MusicBefore;MusicGain=GainBefore;
    for(int32 I=0;I<2;++I){Emitters[MusicSlot+I].Gain=EmitterBefore[I];Emitters[MusicSlot+I].Bus=BusBefore[I];Emitters[MusicSlot+I].bSpatial=SpatialBefore[I];}
    RefreshMix();
}
