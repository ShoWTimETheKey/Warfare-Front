// V26: calm legacy acknowledgements and nonfatal pain retired; original assets retained for previous archives.
// Manifest: ArtSource/Audio/V20/audio-provenance.json (19 new WAVs + 9 retained V18 recordings).
// Independent human recordings; shared event masks are contextual reuse, not new performances.
// Source identities persist across versions/events. No pitch-generated speech variants.
// Medic is excluded: Wounded routes to sampled nonverbal pain, not a medical game mechanic.
// UnderMachinegun requires a real entrenched target; Contact requires actual engagement.
// Name, package root, event mask, language (EN=0/DE=1/neutral=-1), delivery, original recording ID.
{TEXT("S_Voice_EN_ReloadCover_01_V20"), TEXT("/Game/Warfare/Audio/V20"), VEvent(EFrontlineVoiceEvent::ReloadCover), 0, VoiceSpeech, TEXT("kenney:male:war_reloading")},
{TEXT("S_Voice_EN_GrenadeThrowWarning_01_V20"), TEXT("/Game/Warfare/Audio/V20"), VEvent(EFrontlineVoiceEvent::GrenadeThrowWarning), 0, VoiceSpeech, TEXT("kenney:male:war_fire_in_the_hole")},
{TEXT("S_Voice_EN_Cover_01_V20"), TEXT("/Game/Warfare/Audio/V20"), VEvent(EFrontlineVoiceEvent::Cover) | VEvent(EFrontlineVoiceEvent::UnderMachinegun) | VEvent(EFrontlineVoiceEvent::GrenadeWarning) | VEvent(EFrontlineVoiceEvent::IncomingArtillery), 0, VoiceSpeech, TEXT("freesound:800514:full")},
{TEXT("S_Voice_DE_Advance_01_V20"), TEXT("/Game/Warfare/Audio/V20"), VEvent(EFrontlineVoiceEvent::Advance) | VEvent(EFrontlineVoiceEvent::PassFullTrench), 1, VoiceSpeech, TEXT("freesound:443442:full")},
{TEXT("S_Death_01_V20"), TEXT("/Game/Warfare/Audio/V20"), 0u, -1, VoiceFatal, TEXT("haeldb:yell3")},
{TEXT("S_Death_02_V20"), TEXT("/Game/Warfare/Audio/V20"), 0u, -1, VoiceFatal, TEXT("haeldb:yell6")},
{TEXT("S_Death_03_V20"), TEXT("/Game/Warfare/Audio/V20"), 0u, -1, VoiceFatal, TEXT("haeldb:3grunt6")},
{TEXT("S_Death_04_V20"), TEXT("/Game/Warfare/Audio/V20"), 0u, -1, VoiceFatal, TEXT("haeldb:2yell6")},
{TEXT("S_Voice_EN_Advance_01_V18"), TEXT("/Game/Warfare/Audio/V18"), VEvent(EFrontlineVoiceEvent::Advance) | VEvent(EFrontlineVoiceEvent::PassFullTrench), 0, VoiceSpeech, TEXT("kenney:male:war_go_go_go")},
{TEXT("S_Voice_EN_Contact_01_V18"), TEXT("/Game/Warfare/Audio/V18"), VEvent(EFrontlineVoiceEvent::Contact) | VEvent(EFrontlineVoiceEvent::GrenadeWarning) | VEvent(EFrontlineVoiceEvent::IncomingArtillery), 0, VoiceSpeech, TEXT("kenney:male:war_look_out")},
{TEXT("S_Voice_EN_Contact_02_V18"), TEXT("/Game/Warfare/Audio/V18"), VEvent(EFrontlineVoiceEvent::UnderMachinegun) | VEvent(EFrontlineVoiceEvent::Cover), 0, VoiceSpeech, TEXT("kurt:battlecries:19.92-21.00")},
{TEXT("S_Voice_EN_Cover_01_V18"), TEXT("/Game/Warfare/Audio/V18"), VEvent(EFrontlineVoiceEvent::Cover) | VEvent(EFrontlineVoiceEvent::SmokeCover) | VEvent(EFrontlineVoiceEvent::ReloadCover), 0, VoiceSpeech, TEXT("kenney:male:war_cover_me")},
{TEXT("S_Voice_EN_Cover_02_V18"), TEXT("/Game/Warfare/Audio/V18"), VEvent(EFrontlineVoiceEvent::Cover) | VEvent(EFrontlineVoiceEvent::SmokeCover) | VEvent(EFrontlineVoiceEvent::ReloadCover), 0, VoiceSpeech, TEXT("kenney:male:war_watch_my_back")},
{TEXT("S_Voice_EN_UnderMG_01_V18"), TEXT("/Game/Warfare/Audio/V18"), VEvent(EFrontlineVoiceEvent::UnderMachinegun) | VEvent(EFrontlineVoiceEvent::GrenadeWarning) | VEvent(EFrontlineVoiceEvent::IncomingArtillery), 0, VoiceSpeech, TEXT("kenney:male:war_get_down")},
{TEXT("S_Voice_EN_UnderMG_02_V18"), TEXT("/Game/Warfare/Audio/V18"), VEvent(EFrontlineVoiceEvent::UnderMachinegun), 0, VoiceSpeech, TEXT("kenney:male:war_call_for_backup")},
