#include "WarfareFrontlineAudio.h"
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "AudioMixerBlueprintLibrary.h"
#include "SubmixEffects/AudioMixerSubmixEffectDynamicsProcessor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"
#include "Dom/JsonObject.h"

namespace
{
    constexpr int32 EffectSlots = 32;
    constexpr int32 VoiceSlots = 2;
    constexpr int32 AmbienceSlot = EffectSlots + VoiceSlots;
    constexpr int32 MusicSlot = AmbienceSlot + 1;
    constexpr int32 MusicPlayersV31 = 2;
    constexpr int32 RainSlot = MusicSlot + MusicPlayersV31;
    constexpr int32 TankSlotStart = RainSlot + 1;
    constexpr int32 TankAudioVehicles = 4; // Rendering voices only, never a unit cap.
    constexpr int32 AircraftSlotStart = TankSlotStart + TankAudioVehicles*2;
    constexpr int32 TotalSlots = AircraftSlotStart + 4;
    constexpr uint8 VoiceSpeech = 0, VoicePain = 1, VoiceFatal = 2, VoiceBlastScream = 3;
    constexpr float MechanicalPresenceV84411=1.2589254f; // +2 dB; speech/music are unchanged.
    float VehicleGainV828(int32 Part,float Speed)
    {return Part==0?FMath::Lerp(.20f,.34f,Speed):MechanicalPresenceV84411*FMath::Lerp(.14f,.40f,Speed);}
    bool TracksAudibleV828(bool Intact,float Speed)
    {return Intact&&FMath::IsFinite(Speed)&&Speed>.025f;}
    FVector AircraftListeningFocusV8218(const APlayerCameraManager* Camera)
    {
        if(!Camera)return FVector::ZeroVector;
        const FVector P=Camera->GetCameraLocation(),Forward=Camera->GetCameraRotation().Vector();
        // Same tactical listener projection as ground emitters: camera height
        // must not make every engine inaudible. Actual flight altitude remains.
        return Forward.Z<-.02?P+Forward*(-P.Z/Forward.Z):P;
    }
    constexpr uint32 VEvent(EFrontlineVoiceEvent Event) { return 1u << uint32(Event); }
    struct FVoiceClip
    {
        const TCHAR* Name;
        const TCHAR* Package;
        uint32 Events;
        int32 Language;
        uint8 Kind;
        const TCHAR* Source;
    };
    const FVoiceClip VoiceBank[] = {
#include "WarfareFrontlineAudioBankV30.inl"
#include "WarfareFrontlineAudioBankV40.inl"
#include "WarfareFrontlineAudioBankV43.inl"
#include "WarfareFrontlineAudioBankV65.inl"
#include "FRRU/FRRUAudioV82.generated.inl"
    };
    bool IsUrgent(EFrontlineVoiceEvent Event)
    {
        return Event == EFrontlineVoiceEvent::GrenadeWarning
            || Event == EFrontlineVoiceEvent::GrenadeThrowWarning
            || Event == EFrontlineVoiceEvent::IncomingArtillery
            || Event == EFrontlineVoiceEvent::GasAlarm
            || Event == EFrontlineVoiceEvent::AircraftInbound
            || Event == EFrontlineVoiceEvent::TankBurning;
    }
    FString SquadKey(int32 bGerman, int32 SquadId)
    { return FString::Printf(TEXT("%d:%d"), bGerman, SquadId); }
    FString EventKey(EFrontlineVoiceEvent Event, int32 bGerman)
    { return FString::Printf(TEXT("%d:%d"), int32(Event), bGerman); }
    constexpr double VoiceDensityV32 = .70;
    constexpr double VoiceGapV32(double FormerSeconds) { return FormerSeconds / VoiceDensityV32; }
    bool CanSpeak(const FFrontlineVoiceSchedule& State, EFrontlineVoiceEvent Event,
        int32 bGerman, int32 SquadId, double Now)
    {
        const bool Urgent = IsUrgent(Event);
        return Now >= (Urgent ? State.NextUrgent : State.NextGlobal)
            && Now >= State.NextEvent.FindRef(EventKey(Event, bGerman))
            && Now >= (Urgent ? State.NextUrgentSquad : State.NextSquad).FindRef(SquadKey(bGerman, SquadId));
    }
    void CommitSpeech(FFrontlineVoiceSchedule& State, EFrontlineVoiceEvent Event,
        int32 bGerman, int32 SquadId, double Now)
    {
        State.NextGlobal = Now + VoiceGapV32(2.2);
        State.NextUrgent = Now + VoiceGapV32(.85);
        State.NextCasualty = FMath::Max(State.NextCasualty, Now + VoiceGapV32(3.5));
        State.NextSquad.Add(SquadKey(bGerman, SquadId), Now + VoiceGapV32(9.));
        State.NextUrgentSquad.Add(SquadKey(bGerman, SquadId), Now + VoiceGapV32(4.5));
        State.NextEvent.Add(EventKey(Event, bGerman), Now + VoiceGapV32(IsUrgent(Event) ? 7. : 10.));
    }
    // The second vocal channel is reserved for an imminent warning. No spoken
    // line or pain transient is cut off; lower-priority requests wait for silence.
    bool HasVoiceSlot(int32 Active, uint8 Priority)
    { return Active < VoiceSlots && (Priority >= 3 || Active == 0); }
    // The released 1944 reference starts one collective cheer. Its
    // staggered soldier animation is not a repeated melodic-whistle scheduler.
    constexpr double OutcomeStartV46=.12;
    constexpr double OutcomeDeadlineV46=3.5;
    enum class EOutcomeDecisionV46:uint8 { Cancel,Wait,Play };
    EOutcomeDecisionV46 OutcomeDecisionV46(double Now,double Began,double Due,bool Current,int32 Active,int32 Spoken)
    {
        if(!Current||Now>=Began+OutcomeDeadlineV46)return EOutcomeDecisionV46::Cancel;
        return Now<Due||!HasVoiceSlot(Active,3)||Spoken>0?EOutcomeDecisionV46::Wait:EOutcomeDecisionV46::Play;
    }
    float OutcomeMusicHandoffV51(double Age)
    {
        // Collective human relief leads. A ten-second instrumental entrance
        // follows the released reference's dramatic order, without copying it.
        if(Age<0.)return 1.f;
        if(Age<1.6)return .12f;
        if(Age<4.3)return FMath::Lerp(.12f,.42f,float((Age-1.6)/2.7));
        if(Age<7.5)return FMath::Lerp(.42f,.83f,float((Age-4.3)/3.2));
        return FMath::Lerp(.83f,1.f,FMath::Clamp(float((Age-7.5)/2.5),0.f,1.f));
    }
    int32 PickVoice(FFrontlineVoiceSchedule& State, const FString& Key,
        const TArray<int32>& Candidates, double Now)
    {
        if (Candidates.IsEmpty()) return INDEX_NONE;
        auto& Bag = State.Bags.FindOrAdd(Key);
        if (Bag.Remaining.IsEmpty())
        {
            Bag.Remaining = Candidates;
            for (int32 I = Bag.Remaining.Num()-1; I > 0; --I)
                Bag.Remaining.Swap(I, State.Random.RandRange(0, I));
            if (Bag.Remaining.Num() > 1 && FName(VoiceBank[Bag.Remaining.Last()].Source) == Bag.PreviousSource)
                Bag.Remaining.Swap(0, Bag.Remaining.Num()-1);
        }
        for (int32 I = Bag.Remaining.Num()-1; I >= 0; --I)
        {
            const int32 Candidate = Bag.Remaining[I];
            if (Candidates.Contains(Candidate) && Now >= State.NextSource.FindRef(FName(VoiceBank[Candidate].Source)))
                return Candidate;
        }
        return INDEX_NONE; // Never refill a bag merely to evade a source cooldown.
    }
    void CommitVoice(FFrontlineVoiceSchedule& State, const FString& Key, int32 Clip, double Now)
    {
        auto& Bag = State.Bags.FindOrAdd(Key);
        Bag.Remaining.RemoveSingle(Clip);
        Bag.PreviousSource = FName(VoiceBank[Clip].Source);
        State.NextSource.Add(Bag.PreviousSource, Now + 45.);
    }
    // Return a delivery class or INDEX_NONE. Failed sampling also cools this
    // soldier, so repeated damage ticks cannot turn a rare pain into certainty.
    int32 DecideCasualty(FFrontlineVoiceSchedule& State, bool bGerman, int32 SoldierId, int32 SquadId,
        bool bFatal, bool bExplosion, float Damage, double Now, float ProbabilityRoll, float ScreamRoll)
    {
        if (!FMath::IsFinite(Damage) || (!bFatal && Damage < 18.f)
            || Now < State.NextCasualty || Now < State.NextGlobal
            || Now < State.NextPain.FindRef(SoldierId)
            || Now < State.NextSquad.FindRef(SquadKey(bGerman, SquadId))) return INDEX_NONE;
        State.NextPain.Add(SoldierId, Now + 18.);
        if (ProbabilityRoll >= (bFatal ? .38f : .18f)) return INDEX_NONE;
        // Non-verbal blast delivery is now an actual separate pool. Gas and
        // bullets never select it, and one salvo cannot create a chorus.
        if (bFatal && bExplosion && Now >= State.NextScream && ScreamRoll < .50f) return VoiceBlastScream;
        return bFatal ? VoiceFatal : VoicePain;
    }
    void CommitCasualty(FFrontlineVoiceSchedule& State, bool bGerman, int32 SquadId,
        bool bFatal, bool bScream, double Now)
    {
        State.NextCasualty = Now + VoiceGapV32(bFatal ? 6. : 8.);
        State.NextGlobal = Now + VoiceGapV32(2.2);
        State.NextUrgent = Now + VoiceGapV32(.6);
        State.NextSquad.Add(SquadKey(bGerman, SquadId), Now + VoiceGapV32(12.));
        if (bScream) State.NextScream = Now + VoiceGapV32(20.);
    }
    enum class ECauseV43:uint8 { Bullet,Blast,Gas,Burn };
    float CauseAdmissionV43(ECauseV43 Cause)
    {
        switch(Cause){case ECauseV43::Burn:return .96f;case ECauseV43::Blast:return .76f;case ECauseV43::Gas:return .84f;default:return .48f;}
    }
    int32 DecideCauseV43(FFrontlineVoiceSchedule& State,ECauseV43 Cause,int32 German,int32 Person,int32 Squad,double Now,float Roll,float ScreamRoll)
    {
        const bool Severe=Cause==ECauseV43::Burn||Cause==ECauseV43::Blast||Cause==ECauseV43::Gas;
        if(Now<State.NextPain.FindRef(Person)||Now<State.NextSevereCasualtyV43||Now<State.NextUrgent)return INDEX_NONE;
        if(Severe)
        {if(Now<State.NextSevereSquadV43.FindRef(SquadKey(German,Squad)))return INDEX_NONE;}
        else if(Now<State.NextCasualty||Now<State.NextGlobal||Now<State.NextSquad.FindRef(SquadKey(German,Squad)))return INDEX_NONE;
        State.NextPain.Add(Person,Now+18.);
        if(Roll>=CauseAdmissionV43(Cause))return INDEX_NONE;
        if(Cause==ECauseV43::Burn||Cause==ECauseV43::Gas)return VoiceSpeech;
        if(Cause==ECauseV43::Blast&&Now>=State.NextScream&&ScreamRoll<.72f)return VoiceBlastScream;
        return VoiceFatal;
    }
    void CommitCauseV43(FFrontlineVoiceSchedule& State,ECauseV43 Cause,int32 German,int32 Squad,bool Scream,double Now)
    {
        // A real violent event can be heard after an ordinary fatality, but
        // every admitted cry shares this global floor: never a salvo chorus.
        State.NextSevereCasualtyV43=Now+(Cause==ECauseV43::Burn?3.6:4.4);
        State.NextCasualty=Now+(Cause==ECauseV43::Gas?7.2:6.2);
        State.NextGlobal=Now+2.5;State.NextUrgent=Now+.85;
        State.NextSquad.Add(SquadKey(German,Squad),Now+12.);
        State.NextSevereSquadV43.Add(SquadKey(German,Squad),Now+8.5);
        if(Scream)State.NextScream=Now+10.;
    }
    constexpr double BurnWaitV44=1.35;
    double BurnReadyAtV44(const FFrontlineVoiceSchedule& State,int32 German,int32 Squad)
    {
        return FMath::Max(State.NextUrgent,FMath::Max(State.NextSevereCasualtyV43,
            State.NextSevereSquadV43.FindRef(SquadKey(German,Squad))));
    }
    // The same bounded scheduler is used below and in focused checks. A wait
    // neither samples again nor reserves a second vocal channel.
    enum class EBurnQueueV44:uint8 { Cancel,Wait,Play };
    EBurnQueueV44 BurnQueueV44(double Now,double Expires,double ReadyAt,int32 Active,bool CurrentBurn)
    {
        if(!CurrentBurn||Now>Expires||ReadyAt>Expires)return EBurnQueueV44::Cancel;
        return Now<ReadyAt||!HasVoiceSlot(Active,1)?EBurnQueueV44::Wait:EBurnQueueV44::Play;
    }
    bool CurrentBurnV44(const AWarfareFrontlineSoldier* U,const FGuid& Encounter)
    {
        if(!IsValid(U)||U->IsActorBeingDestroyed()||U->IsHidden()||U->IsAlive()||!U->Battle
            ||U->Battle->GetEncounterId()!=Encounter||U->Battle->Phase!=EFrontlinePhase::Fighting)return false;
        const auto& D=U->GetDeathState();
        return D.bStarted&&D.Kind==EFrontlineDamageKind::Fire&&D.Age<3.f&&!D.bSettled&&!D.bFuelRuptureApplied;
    }
    int32 NationalLanguageV82(const AWarfareFrontlineGameMode* Battle,bool LegacyGerman,int32 Squad,int32 Person=INDEX_NONE)
    {
        if(!WarfareFRRUCampaignV82::Active(Battle))return int32(LegacyGerman);
        int32 Language=INDEX_NONE;
        for(const AWarfareFrontlineSoldier* Unit:Battle->Units)
        {
            if(!IsValid(Unit)||Unit->SquadId!=Squad||(Person!=INDEX_NONE&&Unit->SquadId*16+Unit->MemberIndex!=Person))continue;
            const int32 Nation=int32(WarfareFRRUCampaignV82::Nation(Unit));
            if(Nation<0||Nation>4||(Language!=INDEX_NONE&&Language!=Nation))return INDEX_NONE;
            Language=Nation;
        }
        return Language;
    }
    const TCHAR* ConfigSection = TEXT("WarfareFrontline.AudioV14");
    const TCHAR* BusKeys[] = { TEXT("Master"), TEXT("Effects"), TEXT("Voices"), TEXT("Ambience"), TEXT("Music") };
}

bool UFrontlineAudioSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UFrontlineAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    VoiceSchedule.Random.Initialize(int32(FPlatformTime::Cycles()));
    if (GConfig)
    {
        for (int32 I = 0; I < 5; ++I)
        {
            GConfig->GetFloat(ConfigSection, BusKeys[I], Volumes[I], GGameUserSettingsIni);
            Volumes[I] = FMath::IsFinite(Volumes[I]) ? FMath::Clamp(Volumes[I], 0.f, 1.f) : .8f;
        }
        GConfig->GetBool(ConfigSection, TEXT("Muted"), bMuted, GGameUserSettingsIni);
    }
}

void UFrontlineAudioSubsystem::Deinitialize()
{
    SaveSettings(true);
    if (MasterLimiter) UAudioMixerBlueprintLibrary::RemoveMasterSubmixEffect(this,MasterLimiter);
    MasterLimiter=nullptr;
    for (UAudioComponent* Component : Components)
        if (Component) { Component->Stop(); Component->DestroyComponent(); }
    Components.Empty();
    Emitters.Empty();
    Sounds.Empty();
    Super::Deinitialize();
}

TStatId UFrontlineAudioSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UFrontlineAudioSubsystem, STATGROUP_Tickables);
}

float UFrontlineAudioSubsystem::GetVolume(EFrontlineAudioBus Bus) const
{
    const int32 I = int32(Bus);
    return I >= 0 && I < 5 ? Volumes[I] : 0.f;
}

void UFrontlineAudioSubsystem::QueueSettingsSaveV834()
{
    if(FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    bSettingsDirtyV834=true;
    SettingsSaveAtV834=FPlatformTime::Seconds()+.25;
}

void UFrontlineAudioSubsystem::SaveSettings(bool bForce)
{
    if(!bSettingsDirtyV834)return;
    // Review changes are audible, but cannot dirty or flush the player's INI.
    if(FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {bSettingsDirtyV834=false;SettingsSaveAtV834=0.;return;}
    if(!GConfig||GGameUserSettingsIni.IsEmpty()
        ||(!bForce&&FPlatformTime::Seconds()<SettingsSaveAtV834))return;
    for (int32 I = 0; I < 5; ++I) GConfig->SetFloat(ConfigSection, BusKeys[I], Volumes[I], GGameUserSettingsIni);
    GConfig->SetBool(ConfigSection, TEXT("Muted"), bMuted, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
    bSettingsDirtyV834=false;SettingsSaveAtV834=0.;
}

void UFrontlineAudioSubsystem::SetVolume(EFrontlineAudioBus Bus, float Volume)
{
    const int32 I = int32(Bus);
    if (I < 0 || I >= 5 || !FMath::IsFinite(Volume)) return;
    const float Clamped=FMath::Clamp(Volume,0.f,1.f);
    if(Volumes[I]==Clamped)return;
    Volumes[I] = Clamped;
    RefreshMix();
    QueueSettingsSaveV834();
}

void UFrontlineAudioSubsystem::SetMuted(bool bValue)
{
    if(bMuted==bValue)return;
    bMuted = bValue;
    RefreshMix();
    QueueSettingsSaveV834();
}

void UFrontlineAudioSubsystem::LoadBank()
{
    if (bLoaded || !GetWorld() || GetWorld()->GetNetMode() == NM_DedicatedServer) return;
    bLoaded = true;
    const TCHAR* Names[] = {
        TEXT("S_Rifle_British_01_V14"), TEXT("S_Rifle_British_02_V14"),
        TEXT("S_Rifle_German_01_V14"), TEXT("S_Rifle_German_02_V14"),
        TEXT("S_Automatic_British_01_V14"), TEXT("S_Automatic_British_02_V14"),
        TEXT("S_Automatic_German_01_V14"), TEXT("S_Automatic_German_02_V14"),
        TEXT("S_Voice_EN_Advance_01_V14"), TEXT("S_Voice_EN_Advance_02_V14"),
        TEXT("S_Voice_EN_Contact_01_V14"), TEXT("S_Voice_EN_Contact_02_V14"),
        TEXT("S_Voice_EN_Cover_01_V14"), TEXT("S_Voice_EN_Cover_02_V14"),
        TEXT("S_Voice_EN_UnderMG_01_V14"), TEXT("S_Voice_EN_UnderMG_02_V14"),
        TEXT("S_Voice_EN_Wounded_01_V14"), TEXT("S_Voice_DE_Advance_01_V14"),
        TEXT("S_Voice_Nonverbal_Wounded_01_V14"), TEXT("S_Voice_Nonverbal_Wounded_02_V14"),
        TEXT("S_Voice_Nonverbal_Wounded_03_V14"), TEXT("S_Shell_V14"),
        TEXT("S_Grenade_V14"), TEXT("S_FieldWind_Stereo_V14")
    };
    for (const TCHAR* Name : Names)
    {
        const FString Path = FString::Printf(TEXT("/Game/Warfare/Field/V14/Audio/%s.%s"), Name, Name);
        if (USoundWave* Wave = LoadObject<USoundWave>(nullptr, *Path)) Sounds.Add(FName(Name), Wave);
        else UE_LOG(LogTemp, Warning, TEXT("V14_AUDIO_ASSET_MISSING %s"), *Path);
    }
    const TCHAR* V18Names[] = {
#include "WarfareFrontlineAudioBankV18.inl"
    };
    for(const TCHAR* Name:V18Names)
    {
        const FString Path=FString::Printf(TEXT("/Game/Warfare/Audio/V18/%s.%s"),Name,Name);
        if(USoundWave* Wave=LoadObject<USoundWave>(nullptr,*Path))Sounds.Add(FName(Name),Wave);
        else UE_LOG(LogTemp,Error,TEXT("V18_AUDIO_ASSET_MISSING %s"),*Path);
    }
    for(const TCHAR* Name:{TEXT("S_Tank_EngineLoop_V28"),TEXT("S_Tank_TracksLoop_V28"),TEXT("S_Tank_Cannon_01_V28"),TEXT("S_Aircraft_PropellerLoop_V28")})
    {
        const FString Path=FString::Printf(TEXT("/Game/Warfare/Audio/V28/%s.%s"),Name,Name);
        if(auto* Wave=LoadObject<USoundWave>(nullptr,*Path))Sounds.Add(FName(Name),Wave);
        else UE_LOG(LogTemp,Error,TEXT("V28_VEHICLE_AUDIO_MISSING %s"),*Path);
    }
    for(const TCHAR* Name:{TEXT("S_Shell_01_V30"),TEXT("S_Grenade_01_V30"),TEXT("S_Tank_Cannon_01_V30")})
    {
        const FString Path=FString::Printf(TEXT("/Game/Warfare/Audio/V30/%s.%s"),Name,Name);
        if(auto* Wave=LoadObject<USoundWave>(nullptr,*Path))Sounds.Add(FName(Name),Wave);
        else UE_LOG(LogTemp,Error,TEXT("V30_IMPACT_AUDIO_MISSING %s"),*Path);
    }
    for(const TCHAR* Family:{TEXT("Piercing"),TEXT("PlateBlast")})for(int32 V=1;V<=3;++V)
    {
        const FString Name=FString::Printf(TEXT("S_Armour_%s_%02d_V837"),Family,V);
        const FString Path=FString::Printf(TEXT("/Game/Warfare/Audio/V837/%s.%s"),*Name,*Name);
        if(auto* Wave=LoadObject<USoundWave>(nullptr,*Path))Sounds.Add(FName(*Name),Wave);
        // Retained recovery source; V839 has a separate production bank.
    }
    for(const TCHAR* Family:{TEXT("Rifle"),TEXT("Grenade"),TEXT("Cannon")})for(int32 V=1;V<=3;++V)
    {
        const FString Name=FString::Printf(TEXT("S_Armour_%s_%02d_V84411"),Family,V);
        const FString Path=FString::Printf(TEXT("/Game/Warfare/Audio/V84411/%s.%s"),*Name,*Name);
        if(auto* Wave=LoadObject<USoundWave>(nullptr,*Path))Sounds.Add(FName(*Name),Wave);
        else UE_LOG(LogTemp,Error,TEXT("WF_V84411_CONTACT_AUDIO_MISSING %s"),*Path);
    }
    for(const TCHAR* Name:{TEXT("S_ReliefWhistle_01_V32"),TEXT("S_ReliefWhistle_02_V32"),
        TEXT("VO_V32_UK_Relief_01"),TEXT("VO_V32_UK_Relief_02"),TEXT("VO_V32_DE_Relief_01"),TEXT("VO_V32_DE_Relief_02")})
    {
        const FString Path=FString::Printf(TEXT("/Game/Warfare/Audio/V32/%s.%s"),Name,Name);
        if(auto* Wave=LoadObject<USoundWave>(nullptr,*Path))Sounds.Add(FName(Name),Wave);
        else UE_LOG(LogTemp,Error,TEXT("V32_OUTCOME_AUDIO_MISSING %s"),*Path);
    }
    for (const FVoiceClip& Clip : VoiceBank)
    {
        if (Sounds.Contains(FName(Clip.Name))) continue;
        const FString Path = FString::Printf(TEXT("%s/%s.%s"), Clip.Package, Clip.Name, Clip.Name);
        if (USoundWave* Wave = LoadObject<USoundWave>(nullptr, *Path)) Sounds.Add(FName(Clip.Name), Wave);
        else UE_LOG(LogTemp, Error, TEXT("V20_VOICE_ASSET_MISSING %s"), *Path);
    }
    for(const TCHAR* Nation:{TEXT("FR"),TEXT("RU"),TEXT("AH")})for(int32 Variant=1;Variant<=2;++Variant)
    {
        const FString Name=FString::Printf(TEXT("S_OutcomeEnsemble_%s_%02d_V82"),Nation,Variant);
        const FString Path=FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Audio/Relief/%s.%s"),*Name,*Name);
        if(auto* Wave=LoadObject<USoundWave>(nullptr,*Path))Sounds.Add(FName(*Name),Wave);
        else UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_RELIEF_MISSING %s"),*Path);
    }
    for(int32 I=0;I<5;++I)
    {
        const TCHAR* Name=FFrontlineMusicDirectorV31::AssetName(EFrontlineMusicCueV31(I));
        const FString Path=I==int32(EFrontlineMusicCueV31::Victory)
            ?FString::Printf(TEXT("/Game/Warfare/Audio/V46/%s.%s"),Name,Name)
            :FString::Printf(TEXT("/Game/Warfare/Audio/V31/Music/%s.%s"),Name,Name);
        if(auto* Wave=LoadObject<USoundWave>(nullptr,*Path))Sounds.Add(FName(Name),Wave);
        else UE_LOG(LogTemp,Error,TEXT("V31_MUSIC_ASSET_MISSING %s"),*Path);
    }
    for(const TCHAR* Name:{TEXT("S_ReliefWhistle_01_V41"),TEXT("S_ReliefWhistle_02_V41")})
    {
        const FString Path=FString::Printf(TEXT("/Game/Warfare/Audio/V41/%s.%s"),Name,Name);
        if(auto* Wave=LoadObject<USoundWave>(nullptr,*Path))Sounds.Add(FName(Name),Wave);
        else UE_LOG(LogTemp,Error,TEXT("WF_V41_OUTCOME_SOUND_MISSING %s"),*Path);
    }
    for(int32 I=1;I<=4;++I)
    {
        const FString Name=FString::Printf(TEXT("S_ReliefWhistle_%02d_V43"),I);
        const FString Path=FString::Printf(TEXT("/Game/Warfare/Audio/V43/%s.%s"),*Name,*Name);
        if(auto* Wave=LoadObject<USoundWave>(nullptr,*Path))Sounds.Add(FName(*Name),Wave);
        else UE_LOG(LogTemp,Error,TEXT("WF_V43_AUDIO_MISSING %s"),*Path);
    }
    for(const TCHAR* Name:{TEXT("S_OutcomeEnsemble_UK_01_V51"),TEXT("S_OutcomeEnsemble_UK_02_V51"),
        TEXT("S_OutcomeEnsemble_DE_01_V51"),TEXT("S_OutcomeEnsemble_DE_02_V51")})
    {
        const FString Path=FString::Printf(TEXT("/Game/Warfare/Audio/V51/%s.%s"),Name,Name);
        if(USoundWave* Wave=LoadObject<USoundWave>(nullptr,*Path))Sounds.Add(FName(Name),Wave);
        else UE_LOG(LogTemp,Error,TEXT("WF_V51_OUTCOME_SOUND_MISSING %s"),*Path);
    }
    MasterLimiter=NewObject<USubmixEffectDynamicsProcessorPreset>(this);
    if(USoundWave* Rain=LoadObject<USoundWave>(nullptr,TEXT("/Game/Warfare/Weather/V19/Audio/S_Rain_Field_V19.S_Rain_Field_V19")))
        Sounds.Add(TEXT("S_Rain_Field_V19"),Rain);
    else UE_LOG(LogTemp,Error,TEXT("V19_RAIN_AUDIO_MISSING"));
    FSubmixEffectDynamicsProcessorSettings Dynamics;
    Dynamics.DynamicsProcessorType=ESubmixEffectDynamicsProcessorType::Limiter;
    Dynamics.PeakMode=ESubmixEffectDynamicsPeakMode::Peak;
    Dynamics.LinkMode=ESubmixEffectDynamicsChannelLinkMode::Peak;
    Dynamics.ThresholdDb=-1.5f; Dynamics.KneeBandwidthDb=0.f;
    Dynamics.LookAheadMsec=5.f; Dynamics.AttackTimeMsec=1.f; Dynamics.ReleaseTimeMsec=160.f;
    MasterLimiter->SetSettings(Dynamics);
    UAudioMixerBlueprintLibrary::AddMasterSubmixEffect(this,MasterLimiter);
    Emitters.SetNum(TotalSlots);
    for (int32 I = 0; I < TotalSlots; ++I)
    {
        UAudioComponent* Component = NewObject<UAudioComponent>(this);
        Component->SetAutoActivate(false);
        Component->bAutoDestroy = false;
        Component->bIsUISound = I>=MusicSlot && I<RainSlot;
        Component->bAllowSpatialization = I < AmbienceSlot || I>=TankSlotStart;
        Component->bOverridePriority = true;
        Component->Priority = I >= EffectSlots && I < AmbienceSlot ? 2.f : I>=MusicSlot&&I<RainSlot?1.8f:1.f;
        FSoundAttenuationSettings Attenuation;
        Attenuation.bAttenuate = false;
        Attenuation.bSpatialize = I < AmbienceSlot || I>=TankSlotStart;
        Attenuation.bEnableListenerFocus = false;
        Attenuation.bEnableOcclusion = false;
        // The samples contain quiet earth-bank reflections, without room reverb.
        Attenuation.bEnableReverbSend = false;
        Attenuation.NonSpatializedRadiusStart = 0.f;
        Attenuation.NonSpatializedRadiusEnd = 0.f;
        Component->AdjustAttenuation(Attenuation);
        Component->RegisterComponentWithWorld(GetWorld());
        Components.Add(Component);
    }
    UE_LOG(LogTemp, Display, TEXT("V20_AUDIO_BANK loaded=%d clips=%d sources=%d vocalSlots=%d pool=%d limiter=1"), Sounds.Num(), GetLoadedVoiceClipCount(), GetUniqueVoiceSourceCount(), VoiceSlots, TotalSlots);
}

void UFrontlineAudioSubsystem::ResetEncounterAudio()
{
    PendingBurnV44=FPendingBurnV44();
    ClearOutcomeV45();
    bBattleActive=false;
    for(int32 I=0;I<FMath::Min(MusicSlot,Components.Num());++I)if(Components[I])Components[I]->Stop();
    if(Components.IsValidIndex(RainSlot) && Components[RainSlot])Components[RainSlot]->Stop();
    for(int32 I=TankSlotStart;I<Components.Num();++I)
    {if(Components[I])Components[I]->Stop();if(Emitters.IsValidIndex(I))Emitters[I].VehicleId=INDEX_NONE;}
    NextStepTime.Empty();NextFootstepScan=0.;
    // Keep mix settings and source shuffle history; the next battle resets its
    // squad IDs and event counters through the ordinary active transition.
}

void UFrontlineAudioSubsystem::SetBattleActive(bool bActive)
{
    if(!bActive)PendingBurnV44=FPendingBurnV44();
    if (bBattleActive == bActive) return;
    bBattleActive = bActive;
    if (!bActive)
    {
        for(int32 I=0;I<FMath::Min(MusicSlot,Components.Num());++I)if(Components[I])Components[I]->Stop();
        if(Components.IsValidIndex(RainSlot))Components[RainSlot]->Stop();
        for(int32 I=TankSlotStart;I<Components.Num();++I)
        {Components[I]->Stop();Emitters[I].VehicleId=INDEX_NONE;}
        return;
    }
    LoadBank();
    PendingBurnV44=FPendingBurnV44();
    ClearOutcomeV45();
    VoiceSchedule.NextSquad.Empty();
    VoiceSchedule.NextUrgentSquad.Empty();
    VoiceSchedule.NextPain.Empty();
    VoiceSchedule.NextSevereSquadV43.Empty();VoiceSchedule.NextSevereCasualtyV43=0.;
    // Source cooldowns and bags survive an immediate restart; squad IDs do not.
    LastVariant.Empty(); NextStepTime.Empty();
    CasualtyEvents = FatalEvents = BlastScreamEvents = 0;
    MusicLastShotsV31=MusicLastCasualtiesV31=0;
    MusicLastBlastsV31=MusicBlastEventsV31;
    MusicNextObservationV31=0.;
    MusicObservationV31=FFrontlineMusicObservationV31();
    for (int32& Count : VoiceEventCounts) Count = 0;
    WeaponEvents = ImpactEvents = VoiceEvents = VoiceRejected = EffectsDropped = 0;
    EffectsDuck = 1.f;
    StartSound(TEXT("S_Ambience_Frontline_V18"), EFrontlineAudioBus::Ambience, FVector::ZeroVector, 1.4f);
}

float UFrontlineAudioSubsystem::GetPanForPosition(const FVector& Position) const
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC || !PC->PlayerCameraManager) return 0.f;
    const APlayerCameraManager* Camera = PC->PlayerCameraManager;
    const FVector Right = Camera->GetCameraRotation().RotateVector(FVector::RightVector);
    const float Width = FMath::Max(1500.f, Camera->GetCameraCacheView().OrthoWidth);
    return FMath::Clamp(float(FVector::DotProduct(Position-Camera->GetCameraLocation(), Right)) / (Width*.5f), -.92f, .92f);
}

void UFrontlineAudioSubsystem::CaptureSpatialMixViewV834(FSpatialMixViewV834& View) const
{
    View.bCaptured=true;++View.Captures;
    APlayerController* PC=GetWorld()?UGameplayStatics::GetPlayerController(GetWorld(),0):nullptr;
    View.bHasController=PC!=nullptr;
    if(!PC)return;
    PC->GetAudioListenerPosition(View.Listener,View.Forward,View.Right);
    const APlayerCameraManager* Camera=PC->PlayerCameraManager;
    View.bHasCamera=Camera!=nullptr;
    View.AircraftFocus=AircraftListeningFocusV8218(Camera);
    if(!Camera)return;
    View.CameraLocation=Camera->GetCameraLocation();
    View.CameraRight=Camera->GetCameraRotation().RotateVector(FVector::RightVector);
    View.Width=FMath::Max(1500.f,Camera->GetCameraCacheView().OrthoWidth);
}

void UFrontlineAudioSubsystem::UpdateEmitter(int32 Index,FSpatialMixViewV834* SharedView)
{
    if (!Components.IsValidIndex(Index) || !Components[Index]) return;
    UAudioComponent* Component = Components[Index];
    const FFieldEmitter& Emitter = Emitters[Index];
    float Gain = Emitter.Gain * Volumes[0] * GetVolume(Emitter.Bus);
    if (Emitter.Bus == EFrontlineAudioBus::Effects)
        Gain *= Index>=TankSlotStart?FMath::Sqrt(EffectsDuck):EffectsDuck;
    if (Emitter.Bus == EFrontlineAudioBus::Music)
    {
        Gain *= MusicGain;
        if(OutcomeEncounterV45.IsValid()&&GetWorld())
            Gain *= OutcomeMusicHandoffV51(GetWorld()->GetRealTimeSeconds()-OutcomeStartedV45);
    }
    if (Index == RainSlot) Gain *= RainAmount;
    if (Emitter.bSpatial)
    {
        FSpatialMixViewV834 ImmediateView;
        FSpatialMixViewV834& View=SharedView?*SharedView:ImmediateView;
        if(!View.bCaptured)CaptureSpatialMixViewV834(View);
        if (View.bHasController)
        {
            const float Pan = View.Pan(Emitter.Position);
            // A high RTS camera is not a physical microphone 140 m above the men.
            // Project world-to-screen X onto an audible listener-relative plane.
            Component->SetWorldLocation(View.Listener + View.Forward*170.f + View.Right*(Pan*310.f));
            Gain *= FMath::Lerp(1.f, .70f, FMath::Abs(Pan));
            if(Index>=AircraftSlotStart)
            {
                const double Range=FVector::Dist(Emitter.Position,View.AircraftFocus);
                Gain*=float(1./(1.+FMath::Square(Range/3600.)));
            }
            const float Offscreen=FMath::Max(0.f,View.Horizontal(Emitter.Position)-View.Width*.4f);
            const float DistanceFactor=1.f/(1.f+FMath::Square(Offscreen/3600.f));
            Gain*=DistanceFactor;
            Component->SetLowPassFilterEnabled(true);
            const float NearCutoff = Emitter.bCasualty ? 4200.f : Emitter.Bus==EFrontlineAudioBus::Voices ? 7200.f : 18000.f;
            Component->SetLowPassFilterFrequency(FMath::Lerp(2800.f,NearCutoff,DistanceFactor));
        }
    }
    Component->SetVolumeMultiplier(bMuted ? 0.f : Gain);
}

void UFrontlineAudioSubsystem::RefreshMix()
{
    FSpatialMixViewV834 View;
    for (int32 I = 0; I < Components.Num(); ++I) UpdateEmitter(I,&View);
}

void UFrontlineAudioSubsystem::RunV834SpatialMixChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const auto* Battle=GetWorld()?GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>():nullptr;
    const bool Safe=Battle&&Battle->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV834Review"));
    Check(TEXT("v834_audio_disposable_review_only"),Safe);if(!Safe)return;
    LoadBank();
    auto* PC=UGameplayStatics::GetPlayerController(GetWorld(),0);
    const auto* Camera=PC?PC->PlayerCameraManager.Get():nullptr;
    Check(TEXT("v834_audio_actual_controller_and_pool"),Camera&&Components.Num()==TotalSlots);
    if(!Camera||Components.Num()!=TotalSlots)return;
    FVector Listener,Forward,Right;PC->GetAudioListenerPosition(Listener,Forward,Right);
    const FVector CameraRight=Camera->GetCameraRotation().RotateVector(FVector::RightVector),Origin=Camera->GetCameraLocation();
    const float Width=FMath::Max(1500.f,Camera->GetCameraCacheView().OrthoWidth);
    FSpatialMixViewV834 View;
    bool PanEqual=true,LocationEqual=true,GainEqual=true,LowpassInputsEqual=true,ImmediateEqual=true,SourceInputsEqual=true;
    int32 Samples=0;
    const int32 Slots[]={0,EffectSlots,TankSlotStart,AircraftSlotStart};
    for(int32 Slot:Slots)
    {
        const FFieldEmitter Saved=Emitters[Slot];auto* Component=Components[Slot].Get();
        const FVector SavedLocation=Component->GetComponentLocation();const float SavedVolume=Component->VolumeMultiplier;
        for(int32 P=0;P<12;++P)
        {
            FFieldEmitter& E=Emitters[Slot];E.bSpatial=true;E.bCasualty=P%2!=0;E.Gain=.47f;
            E.Bus=Slot==EffectSlots?EFrontlineAudioBus::Voices:EFrontlineAudioBus::Effects;
            E.Position=Origin+CameraRight*((P-5.5f)*Width*.37f)+Forward*(P*167.f)+FVector(0,0,P*83.f);
            const float LegacyPan=GetPanForPosition(E.Position);
            const float Horizontal=FMath::Abs(float(FVector::DotProduct(E.Position-Origin,CameraRight)));
            UpdateEmitter(Slot,&View);++Samples;
            PanEqual&=View.Pan(E.Position)==LegacyPan;
            LocationEqual&=Component->GetComponentLocation().Equals(Listener+Forward*170.f+Right*(LegacyPan*310.f),.000001);
            FVector CurrentListener,CurrentForward,CurrentRight;
            PC->GetAudioListenerPosition(CurrentListener,CurrentForward,CurrentRight);
            SourceInputsEqual&=View.bHasController==(UGameplayStatics::GetPlayerController(GetWorld(),0)!=nullptr)
                &&View.bHasCamera==(PC->PlayerCameraManager!=nullptr)
                &&View.Listener==CurrentListener&&View.Forward==CurrentForward&&View.Right==CurrentRight
                &&View.CameraLocation==Camera->GetCameraLocation()
                &&View.CameraRight==Camera->GetCameraRotation().RotateVector(FVector::RightVector)
                &&View.Width==FMath::Max(1500.f,Camera->GetCameraCacheView().OrthoWidth)
                &&View.AircraftFocus==AircraftListeningFocusV8218(Camera);
            LowpassInputsEqual&=View.Horizontal(E.Position)==Horizontal&&View.Width==Width;
            const FVector CachedLocation=Component->GetComponentLocation();const float CachedVolume=Component->VolumeMultiplier;
            UpdateEmitter(Slot); // Independent playback events must capture the current listener, not reuse the pass.
            // Compare the real routine with shared versus fresh inputs. A
            // duplicate gain formula under /fp:fast can differ by one ULP
            // solely from compiler reassociation; it is not this change's oracle.
            uint32 CachedBits=0,FreshBits=0;
            FMemory::Memcpy(&CachedBits,&CachedVolume,sizeof(CachedBits));
            FMemory::Memcpy(&FreshBits,&Component->VolumeMultiplier,sizeof(FreshBits));
            if(GainEqual&&CachedBits!=FreshBits)
                Check(FString::Printf(TEXT("v834_audio_first_shared_fresh_gain_mismatch_slot_%d_probe_%d_cached_%08x_fresh_%08x"),Slot,P,CachedBits,FreshBits),false);
            GainEqual&=CachedBits==FreshBits;
            ImmediateEqual&=Component->GetComponentLocation().Equals(CachedLocation,.000001)&&CachedBits==FreshBits;
        }
        Emitters[Slot]=Saved;UpdateEmitter(Slot);Component->SetWorldLocation(SavedLocation);Component->SetVolumeMultiplier(SavedVolume);
    }
    Check(TEXT("v834_audio_pan_exactly_matches_legacy"),PanEqual);
    Check(TEXT("v834_audio_actual_component_positions_match_legacy"),LocationEqual);
    Check(TEXT("v834_audio_snapshot_complete_listener_camera_aircraft_inputs_match_original_getters_exactly"),SourceInputsEqual);
    Check(TEXT("v834_audio_actual_effect_voice_tank_aircraft_gains_shared_vs_fresh_bitwise_equal"),GainEqual);
    Check(TEXT("v834_audio_offscreen_and_lowpass_inputs_exactly_match_legacy"),LowpassInputsEqual);
    Check(TEXT("v834_audio_immediate_events_use_fresh_equivalent_listener"),ImmediateEqual);
    Check(FString::Printf(TEXT("v834_audio_shared_view_captures_%d_for_%d_updates"),View.Captures,Samples),View.Captures==1&&Samples==48);
    FSpatialMixViewV834 Absent;
    Check(TEXT("v834_audio_no_camera_pan_and_horizontal_fallback_unchanged"),Absent.Pan(FVector(1730,-381,215))==0.f&&Absent.Horizontal(FVector(1730,-381,215))==0.f&&Absent.Width==6000.f);
    const bool NoSave=FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
    Check(TEXT("v834_audio_settings_review_requires_no_save"),NoSave);
    if(NoSave)
    {
        float SavedVolumes[5];for(int32 I=0;I<5;++I)SavedVolumes[I]=Volumes[I];
        const bool SavedMuted=bMuted,SavedDirty=bSettingsDirtyV834;
        const double SavedAt=SettingsSaveAtV834;
        FString ConfigBefore[6],ConfigAfter[6];
        for(int32 I=0;I<6;++I)if(GConfig)GConfig->GetString(ConfigSection,I<5?BusKeys[I]:TEXT("Muted"),ConfigBefore[I],GGameUserSettingsIni);
        // Repeated or clamped-identical controls leave the existing debounce
        // deadline alone. Exercise the real setter rather than a timing copy.
        bSettingsDirtyV834=true;SettingsSaveAtV834=FPlatformTime::Seconds()+10.;
        const double Deadline=SettingsSaveAtV834;
        SetVolume(EFrontlineAudioBus::Master,Volumes[0]);SetMuted(bMuted);
        Check(TEXT("v834_audio_identical_setters_do_not_restart_save_deadline"),bSettingsDirtyV834&&SettingsSaveAtV834==Deadline);
        // Even an already-dirty forced shutdown must not write in NoSave mode.
        SaveSettings(true);
        Check(TEXT("v834_audio_no_save_cancels_dirty_shutdown_flush"),!bSettingsDirtyV834&&SettingsSaveAtV834==0.);
        SetVolume(EFrontlineAudioBus::Master,SavedVolumes[0]>.5f?.37f:.73f);
        SetMuted(true);
        bool ImmediateMute=true;for(const auto& C:Components)if(C)ImmediateMute&=C->VolumeMultiplier==0.f;
        Check(TEXT("v834_audio_mute_refreshes_actual_pool_immediately"),bMuted&&ImmediateMute);
        SetMuted(false);SetVolume(EFrontlineAudioBus::Master,.41f);
        SaveSettings(true);
        bool ConfigUnchanged=true;
        for(int32 I=0;I<6;++I)if(GConfig)
        {GConfig->GetString(ConfigSection,I<5?BusKeys[I]:TEXT("Muted"),ConfigAfter[I],GGameUserSettingsIni);ConfigUnchanged&=ConfigAfter[I]==ConfigBefore[I];}
        Check(TEXT("v834_audio_no_save_setters_never_dirty_or_change_config"),!bSettingsDirtyV834&&SettingsSaveAtV834==0.&&ConfigUnchanged);
        for(int32 I=0;I<5;++I)Volumes[I]=SavedVolumes[I];
        bMuted=SavedMuted;bSettingsDirtyV834=SavedDirty;SettingsSaveAtV834=SavedAt;RefreshMix();
    }
}

void UFrontlineAudioSubsystem::Tick(float DeltaTime)
{
    SaveSettings();
    if(!GetWorld()||!GetWorld()->HasBegunPlay())return;
    DeltaTime=FMath::Clamp(float(FApp::GetDeltaTime()),0.f,.1f);
    LoadBank();
    const auto* Battle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    const bool Fighting=Battle&&Battle->Phase==EFrontlinePhase::Fighting;
    const bool Result=Battle&&(Battle->Phase==EFrontlinePhase::Victory||Battle->Phase==EFrontlinePhase::Defeat);
    const float TargetMusic=(Fighting?.65f:.85f)*(GetActiveVoices()?(Result?.80f:.55f):1.f);
    MusicGain=FMath::FInterpTo(MusicGain,TargetMusic,DeltaTime,1.4f);
    TickMusicV31(float(FApp::GetDeltaTime()));
    TickPendingBurnV44();
    TickOutcomeV45();
    const bool Relief=Result&&OutcomeEncounterV45.IsValid()&&OutcomePlayedV45>0
        &&GetWorld()->GetRealTimeSeconds()-OutcomeStartedV45<6.9;
    EffectsDuck = FMath::FInterpTo(EffectsDuck, Relief?.37f:GetActiveVoices()?.62f:1.f, DeltaTime, 7.f);
    FSpatialMixViewV834 View;
    for (int32 I = 0; I < Components.Num(); ++I)
        if (Components[I] && Components[I]->IsPlaying()) UpdateEmitter(I,&View);
    for(int32 I=TankSlotStart;I<Components.Num();++I)
        if(Emitters[I].VehicleId!=INDEX_NONE && !GetWorld()->IsPaused()
            && GetWorld()->GetRealTimeSeconds()-Emitters[I].LastVehicleUpdate>.5)
        {Components[I]->Stop();Emitters[I].VehicleId=INDEX_NONE;}
    // A zero-volume loop can be culled by a platform mixer; resume it on unmute.
    if (bBattleActive && !bMuted && Volumes[0] > 0.f && Volumes[3] > 0.f && Components.IsValidIndex(AmbienceSlot)
        && !Components[AmbienceSlot]->IsPlaying())
        StartSound(TEXT("S_Ambience_Frontline_V18"), EFrontlineAudioBus::Ambience, FVector::ZeroVector, 1.4f);
    if(bBattleActive&&!bMuted&&RainAmount>.015f&&Volumes[0]>0.f&&Volumes[3]>0.f&&Components.IsValidIndex(RainSlot)&&!Components[RainSlot]->IsPlaying())
        StartSound(TEXT("S_Rain_Field_V19"),EFrontlineAudioBus::Ambience,FVector::ZeroVector,1.3f);
    if(!bBattleActive||GetWorld()->IsPaused())return;
    const double Now=GetWorld()->GetRealTimeSeconds();
    // One bounded scan, with individual gait cadence; footsteps never steal a voice.
    if(Battle&&Now>=NextFootstepScan)
    {
        NextFootstepScan=Now+.12;
        for(const AWarfareFrontlineSoldier* Unit:Battle->Units)
            if(Unit&&Unit->IsAlive()&&Unit->bMoving)PlayFootstep(Unit->Feet(),Unit->SquadId*16+Unit->MemberIndex);
    }
}

int32 UFrontlineAudioSubsystem::GetActiveEffects() const
{
    int32 Count = 0;
    for (int32 I = 0; I < FMath::Min(EffectSlots, Components.Num()); ++I) if (Components[I] && Components[I]->IsPlaying()) ++Count;
    return Count;
}

int32 UFrontlineAudioSubsystem::GetActiveMusicPlayersV75() const
{
    int32 Count=0;for(int32 I=MusicSlot;I<FMath::Min(RainSlot,Components.Num());++I)
        if(Components[I]&&Components[I]->IsPlaying())++Count;
    return Count;
}
int32 UFrontlineAudioSubsystem::GetActiveVoices() const
{
    int32 Count = 0;
    for (int32 I = EffectSlots; I < FMath::Min(AmbienceSlot, Components.Num()); ++I) if (Components[I] && Components[I]->IsPlaying()) ++Count;
    return Count;
}

int32 UFrontlineAudioSubsystem::ChooseVariant(const FString& Key, int32 Count)
{
    int32& Previous = LastVariant.FindOrAdd(Key);
    if (Count <= 1) return Previous = 1;
    int32 Next = FMath::RandRange(1, Count-1);
    if (Next >= Previous && Previous > 0) ++Next;
    Previous = Next;
    return Next;
}

bool UFrontlineAudioSubsystem::StartSound(FName Name, EFrontlineAudioBus Bus, const FVector& Position, float Gain, float Pitch, bool bCasualty, uint8 VoicePriority,bool bOutcome,bool bOutcomeSpeech)
{
    const TObjectPtr<USoundWave>* Found = Sounds.Find(Name);
    if (!Found || !*Found || Components.Num() != TotalSlots) return false;
    int32 First = 0, Last = EffectSlots;
    if (Bus == EFrontlineAudioBus::Voices)
    {
        if (!HasVoiceSlot(GetActiveVoices(), VoicePriority)) return false;
        First = EffectSlots; Last = AmbienceSlot;
    }
    else if (Bus == EFrontlineAudioBus::Ambience) { First = AmbienceSlot; Last = MusicSlot; }
    else if (Bus == EFrontlineAudioBus::Music) { First = MusicSlot; Last = RainSlot; }
    if(Name==TEXT("S_Rain_Field_V19")) {First=RainSlot;Last=RainSlot+1;}
    int32 Slot = INDEX_NONE;
    for (int32 I = First; I < Last; ++I)
        if (!Components[I]->IsPlaying()) { Slot = I; break; }
    if (Slot == INDEX_NONE)
    {
        if (Bus == EFrontlineAudioBus::Effects) ++EffectsDropped;
        return false; // Never truncate speech or a transient that has already begun.
    }
    FFieldEmitter& Emitter = Emitters[Slot];
    Emitter.Position = Position;
    Emitter.Bus = Bus;
    Emitter.Gain = Gain;
    Emitter.Started = GetWorld()->GetRealTimeSeconds();
    Emitter.VoicePriority = VoicePriority;
    Emitter.bCasualty = bCasualty;
    Emitter.bOutcomeV45=bOutcome;Emitter.bOutcomeSpeechV45=bOutcomeSpeech;
    // The authored stereo ensemble has its field perspective already mixed.
    // Do not collapse it to a moving point or leave the prior voice low-pass on.
    Emitter.bSpatial = !bOutcome && (Bus == EFrontlineAudioBus::Effects || Bus == EFrontlineAudioBus::Voices);
    UAudioComponent* Component = Components[Slot];
    Component->bAllowSpatialization=Emitter.bSpatial;
    Component->SetLowPassFilterEnabled(Emitter.bSpatial);
    Component->SetSound(*Found);
    Component->SetPitchMultiplier(Pitch);
    UpdateEmitter(Slot);
    Component->Play();
    return true;
}

void UFrontlineAudioSubsystem::PlayWeapon(bool bAutomatic, bool bGerman, const FVector& Position, int32 SquadId)
{
    if (!bBattleActive || !GetWorld() || GetWorld()->IsPaused()) return;
    const FString Family = FString::Printf(TEXT("%s_%s"), bAutomatic ? TEXT("Automatic") : TEXT("Rifle"), bGerman ? TEXT("German") : TEXT("British"));
    const int32 Variant = ChooseVariant(Family + FString::FromInt(SquadId), 2);
    FName Name(*FString::Printf(TEXT("S_%s_%02d_V18"), *Family, Variant));
    if(!Sounds.Contains(Name))Name=FName(*FString::Printf(TEXT("S_%s_%02d_V14"),*Family,Variant));
    // Automatic reports gain 2 dB in V84.4.11. The same density headroom,
    // stereo distance treatment and recorded attack remain in force.
    const float Density = FMath::Clamp(1.f - .014f * GetActiveEffects(), .68f, 1.f);
    if (StartSound(Name, EFrontlineAudioBus::Effects, Position, (bAutomatic ? .29f*MechanicalPresenceV84411 : .4488f)*Density, FMath::FRandRange(.985f, 1.015f))) ++WeaponEvents;
}

void UFrontlineAudioSubsystem::PlayHeavyWeaponV8215(bool bGerman,const FVector& Position,int32 SquadId)
{
    if(!bBattleActive||!GetWorld()||GetWorld()->IsPaused()||Position.ContainsNaN())return;
    const FString Family=bGerman?TEXT("Rifle_German"):TEXT("Rifle_British");
    const int32 Variant=ChooseVariant(Family+FString::FromInt(SquadId),2);
    FName Name(*FString::Printf(TEXT("S_%s_%02d_V18"),*Family,Variant));
    if(!Sounds.Contains(Name))Name=FName(*FString::Printf(TEXT("S_%s_%02d_V14"),*Family,Variant));
    // The same accepted rifle recording gets a deeper, heavier report. A
    // cannon/explosion layer would misrepresent the non-explosive AP round.
    const float Density=FMath::Clamp(1.f-.014f*GetActiveEffects(),.68f,1.f);
    if(StartSound(Name,EFrontlineAudioBus::Effects,Position,.88f*Density,.78f))++WeaponEvents;
}

void UFrontlineAudioSubsystem::PlayImpact(bool bGrenade, const FVector& Position)
{
    if (!bBattleActive || !GetWorld() || GetWorld()->IsPaused()) return;
    // Count actual impact events before voice-pool admission, independently of
    // volume/mute and without observing fire-support button presses.
    MusicBlastEventsV31+=bGrenade?1:2;
    const FString Family=bGrenade?TEXT("Grenade"):TEXT("Shell");
    FName Name(*FString::Printf(TEXT("S_%s_01_V30"),*Family));
    if(!Sounds.Contains(Name))Name=FName(*FString::Printf(TEXT("S_%s_01_V18"),*Family));
    if(!Sounds.Contains(Name))Name=bGrenade?TEXT("S_Grenade_V14"):TEXT("S_Shell_V14");
    if (StartSound(Name, EFrontlineAudioBus::Effects, Position, bGrenade ? .58f : .74f)) ++ImpactEvents;
}

void UFrontlineAudioSubsystem::PlayIncomingShell(const FVector& Position)
{
    if(bBattleActive&&GetWorld()&&!GetWorld()->IsPaused())
        StartSound(TEXT("S_IncomingShell_01_V18"),EFrontlineAudioBus::Effects,Position,.51f);
}

bool UFrontlineAudioSubsystem::ArmourContactAssetsReadyV837()const
{return ArmourContactAssetsReadyV839();}
bool UFrontlineAudioSubsystem::ArmourContactAssetsReadyV839()const
{
    for(const TCHAR* Family:{TEXT("Rifle"),TEXT("Grenade"),TEXT("Cannon")})for(int32 V=1;V<=3;++V)
        if(!Sounds.Contains(FName(*FString::Printf(TEXT("S_Armour_%s_%02d_V84411"),Family,V))))return false;
    return true;
}
bool UFrontlineAudioSubsystem::PlayArmourContactV837(const FVector& Position,float Strength,bool bExplosive)
{return PlayArmourContactV839(Position,Strength,FrontlineArmourAudioV839::Resolve(EFrontlineArmourContactSoundV839::Automatic,bExplosive));}
bool UFrontlineAudioSubsystem::PlayArmourContactV839(const FVector& Position,float Strength,EFrontlineArmourContactSoundV839 Kind)
{
    if(!bBattleActive||!GetWorld()||GetWorld()->IsPaused()||Position.ContainsNaN()||!FMath::IsFinite(Strength)||Strength<=0)return false;
    if(Kind<EFrontlineArmourContactSoundV839::Rifle||Kind>EFrontlineArmourContactSoundV839::Cannon)return false;
    const int32 Index=int32(Kind)-1;const bool Explosive=Kind!=EFrontlineArmourContactSoundV839::Rifle;
    const FString Family=FrontlineArmourAudioV839::Family(Kind);
    const int32 Variant=ChooseVariant(TEXT("ArmourContactV839")+Family,3);
    const FName Name(*FString::Printf(TEXT("S_Armour_%s_%02d_V84411"),*Family,Variant));
    ++ArmourContactRequestsV839[Index];
    const float Density=FMath::Clamp(1.f-.01f*GetActiveEffects(),.72f,1.f);
    const bool Played=StartSound(Name,EFrontlineAudioBus::Effects,Position,
        FMath::Lerp(.35f,Explosive?.63f:.52f,FMath::Clamp(Strength,0.f,1.f))*Density,
        FMath::FRandRange(.98f,1.02f));
    if(Played){++ArmourContactEventsV837;++ArmourContactPlayedV839[Index];LastArmourContactSoundV839=Name;
        LastArmourContactRealTimeV839=GetWorld()->GetRealTimeSeconds();LastArmourContactGameTimeV839=GetWorld()->TimeSeconds;}
    return Played;
}
TSharedRef<FJsonObject> UFrontlineAudioSubsystem::DescribeArmourContactV839() const
{
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Families;int32 Active=0;
    for(int32 I=0;I<3;++I)
    {
        const auto Kind=EFrontlineArmourContactSoundV839(I+1);auto R=MakeShared<FJsonObject>();
        R->SetStringField(TEXT("family"),FrontlineArmourAudioV839::Family(Kind));
        R->SetNumberField(TEXT("requests"),ArmourContactRequestsV839[I]);R->SetNumberField(TEXT("admitted"),ArmourContactPlayedV839[I]);
        TArray<TSharedPtr<FJsonValue>> Assets;
        for(int32 V=1;V<=3;++V)
        {
            const FName Name(*FString::Printf(TEXT("S_Armour_%s_%02d_V84411"),FrontlineArmourAudioV839::Family(Kind),V));
            auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("name"),Name.ToString());
            const auto* W=Sounds.Find(Name);A->SetBoolField(TEXT("loaded"),W&&*W);
            if(W&&*W){A->SetStringField(TEXT("actual_wave"),(*W)->GetPathName());A->SetNumberField(TEXT("duration_seconds"),(*W)->Duration);
                A->SetBoolField(TEXT("looping"),(*W)->bLooping);A->SetNumberField(TEXT("channels"),(*W)->NumChannels);}
            Assets.Add(MakeShared<FJsonValueObject>(A));
        }
        R->SetArrayField(TEXT("assets"),Assets);Families.Add(MakeShared<FJsonValueObject>(R));
    }
    for(const auto& C:Components)if(C&&C->IsPlaying()&&C->Sound&&C->Sound->GetName().StartsWith(TEXT("S_Armour_"))
        &&C->Sound->GetName().EndsWith(TEXT("_V84411")))++Active;
    J->SetArrayField(TEXT("families"),Families);J->SetNumberField(TEXT("actual_active_contact_components"),Active);
    J->SetBoolField(TEXT("assets_ready"),ArmourContactAssetsReadyV839());J->SetStringField(TEXT("last_admitted_sound"),LastArmourContactSoundV839.ToString());
    J->SetNumberField(TEXT("last_admitted_real_seconds"),LastArmourContactRealTimeV839);J->SetNumberField(TEXT("last_admitted_game_seconds"),LastArmourContactGameTimeV839);
    J->SetStringField(TEXT("scope"),TEXT("Actual SoundWaves and admitted production contact events. Short CC0 recorded-steel Foley, damped without synthesized ringing or noise body; numerical checks do not establish listening quality."));
    return J;
}
void UFrontlineAudioSubsystem::RunArmourAudioChecksV839(TFunctionRef<void(const FString&,bool)> Check)
{
    LoadBank();bool Finite=ArmourContactAssetsReadyV839();TSet<const USoundWave*> Distinct;
    for(const TCHAR* Family:{TEXT("Rifle"),TEXT("Grenade"),TEXT("Cannon")})for(int32 V=1;V<=3;++V)
    {
        const auto* W=Sounds.Find(FName(*FString::Printf(TEXT("S_Armour_%s_%02d_V84411"),Family,V)));
        if(!W||!*W){Finite=false;continue;}
        Finite&=!(*W)->bLooping&&(*W)->NumChannels==2&&FMath::IsFinite((*W)->Duration)&&(*W)->Duration>=.20f&&(*W)->Duration<=.40f
            &&(*W)->GetPathName().StartsWith(TEXT("/Game/Warfare/Audio/V84411/"));
        Distinct.Add(W->Get());
    }
    Check(TEXT("v839_nine_distinct_actual_short_nonlooping_armour_waves"),Finite&&Distinct.Num()==9);
    bool Admitted=true;for(int32 I=0;I<3;++I)Admitted&=ArmourContactPlayedV839[I]<=ArmourContactRequestsV839[I];
    Check(TEXT("v839_armour_audio_actual_events_require_effect_pool_admission"),Admitted);
    Check(TEXT("v84411_machinegun_track_aircraft_gain_change_is_two_decibels"),
        FMath::IsNearlyEqual(20.f*FMath::LogX(10.f,MechanicalPresenceV84411),2.f,.001f));
}

void UFrontlineAudioSubsystem::PlayFootstep(const FVector& Position,int32 SoldierId)
{
    if(!bBattleActive||!GetWorld()||GetActiveEffects()>20)return;
    const double Now=GetWorld()->GetRealTimeSeconds();
    if(Now<NextStepTime.FindRef(SoldierId))return;
    NextStepTime.Add(SoldierId,Now+.42+(SoldierId%7)*.017);
    const int32 Variant=ChooseVariant(TEXT("Step")+FString::FromInt(SoldierId),2);
    StartSound(FName(*FString::Printf(TEXT("S_Step_Mud_%02d_V18"),Variant)),EFrontlineAudioBus::Effects,Position,.075f);
}

void UFrontlineAudioSubsystem::UpdateTankAudio(int32 TankId,const FVector& Position,float SpeedFraction,bool bTracksIntact,bool bBurning,bool bActive)
{
    if(TankId==INDEX_NONE || !GetWorld() || Position.ContainsNaN())return;
    if(!bBattleActive)bActive=false;
    if(bActive)LoadBank();
    if(Components.Num()!=TotalSlots)return;
    int32 Pair=INDEX_NONE;
    for(int32 I=TankSlotStart;I<AircraftSlotStart;I+=2)if(Emitters[I].VehicleId==TankId){Pair=I;break;}
    if(Pair==INDEX_NONE && bActive && !bBurning)
        for(int32 I=TankSlotStart;I<AircraftSlotStart;I+=2)if(Emitters[I].VehicleId==INDEX_NONE){Pair=I;break;}
    if(Pair==INDEX_NONE && bActive && !bBurning)
    {
        // Four real vehicle voices remain the budget. A near-camera arrival may
        // replace a clearly less audible off-screen tank, rather than allowing
        // the first four spawns to monopolize every engine for the whole match.
        const APlayerController* PC=UGameplayStatics::GetPlayerController(GetWorld(),0);
        const APlayerCameraManager* Camera=PC?PC->PlayerCameraManager:nullptr;
        if(Camera)
        {
            const FVector Right=Camera->GetCameraRotation().RotateVector(FVector::RightVector);
            auto Distance=[&](const FVector& P){return FMath::Abs(FVector::DotProduct(P-Camera->GetCameraLocation(),Right));};
            const double Candidate=Distance(Position);double Farthest=Candidate+FMath::Max(600.f,Camera->GetCameraCacheView().OrthoWidth*.12f);
            for(int32 I=TankSlotStart;I<AircraftSlotStart;I+=2)
                if(Distance(Emitters[I].Position)>Farthest){Farthest=Distance(Emitters[I].Position);Pair=I;}
            if(Pair!=INDEX_NONE)for(int32 I=Pair;I<Pair+2;++I){Components[I]->Stop();Emitters[I].VehicleId=INDEX_NONE;}
        }
    }
    if(Pair==INDEX_NONE)return;
    const float Speed=FMath::IsFinite(SpeedFraction)?FMath::Clamp(SpeedFraction,0.f,1.f):0.f;
    for(int32 Part=0;Part<2;++Part)
    {
        const int32 I=Pair+Part;auto* Component=Components[I].Get();auto& E=Emitters[I];
        if(!bActive || bBurning)
        {Component->Stop();E.VehicleId=INDEX_NONE;continue;}
        E.VehicleId=TankId;E.LastVehicleUpdate=GetWorld()->GetRealTimeSeconds();E.Position=Position;
        E.Bus=EFrontlineAudioBus::Effects;E.bSpatial=true;E.bCasualty=false;E.VoicePriority=0;
        const bool Audible=Part==0 || TracksAudibleV828(bTracksIntact,Speed);
        E.Gain=VehicleGainV828(Part,Speed);
        if(!Audible){Component->Stop();continue;}
        if(!Component->IsPlaying())
        {
            const FName Name(Part==0?TEXT("S_Tank_EngineLoop_V28"):TEXT("S_Tank_TracksLoop_V28"));
            const auto* Sound=Sounds.Find(Name);if(!Sound || !*Sound)continue;
            Component->SetSound(*Sound);UpdateEmitter(I);Component->Play();
        }
        // Only mechanical rotation follows speed; spoken voices stay at 1.0.
        Component->SetPitchMultiplier(Part==0?FMath::Lerp(.94f,1.13f,Speed):FMath::Lerp(.78f,1.18f,Speed));
        UpdateEmitter(I);
    }
}

void UFrontlineAudioSubsystem::SetMenuMusicNationV828(int32 Nation)
{
    // UI intent expires with the selector; never persist it as gameplay state.
    MenuMusicNationV828=Nation>=0&&Nation<4?Nation:INDEX_NONE;
}

void UFrontlineAudioSubsystem::RunV828AudioChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Allowed=FParse::Param(FCommandLine::Get(),TEXT("WFV828Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
    Check(TEXT("v828_audio_requires_isolated_review"),Allowed);if(!Allowed)return;
    const bool FrenchAvailable=WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::French),RussianAvailable=WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::Russian);
    LoadBank();RunFrontlineMusicV828PolicyChecks(Check);
    TSet<const USoundWave*> Waves;bool Valid=true;
    for(int32 Nation=0;Nation<4;++Nation)for(int32 I=0;I<3;++I)
    {
        const FName Name=FFrontlineMusicDirectorV31::NationalAssetNameV828(EFrontlineMusicCueV31(I),Nation);
        const auto* Found=Sounds.Find(Name);const USoundWave* Wave=Found?Found->Get():nullptr;
        Valid&=Wave&&Wave->NumChannels==2&&Wave->bLooping&&FMath::IsNearlyEqual(Wave->Duration,64.f,.005f);
        if(Wave)Waves.Add(Wave);
    }
    Check(TEXT("v8211_audio_only_accepted_score_bound_for_all_nations"),Valid&&Waves.Num()==3);
    Check(TEXT("v828_audio_two_music_four_vehicle_decoder_budgets_preserved"),MusicPlayersV31==2&&TankAudioVehicles==4);
    Check(TEXT("v828_audio_tracks_stop_when_stationary_or_disabled"),!TracksAudibleV828(true,0.f)&&!TracksAudibleV828(false,1.f)&&TracksAudibleV828(true,.1f));
    Check(TEXT("v84411_audio_tracks_retain_previous_balance_plus_two_decibels"),20.f*FMath::LogX(10.f,VehicleGainV828(1,1.f)/.24f)>6.f
        &&20.f*FMath::LogX(10.f,VehicleGainV828(1,1.f)/.24f)<6.6f);
    const bool OldMute=bMuted;const float OldMaster=Volumes[0],OldMusic=Volumes[4],OldGain=MusicGain,OldDuck=EffectsDuck;
    const FFieldEmitter OldEmitter=Emitters[MusicSlot];const FFieldEmitter OldVehicle=Emitters[TankSlotStart+1];
    bMuted=false;Volumes[0]=.8f;Volumes[4]=.45f;MusicGain=.65f*.55f;EffectsDuck=.62f;
    Emitters[MusicSlot].Bus=EFrontlineAudioBus::Music;Emitters[MusicSlot].Gain=1.8f;Emitters[MusicSlot].bSpatial=false;
    UpdateEmitter(MusicSlot);
    const float OutcomeFactor=OutcomeEncounterV45.IsValid()?OutcomeMusicHandoffV51(GetWorld()->GetRealTimeSeconds()-OutcomeStartedV45):1.f;
    Check(TEXT("v828_audio_actual_music_emitter_honors_moderate_voice_duck"),FMath::IsNearlyEqual(Components[MusicSlot]->VolumeMultiplier,.8f*.45f*1.8f*.65f*.55f*OutcomeFactor,.0001f));
    Emitters[TankSlotStart+1].Bus=EFrontlineAudioBus::Effects;Emitters[TankSlotStart+1].Gain=.4f;Emitters[TankSlotStart+1].bSpatial=false;
    UpdateEmitter(TankSlotStart+1);
    Check(TEXT("v828_audio_actual_tracks_retain_body_during_speech"),FMath::IsNearlyEqual(Components[TankSlotStart+1]->VolumeMultiplier,.4f*.8f*Volumes[1]*FMath::Sqrt(.62f),.0001f));
    bMuted=true;UpdateEmitter(MusicSlot);UpdateEmitter(TankSlotStart+1);
    Check(TEXT("v828_audio_mute_zeroes_actual_music_and_tracks"),Components[MusicSlot]->VolumeMultiplier==0.f&&Components[TankSlotStart+1]->VolumeMultiplier==0.f);
    bMuted=OldMute;Volumes[0]=OldMaster;Volumes[4]=OldMusic;MusicGain=OldGain;EffectsDuck=OldDuck;
    Emitters[MusicSlot]=OldEmitter;Emitters[TankSlotStart+1]=OldVehicle;RefreshMix();
    Check(TEXT("v828_audio_preview_preserves_each_national_campaign_gate"),FrenchAvailable==WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::French)
        &&RussianAvailable==WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::Russian));
}

void UFrontlineAudioSubsystem::PlayTankCannon(const FVector& Position)
{
    if(!bBattleActive || !GetWorld() || GetWorld()->IsPaused())return;
    if(StartSound(Sounds.Contains(TEXT("S_Tank_Cannon_01_V30"))?TEXT("S_Tank_Cannon_01_V30"):TEXT("S_Tank_Cannon_01_V28"),EFrontlineAudioBus::Effects,Position,.65f))++WeaponEvents;
}

void UFrontlineAudioSubsystem::UpdateAircraftAudio(int32 AircraftId,const FVector& Position,float SpeedFraction,bool bActive,const FVector& Velocity)
{
    if(AircraftId==INDEX_NONE || !GetWorld() || Position.ContainsNaN())return;
    if(!bBattleActive)bActive=false;
    if(bActive)LoadBank();if(Components.Num()!=TotalSlots)return;
    int32 Slot=INDEX_NONE;
    for(int32 I=AircraftSlotStart;I<TotalSlots;++I)if(Emitters[I].VehicleId==AircraftId){Slot=I;break;}
    if(Slot==INDEX_NONE && bActive)
        for(int32 I=AircraftSlotStart;I<TotalSlots;++I)if(Emitters[I].VehicleId==INDEX_NONE){Slot=I;break;}
    if(Slot==INDEX_NONE)return;
    auto* Component=Components[Slot].Get();auto& E=Emitters[Slot];
    if(!bActive){Component->Stop();E.VehicleId=INDEX_NONE;return;}
    E.VehicleId=AircraftId;E.LastVehicleUpdate=GetWorld()->GetRealTimeSeconds();E.Position=Position;
    E.AircraftVelocityV8218=Velocity.ContainsNaN()?FVector::ZeroVector:Velocity;
    E.Bus=EFrontlineAudioBus::Effects;E.bSpatial=true;E.bCasualty=false;E.VoicePriority=0;
    const float Speed=FMath::IsFinite(SpeedFraction)?FMath::Clamp(SpeedFraction,0.f,1.f):0.f;E.Gain=MechanicalPresenceV84411*FMath::Lerp(.22f,.43f,Speed);
    if(!Component->IsPlaying())
    {
        const auto* Sound=Sounds.Find(TEXT("S_Aircraft_PropellerLoop_V28"));if(!Sound || !*Sound)return;
        Component->SetSound(*Sound);UpdateEmitter(Slot);Component->Play();
    }
    const auto* PC=UGameplayStatics::GetPlayerController(GetWorld(),0);
    const FVector Focus=AircraftListeningFocusV8218(PC?PC->PlayerCameraManager:nullptr);
    const float Radial=float(FVector::DotProduct(E.AircraftVelocityV8218,(Focus-Position).GetSafeNormal()));
    const float Doppler=FMath::Clamp(34300.f/(34300.f-FMath::Clamp(Radial,-5000.f,5000.f)),.91f,1.12f);
    Component->SetPitchMultiplier(FMath::Lerp(.97f,1.08f,Speed)*Doppler);UpdateEmitter(Slot);
}

TSharedRef<FJsonObject> UFrontlineAudioSubsystem::DescribeAircraftAudioV8218(int32 AircraftId) const
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("playing"),false);
    for(int32 I=AircraftSlotStart;I<Emitters.Num()&&I<Components.Num();++I)
        if(Emitters[I].VehicleId==AircraftId&&Components[I])
        {
            const auto* C=Components[I].Get();J->SetBoolField(TEXT("playing"),C->IsPlaying());
            J->SetStringField(TEXT("sound"),C->Sound?C->Sound->GetPathName():TEXT("missing"));
            J->SetNumberField(TEXT("gain"),C->VolumeMultiplier);J->SetNumberField(TEXT("pitch"),C->PitchMultiplier);
            J->SetNumberField(TEXT("pan"),GetPanForPosition(Emitters[I].Position));
            J->SetStringField(TEXT("physical_position"),Emitters[I].Position.ToString());
            J->SetStringField(TEXT("physical_velocity"),Emitters[I].AircraftVelocityV8218.ToString());break;
        }
    return J;
}

void UFrontlineAudioSubsystem::RunV28AudioChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Allowed=FParse::Param(FCommandLine::Get(),TEXT("WFV28Review"));
    Check(TEXT("v28_audio_isolated_entry"),Allowed);if(!Allowed)return;
    LoadBank();
    Check(TEXT("event_mask_has_capacity"),uint32(EFrontlineVoiceEvent::Count)<32);
    for(bool German:{false,true})
        for(auto Event:{EFrontlineVoiceEvent::GasAlarm,EFrontlineVoiceEvent::AircraftInbound,EFrontlineVoiceEvent::ArmorSighted,
            EFrontlineVoiceEvent::FriendlyTankAdvance,EFrontlineVoiceEvent::TracksDisabled,EFrontlineVoiceEvent::TankBurning})
            Check(FString::Printf(TEXT("%s_actual_context_%d_has_loaded_voice"),German?TEXT("de"):TEXT("en"),int32(Event)),VoiceCandidates(Event,German,VoiceSpeech).Num()>0);
    bool Ready=true,Unique=true;TSet<FName> Sources;
    for(const auto& Clip:VoiceBank)
    {Ready&=Sounds.Contains(FName(Clip.Name));Unique&=!Sources.Contains(FName(Clip.Source));Sources.Add(FName(Clip.Source));}
    Check(TEXT("active_v28_registry_all_assets_loaded"),Ready);
    Check(TEXT("one_registry_row_per_independent_source"),Unique);
    Check(TEXT("gas_and_aircraft_get_warning_priority"),IsUrgent(EFrontlineVoiceEvent::GasAlarm)&&IsUrgent(EFrontlineVoiceEvent::AircraftInbound));
    for(const TCHAR* Name:{TEXT("S_Tank_EngineLoop_V28"),TEXT("S_Tank_TracksLoop_V28"),TEXT("S_Tank_Cannon_01_V28"),TEXT("S_Aircraft_PropellerLoop_V28")})
        Check(FString(TEXT("actual_vehicle_sound_loaded_"))+Name,Sounds.Contains(FName(Name)));
    Check(TEXT("tank_loop_budget_is_separate_from_two_voice_slots"),VoiceSlots==2 && TankSlotStart>AmbienceSlot && TankAudioVehicles==4);
}

void UFrontlineAudioSubsystem::RunV30AudioChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Allowed=FParse::Param(FCommandLine::Get(),TEXT("WFV30Review"));
    Check(TEXT("v30_audio_isolated_entry"),Allowed);if(!Allowed)return;
    LoadBank();int32 NewVoices[2]={};bool RetiredAbsent=true,Ready=true,Unique=true;TSet<FName> Sources;
    for(const auto& Clip:VoiceBank)
    {
        RetiredAbsent&=FCString::Strcmp(Clip.Source,TEXT("kenney:male:war_fire_in_the_hole"))!=0;
        Ready&=Sounds.Contains(FName(Clip.Name));Unique&=!Sources.Contains(FName(Clip.Source));Sources.Add(FName(Clip.Source));
        if(FString(Clip.Name).StartsWith(TEXT("VO_V30_"))&&Clip.Language>=0&&Clip.Language<=1)++NewVoices[Clip.Language];
    }
    Check(TEXT("modern_fire_in_the_hole_source_removed_from_active_pool"),RetiredAbsent);
    Check(TEXT("new_original_bilingual_pool_has_at_least_32_performances"),NewVoices[0]>=16&&NewVoices[1]>=16);
    Check(TEXT("active_pool_actual_soundwaves_loaded_and_source_ids_unique"),Ready&&Unique);
    for(const TCHAR* Name:{TEXT("S_Shell_01_V30"),TEXT("S_Grenade_01_V30"),TEXT("S_Tank_Cannon_01_V30")})
        Check(FString(TEXT("v30_remastered_effect_loaded_"))+Name,Sounds.Contains(FName(Name)));
    for(bool German:{false,true})
        Check(FString(German?TEXT("de_"):TEXT("en_"))+TEXT("grenade_throw_warning_has_multiple_alternatives"),
            VoiceCandidates(EFrontlineVoiceEvent::GrenadeThrowWarning,German,VoiceSpeech).Num()>=2);
}

TArray<int32> UFrontlineAudioSubsystem::VoiceCandidates(EFrontlineVoiceEvent Event, int32 bGerman, uint8 Kind) const
{
    TArray<int32> Result;
    TSet<FName> Sources;
    for (int32 I = 0; I < UE_ARRAY_COUNT(VoiceBank); ++I)
    {
        const FVoiceClip& Clip = VoiceBank[I];
        const FName Source(Clip.Source);
        if (Clip.Kind != Kind || (Clip.Language >= 0 && Clip.Language != int32(bGerman))
            || (Kind == VoiceSpeech && !(Clip.Events & VEvent(Event)))
            || !Sounds.Contains(FName(Clip.Name)) || Sources.Contains(Source)) continue;
        Sources.Add(Source); Result.Add(I);
    }
    return Result;
}

int32 UFrontlineAudioSubsystem::GetLoadedVoiceClipCount() const
{
    int32 Count = 0;
    for (const FVoiceClip& Clip : VoiceBank) if (Sounds.Contains(FName(Clip.Name))) ++Count;
    return Count;
}

int32 UFrontlineAudioSubsystem::GetUniqueVoiceSourceCount() const
{
    TSet<FName> Sources;
    for (const FVoiceClip& Clip : VoiceBank)
        if (Sounds.Contains(FName(Clip.Name))) Sources.Add(FName(Clip.Source));
    return Sources.Num();
}

void UFrontlineAudioSubsystem::RunFRRUV82AudioChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    bool Allowed=FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"));
#if WITH_EDITOR
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV13ManualReview")))
    {
        FString User;const auto* Battle=GetWorld()?Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode()):nullptr;
        Allowed=WarfareFRRUCampaignV82::TryPersistenceReviewUserV82(User)&&Battle&&Battle->HasActorBegunPlay()
            &&WarfareFRRUCampaignV82::Active(Battle)&&Battle->Phase==EFrontlinePhase::Fighting;
    }
#endif
    if(!Allowed)
    {Check(TEXT("national_audio_requires_isolated_world_review"),false);return;}
    LoadBank();
    int32 NationalClips=0;bool Saved=true,Semantics=true;
    for(const auto& Clip:VoiceBank)if(Clip.Language>=2)
    {
        ++NationalClips;Saved&=Sounds.Contains(FName(Clip.Name));
        Semantics&=Clip.Kind==VoiceSpeech&&Clip.Events!=VEvent(EFrontlineVoiceEvent::Wounded)
            &&!FString(Clip.Name).Contains(TEXT("voice_ready"))&&!FString(Clip.Name).Contains(TEXT("voice_gun"))
            &&!FString(Clip.Name).Contains(TEXT("voice_signals"));
    }
    Check(TEXT("national_audio_curated_96_actual_saved_speech_clips"),NationalClips==96&&Saved);
    Check(TEXT("national_audio_no_recruit_or_surviving_wound_misbindings"),Semantics);
    for(int32 Language=2;Language<=4;++Language)
    {
        bool Isolated=true;
        for(auto Event:{EFrontlineVoiceEvent::Advance,EFrontlineVoiceEvent::Contact,EFrontlineVoiceEvent::Cover,
            EFrontlineVoiceEvent::GunTakeover,EFrontlineVoiceEvent::IncomingArtillery,EFrontlineVoiceEvent::GasAlarm,
            EFrontlineVoiceEvent::GasChoking,EFrontlineVoiceEvent::BurningPain,EFrontlineVoiceEvent::WireCut})
        {
            const auto Pool=VoiceCandidates(Event,Language,VoiceSpeech);Isolated&=!Pool.IsEmpty();
            for(int32 Index:Pool)Isolated&=VoiceBank[Index].Language==Language||VoiceBank[Index].Language==-1;
        }
        Check(FString::Printf(TEXT("nation_%d_actual_context_pool_without_foreign_speech"),Language),Isolated);
        Check(FString::Printf(TEXT("nation_%d_shared_nonverbal_death_and_blast_pool"),Language),
            VoiceCandidates(EFrontlineVoiceEvent::Wounded,Language,VoiceFatal).Num()==10
            &&VoiceCandidates(EFrontlineVoiceEvent::Wounded,Language,VoiceBlastScream).Num()==6);
    }
    Check(TEXT("national_audio_unknown_actor_cannot_fall_back_to_english"),
        NationalLanguageV82(Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode()),false,MAX_int32)==INDEX_NONE);
    FFrontlineVoiceSchedule Local;
    CommitSpeech(Local,EFrontlineVoiceEvent::Advance,2,15,100.);
    Check(TEXT("national_audio_distinct_nation_keys_and_shared_global_floor"),
        Local.NextSquad.Contains(SquadKey(2,15))&&!Local.NextSquad.Contains(SquadKey(3,15))
        &&!CanSpeak(Local,EFrontlineVoiceEvent::Contact,3,21,101.)
        &&CanSpeak(Local,EFrontlineVoiceEvent::Contact,3,21,104.));
    bool Endings=true;
    for(const TCHAR* Nation:{TEXT("FR"),TEXT("RU"),TEXT("AH")})for(int32 Variant=1;Variant<=2;++Variant)
    {
        const auto* Found=Sounds.Find(FName(*FString::Printf(TEXT("S_OutcomeEnsemble_%s_%02d_V82"),Nation,Variant)));
        Endings&=Found&&*Found&&(*Found)->NumChannels==2;
    }
    Check(TEXT("national_audio_six_actual_stereo_relief_ensembles"),Endings);
    Check(TEXT("national_audio_original_voice_channel_budget_preserved"),VoiceSlots==2&&MusicPlayersV31==2);
}

bool UFrontlineAudioSubsystem::PlayCasualty(bool bGerman, const FVector& Position, int32 SoldierId,
    bool bFatal, bool bExplosion, float Damage, int32 SquadId)
{
    // V26 has only misses/suppression or fatalities, never a wounded survivor.
    if (!bFatal) return false;
    if (!bBattleActive || !GetWorld() || GetWorld()->IsPaused() || bMuted || Volumes[0] <= 0.f || Volumes[2] <= 0.f) return false;
    if (!HasVoiceSlot(GetActiveVoices(), 1)) return false;
    if (SquadId == INDEX_NONE) SquadId = SoldierId / 16; // Existing callers used this stable ID encoding.
    const auto* NationalBattle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    const int32 Language=NationalLanguageV82(NationalBattle,bGerman,SquadId,SoldierId);
    if(Language==INDEX_NONE)return false;
    const double Now = GetWorld()->GetRealTimeSeconds();
    if(!FMath::IsFinite(Damage))return false;
    const ECauseV43 Cause=bExplosion?ECauseV43::Blast:ECauseV43::Bullet;
    int32 Kind = DecideCauseV43(VoiceSchedule,Cause,Language,SoldierId,SquadId,
        Now, VoiceSchedule.Random.FRand(), VoiceSchedule.Random.FRand());
    if (Kind == INDEX_NONE) return false;
    auto Candidates = VoiceCandidates(EFrontlineVoiceEvent::Wounded, Language, uint8(Kind));
    if (Kind == VoiceBlastScream && Candidates.IsEmpty())
    {
        Kind = bFatal ? VoiceFatal : VoicePain;
        Candidates = VoiceCandidates(EFrontlineVoiceEvent::Wounded, Language, uint8(Kind));
    }
    const FString Key = FString::Printf(TEXT("pain:%d:%d"), Language, Kind);
    const int32 Index = PickVoice(VoiceSchedule, Key, Candidates, Now);
    if (Index == INDEX_NONE) return false;
    const FVoiceClip& Clip = VoiceBank[Index];
    const float Gain = Kind == VoiceBlastScream ? .44f : bFatal ? .46f : .36f;
    if (!StartSound(FName(Clip.Name), EFrontlineAudioBus::Voices, Position, Gain, 1.f, true, 1)) return false;
    CommitVoice(VoiceSchedule, Key, Index, Now);
    CommitCauseV43(VoiceSchedule,Cause,Language,SquadId,Kind==VoiceBlastScream,Now);
    ++CasualtyEvents;
    if (bFatal) ++FatalEvents;
    if (Kind == VoiceBlastScream) ++BlastScreamEvents;
    UE_LOG(LogTemp, Display, TEXT("V20_CASUALTY_AUDIO fatal=%d blast=%d scream=%d german=%d person=%d squad=%d sample=%s source=%s active=%d"),
        bFatal, bExplosion, Kind == VoiceBlastScream, Language, SoldierId, SquadId, Clip.Name, Clip.Source, GetActiveVoices());
    return true;
}

bool UFrontlineAudioSubsystem::PlayGasDistressV8215(bool bGerman,const FVector& Position,int32 SoldierId,int32 SquadId)
{
    if(!bBattleActive||!GetWorld()||GetWorld()->IsPaused()||bMuted||Volumes[0]<=0||Volumes[2]<=0||Position.ContainsNaN()||!HasVoiceSlot(GetActiveVoices(),1))return false;
    const auto* Battle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    const int32 Language=NationalLanguageV82(Battle,bGerman,SquadId,SoldierId);if(Language==INDEX_NONE)return false;
    const double Now=GetWorld()->GetRealTimeSeconds();
    const FString Group=FString::Printf(TEXT("gas-distress:%d:%d"),Language,SquadId);
    if(Now<VoiceSchedule.NextEvent.FindRef(TEXT("gas-distress-global"))||Now<VoiceSchedule.NextEvent.FindRef(Group)
        ||Now<VoiceSchedule.NextSevereCasualtyV43||Now<VoiceSchedule.NextUrgent)return false;
    const auto Candidates=VoiceCandidates(EFrontlineVoiceEvent::GasChoking,Language,VoiceSpeech);
    const FString Key=FString::Printf(TEXT("gas-fatal:%d"),Language);const int32 Index=PickVoice(VoiceSchedule,Key,Candidates,Now);
    if(Index==INDEX_NONE)return false;
    if(!StartSound(FName(VoiceBank[Index].Name),EFrontlineAudioBus::Voices,Position,.42f,.98f,true,1))return false;
    CommitVoice(VoiceSchedule,Key,Index,Now);
    VoiceSchedule.NextEvent.Add(TEXT("gas-distress-global"),Now+4.8);VoiceSchedule.NextEvent.Add(Group,Now+12.);
    // Early coughing must not reserve the person's fatal voice for 18 seconds.
    // The finite global gap controls simultaneous victims without a chorus.
    VoiceSchedule.NextGlobal=FMath::Max(VoiceSchedule.NextGlobal,Now+1.7);
    ++VoiceEventCounts[int32(EFrontlineVoiceEvent::GasChoking)];return true;
}

bool UFrontlineAudioSubsystem::PlayGasCasualty(bool bGerman,const FVector& Position,int32 SoldierId,int32 SquadId)
{
    if(!bBattleActive||!GetWorld()||GetWorld()->IsPaused()||bMuted||Volumes[0]<=0||Volumes[2]<=0||!HasVoiceSlot(GetActiveVoices(),1))return false;
    const auto* NationalBattle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    const int32 Language=NationalLanguageV82(NationalBattle,bGerman,SquadId,SoldierId);
    if(Language==INDEX_NONE)return false;
    const double Now=GetWorld()->GetRealTimeSeconds();
    if(DecideCauseV43(VoiceSchedule,ECauseV43::Gas,Language,SoldierId,SquadId,Now,VoiceSchedule.Random.FRand(),0)==INDEX_NONE)return false;
    // A fatal inhalation can break into a cry before the last choking breath.
    // Reuse the admitted male bank, spatial bus and shared scream floor.
    const bool Cry=Now>=VoiceSchedule.NextScream&&VoiceSchedule.Random.FRand()<.42f;
    auto Candidates=VoiceCandidates(Cry?EFrontlineVoiceEvent::Wounded:EFrontlineVoiceEvent::GasChoking,Language,Cry?VoiceBlastScream:VoiceSpeech);
    bool ActualCry=Cry;
    if(Candidates.IsEmpty()){Candidates=VoiceCandidates(EFrontlineVoiceEvent::GasChoking,Language,VoiceSpeech);ActualCry=false;}
    const FString Key=FString::Printf(TEXT("gas-fatal:%d:%d"),Language,int32(ActualCry));const int32 Index=PickVoice(VoiceSchedule,Key,Candidates,Now);
    if(Index==INDEX_NONE)return false;const auto& Clip=VoiceBank[Index];
    if(!StartSound(FName(Clip.Name),EFrontlineAudioBus::Voices,Position,ActualCry?.56f:.54f,ActualCry?.95f:.98f,true,1))return false;
    CommitVoice(VoiceSchedule,Key,Index,Now);CommitCauseV43(VoiceSchedule,ECauseV43::Gas,Language,SquadId,ActualCry,Now);
    BlastScreamEvents+=ActualCry?1:0;
    ++CasualtyEvents;++FatalEvents;++VoiceEventCounts[int32(EFrontlineVoiceEvent::GasChoking)];return true;
}

bool UFrontlineAudioSubsystem::PlayBurnCasualty(bool bGerman,const FVector& Position,int32 SoldierId,int32 SquadId)
{
    if(!bBattleActive||!GetWorld()||GetWorld()->IsPaused()||bMuted||Volumes[0]<=0||Volumes[2]<=0||Position.ContainsNaN())return false;
    const auto* NationalBattle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    const int32 Language=NationalLanguageV82(NationalBattle,bGerman,SquadId,SoldierId);
    if(Language==INDEX_NONE)return false;
    const double Now=GetWorld()->GetRealTimeSeconds();
    if(PendingBurnV44.IsPending()||Now<VoiceSchedule.NextPain.FindRef(SoldierId))return false;
    const double ReadyAt=BurnReadyAtV44(VoiceSchedule,Language,SquadId);
    if(ReadyAt>Now+BurnWaitV44)return false;
    // One draw at the original fatal event; repeated busy-channel checks never
    // turn a failed 96% admission into a guaranteed scream.
    VoiceSchedule.NextPain.Add(SoldierId,Now+18.);
    if(VoiceSchedule.Random.FRand()>=CauseAdmissionV43(ECauseV43::Burn))return false;
    const auto Candidates=VoiceCandidates(EFrontlineVoiceEvent::BurningPain,Language,VoiceSpeech);
    const FString Key=FString::Printf(TEXT("burn-fatal-v43:%d"),Language);const int32 Index=PickVoice(VoiceSchedule,Key,Candidates,Now);
    if(Index==INDEX_NONE)return false;const auto& Clip=VoiceBank[Index];
    if(ReadyAt>Now||!HasVoiceSlot(GetActiveVoices(),1))
    {
        auto* B=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
        if(!B||B->Phase!=EFrontlinePhase::Fighting)return false;
        // Death.cpp calls this after the lethal hit, immediately before it
        // installs DeathState. Observe that completed state on the next tick.
        for(AWarfareFrontlineSoldier* U:B->Units)
            if(IsValid(U)&&!U->IsAlive()&&U->SquadId==SquadId&&U->SquadId*16+U->MemberIndex==SoldierId
                &&NationalLanguageV82(B,U->GetFaction()==EFrontlineFaction::German,U->SquadId,SoldierId)==Language)
            {
                PendingBurnV44.Unit=U;PendingBurnV44.Encounter=B->GetEncounterId();PendingBurnV44.Expires=Now+BurnWaitV44;
                PendingBurnV44.Clip=Index;PendingBurnV44.Person=SoldierId;PendingBurnV44.Squad=SquadId;PendingBurnV44.Language=Language;
                UE_LOG(LogTemp,Display,TEXT("V44_BURN_AUDIO queued person=%d squad=%d sample=%s maximum_wait=1.35"),SoldierId,SquadId,Clip.Name);
                return true;
            }
        return false;
    }
    if(!StartSound(FName(Clip.Name),EFrontlineAudioBus::Voices,Position,.55f,1.f,true,1))return false;
    CommitVoice(VoiceSchedule,Key,Index,Now);CommitCauseV43(VoiceSchedule,ECauseV43::Burn,Language,SquadId,false,Now);
    ++CasualtyEvents;++FatalEvents;++VoiceEventCounts[int32(EFrontlineVoiceEvent::BurningPain)];return true;
}

void UFrontlineAudioSubsystem::TickPendingBurnV44()
{
    if(!PendingBurnV44.IsPending())return;
    const auto Pending=PendingBurnV44;auto* U=Pending.Unit.Get();
    const bool Audible=bBattleActive&&GetWorld()&&!GetWorld()->IsPaused()&&!bMuted&&Volumes[0]>0&&Volumes[2]>0;
    const double Now=GetWorld()?GetWorld()->GetRealTimeSeconds():0.;
    const auto Decision=BurnQueueV44(Now,Pending.Expires,BurnReadyAtV44(VoiceSchedule,Pending.Language,Pending.Squad),
        GetActiveVoices(),Audible&&CurrentBurnV44(U,Pending.Encounter));
    if(Decision==EBurnQueueV44::Wait)return;
    PendingBurnV44=FPendingBurnV44();
    if(Decision==EBurnQueueV44::Cancel)return;
    const auto& Clip=VoiceBank[Pending.Clip];
    if(Now<VoiceSchedule.NextSource.FindRef(FName(Clip.Source)))return;
    if(!StartSound(FName(Clip.Name),EFrontlineAudioBus::Voices,U->GetActorLocation(),.55f,1.f,true,1))return;
    const FString Key=FString::Printf(TEXT("burn-fatal-v43:%d"),Pending.Language);
    CommitVoice(VoiceSchedule,Key,Pending.Clip,Now);CommitCauseV43(VoiceSchedule,ECauseV43::Burn,Pending.Language,Pending.Squad,false,Now);
    ++CasualtyEvents;++FatalEvents;++VoiceEventCounts[int32(EFrontlineVoiceEvent::BurningPain)];
    UE_LOG(LogTemp,Display,TEXT("V44_BURN_AUDIO played person=%d squad=%d sample=%s waited=%.3f"),Pending.Person,Pending.Squad,Clip.Name,BurnWaitV44-(Pending.Expires-Now));
}

void UFrontlineAudioSubsystem::RunV65AudioChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    LoadBank();bool Added=true;int32 AddedCount=0;TSet<FName> Identities;
    for(const auto& Clip:VoiceBank)if(FString(Clip.Name).StartsWith(TEXT("VO_V65_")))
    {
        ++AddedCount;Identities.Add(FName(Clip.Source));const auto* Wave=Sounds.Find(FName(Clip.Name));
        Added&=Wave&&*Wave&&(*Wave)->NumChannels==1&&(*Wave)->Duration>.3f&&(*Wave)->Duration<12.f;
    }
    Check(TEXT("v65_twelve_unique_cleared_male_performances_loaded"),Added&&AddedCount==12&&Identities.Num()==12);
    bool Pools=true;for(bool German:{false,true})
        Pools&=VoiceCandidates(EFrontlineVoiceEvent::BurningPain,German,VoiceSpeech).Num()==8
            &&VoiceCandidates(EFrontlineVoiceEvent::Wounded,German,VoiceFatal).Num()==10
            &&VoiceCandidates(EFrontlineVoiceEvent::Wounded,German,VoiceBlastScream).Num()==6;
    Check(TEXT("v65_bilingual_burn_8_fatal_10_blast_6_distinct_sources"),Pools);
    int32 Counts[4]={};for(int32 Cause=0;Cause<4;++Cause)for(int32 I=0;I<100;++I)
    {FFrontlineVoiceSchedule S;Counts[Cause]+=DecideCauseV43(S,ECauseV43(Cause),false,I,I,100.,(I+.5f)/100.f,.50f)!=INDEX_NONE;}
    Check(TEXT("v65_cause_weighted_48_76_60_96_percent_admission"),Counts[0]==48&&Counts[1]==76&&Counts[2]==60&&Counts[3]==96);
    FFrontlineVoiceSchedule S;CommitCauseV43(S,ECauseV43::Burn,false,1,false,100.);
    Check(TEXT("v65_fire_global_floor_keeps_one_casualty_voice"),DecideCauseV43(S,ECauseV43::Burn,true,2,2,103.59,0.f,0.f)==INDEX_NONE
        &&DecideCauseV43(S,ECauseV43::Burn,true,3,3,103.61,0.f,0.f)==VoiceSpeech&&!HasVoiceSlot(1,1)&&VoiceSlots==2);
}

void UFrontlineAudioSubsystem::RunV44AudioChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const auto* B=GetWorld()?Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode()):nullptr;
    FString UserDir;
    const bool Isolated=B&&B->IsProgressionVolatile()&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty()
        &&(FParse::Param(FCommandLine::Get(),TEXT("WFV44Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV43Review")));
    Check(TEXT("v44_audio_check_uses_isolated_volatile_world"),Isolated);if(!Isolated)return;
    RunV43AudioChecks(Check);
    FFrontlineVoiceSchedule S;S.NextUrgent=100.5;
    const double End=100.+BurnWaitV44,Ready=BurnReadyAtV44(S,false,9);
    Check(TEXT("v44_burn_waits_for_actual_voice_capacity_and_warning_gap"),
        BurnQueueV44(100.,End,Ready,0,true)==EBurnQueueV44::Wait
        &&BurnQueueV44(100.6,End,Ready,1,true)==EBurnQueueV44::Wait
        &&BurnQueueV44(100.6,End,Ready,0,true)==EBurnQueueV44::Play);
    Check(TEXT("v44_burn_drops_stale_or_no_longer_observed_presentations"),
        BurnQueueV44(101.351,End,Ready,0,true)==EBurnQueueV44::Cancel
        &&BurnQueueV44(100.6,End,Ready,0,false)==EBurnQueueV44::Cancel
        &&!CurrentBurnV44(nullptr,FGuid()));
    CommitCauseV43(S,ECauseV43::Bullet,false,1,false,100.);
    Check(TEXT("v44_pending_burn_cannot_evade_global_or_squad_spacing"),
        BurnQueueV44(100.6,End,BurnReadyAtV44(S,false,9),0,true)==EBurnQueueV44::Cancel);
    // Exercise the actual cancellation entry points on a transient, unregistered
    // scheduler: never stop the review scene's real voices or weather loops.
    auto* Scratch=NewObject<UFrontlineAudioSubsystem>(GetWorld());
    Scratch->PendingBurnV44.Clip=0;Scratch->SetBattleActive(false);
    const bool OutcomeCleared=!Scratch->PendingBurnV44.IsPending();
    Scratch->PendingBurnV44.Clip=0;Scratch->ResetEncounterAudio();
    Check(TEXT("v44_reset_and_outcome_clear_pending_burn_even_if_already_inactive"),OutcomeCleared&&!Scratch->PendingBurnV44.IsPending());
}

void UFrontlineAudioSubsystem::RunV41AudioChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    LoadBank();bool Pools=true;TSet<FName> Sources;
    for(bool German:{false,true})for(const auto Event:{EFrontlineVoiceEvent::BurningPain,EFrontlineVoiceEvent::BattleRelief})
    {
        const auto Pool=VoiceCandidates(Event,German,VoiceSpeech);Pools&=Pool.Num()==(Event==EFrontlineVoiceEvent::BurningPain?4:3);
        for(int32 I:Pool){const auto& C=VoiceBank[I];Pools&=Sounds.Contains(FName(C.Name));Sources.Add(FName(C.Source));}
    }
    Check(TEXT("v41_four_burn_and_three_relief_per_faction_loaded"),Pools&&Sources.Num()==14);
    const auto* Music=Sounds.Find(TEXT("S_Music_Victory_V41"));
    Check(TEXT("v41_original_stereo_victory_loop_loaded"),Music&&*Music&&(*Music)->NumChannels==2&&FMath::Abs((*Music)->Duration-64.f)<.01f&&(*Music)->bLooping);
    Check(TEXT("v41_rousing_whistle_pair_loaded"),Sounds.Contains(TEXT("S_ReliefWhistle_01_V41"))&&Sounds.Contains(TEXT("S_ReliefWhistle_02_V41")));
    Check(TEXT("v41_voice_event_mask_capacity"),int32(EFrontlineVoiceEvent::Count)<=32);
    RunFrontlineMusicV31PolicyChecks(Check);
}

void UFrontlineAudioSubsystem::RunV43AudioChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    LoadBank();bool Loaded=true;TSet<FName> Identities;
    for(bool German:{false,true})for(auto Event:{EFrontlineVoiceEvent::BurningPain,EFrontlineVoiceEvent::BattleRelief})
    {
        const auto Pool=VoiceCandidates(Event,German,VoiceSpeech);
        Loaded&=Pool.Num()==(Event==EFrontlineVoiceEvent::BurningPain?4:3);
        for(int32 I:Pool){Loaded&=FString(VoiceBank[I].Name).StartsWith(TEXT("VO_V43_"));Identities.Add(FName(VoiceBank[I].Source));}
    }
    Check(TEXT("v43_matched_bilingual_burn_relief_pool_loaded_without_fake_new_actors"),Loaded&&Identities.Num()==14);
    const auto Blasts=VoiceCandidates(EFrontlineVoiceEvent::Wounded,false,VoiceBlastScream);
    Check(TEXT("v43_four_cleared_blast_cries_loaded"),Blasts.Num()==4);
    const auto* Music=Sounds.Find(TEXT("S_Music_Victory_V43"));
    bool Score=Music&&*Music&&(*Music)->bLooping&&(*Music)->NumChannels==2&&FMath::Abs((*Music)->Duration-64.f)<.01f;
    for(int32 I=1;I<=4;++I)Score&=Sounds.Contains(FName(*FString::Printf(TEXT("S_ReliefWhistle_%02d_V43"),I)));
    Check(TEXT("v43_original_victory_score_and_four_whistle_responses_loaded"),Score);
    int32 Counts[4]={};bool CauseCorrect=true;
    for(int32 Cause=0;Cause<4;++Cause)for(int32 I=0;I<100;++I)
    {
        FFrontlineVoiceSchedule S;
        const int32 K=DecideCauseV43(S,ECauseV43(Cause),false,I,I,100.,(I+.5f)/100.f,.50f);
        Counts[Cause]+=K!=INDEX_NONE;
        if(K!=INDEX_NONE)CauseCorrect&=(Cause==0?K==VoiceFatal:Cause==1?K==VoiceBlastScream:K==VoiceSpeech);
    }
    Check(TEXT("v8215_cause_weights_burn_96_blast_76_gas_84_rifle_48_percent"),Counts[0]==48&&Counts[1]==76&&Counts[2]==84&&Counts[3]==96&&CauseCorrect);
    FFrontlineVoiceSchedule S;CommitCauseV43(S,ECauseV43::Bullet,false,1,false,100.);
    const int32 TooSoon=DecideCauseV43(S,ECauseV43::Burn,true,20,2,104.39,0,0);
    const int32 BurnAfter=DecideCauseV43(S,ECauseV43::Burn,true,21,2,104.41,0,0);
    Check(TEXT("v43_severe_cries_bypass_ordinary_gap_but_keep_4_4_second_global_floor"),TooSoon==INDEX_NONE&&BurnAfter==VoiceSpeech);
    const int32 Retry=DecideCauseV43(S,ECauseV43::Burn,true,21,2,104.5,0,0);
    Check(TEXT("v43_same_person_cannot_repeat_and_two_channel_budget_preserved"),Retry==INDEX_NONE&&VoiceSlots==2&&!HasVoiceSlot(1,1));
    CommitCauseV43(S,ECauseV43::Blast,false,3,true,200.);
    const int32 Cooled=DecideCauseV43(S,ECauseV43::Blast,true,22,4,206.,0,0);
    const int32 Ready=DecideCauseV43(S,ECauseV43::Blast,true,23,4,214.01,0,0);
    Check(TEXT("v43_blast_scream_10_second_cap_keeps_low_fatal_alternative"),Cooled==VoiceFatal&&Ready==VoiceBlastScream);
    FFrontlineVoiceSchedule GasSchedule;CommitCauseV43(GasSchedule,ECauseV43::Bullet,false,1,false,100.);
    const int32 GasBefore=DecideCauseV43(GasSchedule,ECauseV43::Gas,true,501,2,104.39,0,0);
    const int32 GasAfter=DecideCauseV43(GasSchedule,ECauseV43::Gas,true,502,2,104.41,0,0);
    Check(TEXT("v8215_gas_choking_uses_severe_slot_without_removing_global_floor"),GasBefore==INDEX_NONE&&GasAfter==VoiceSpeech);
}

void UFrontlineAudioSubsystem::RunV40AudioChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    bool Pools=true;TSet<FName> Sources;int32 Added=0;
    for(bool German:{false,true})
    {
        for(const auto& Pair:{TPair<EFrontlineVoiceEvent,int32>(EFrontlineVoiceEvent::GasAlarm,4),{EFrontlineVoiceEvent::MaskSealed,2},{EFrontlineVoiceEvent::WireCut,2},{EFrontlineVoiceEvent::GasChoking,3}})
        {
            int32 Count=0;for(int32 I:VoiceCandidates(Pair.Key,German,VoiceSpeech))if(FString(VoiceBank[I].Name).StartsWith(TEXT("VO_V40_"))){++Count;++Added;Sources.Add(FName(VoiceBank[I].Source));}
            Pools&=Count==Pair.Value;
        }
    }
    Check(TEXT("v40_twenty_two_loaded_independent_bilingual_field_performances"),Pools&&Added==22&&Sources.Num()==22);
    Check(TEXT("v40_voice_event_bitmask_has_capacity_and_voice_budget_unchanged"),uint32(EFrontlineVoiceEvent::Count)<=32&&VoiceSlots==2);
}

bool UFrontlineAudioSubsystem::PlayVoice(EFrontlineVoiceEvent Event, bool bGerman, const FVector& Position, int32 SquadId)
{
    if (!bBattleActive || !GetWorld() || GetWorld()->IsPaused() || bMuted || Volumes[0] <= 0.f || Volumes[2] <= 0.f) return false;
    if (Event >= EFrontlineVoiceEvent::Count) return false;
    if (Event == EFrontlineVoiceEvent::Wounded)
        return false;
    const auto* NationalBattle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    const int32 Language=NationalLanguageV82(NationalBattle,bGerman,SquadId,INDEX_NONE);
    if(Language==INDEX_NONE)return false;
    const double Now = GetWorld()->GetRealTimeSeconds();
    const uint8 Priority = IsUrgent(Event) ? 3 : 2;
    if (!CanSpeak(VoiceSchedule, Event, Language, SquadId, Now) || !HasVoiceSlot(GetActiveVoices(), Priority))
    { ++VoiceRejected; return false; }
    const auto Candidates = VoiceCandidates(Event, Language, VoiceSpeech);
    const FString Key = EventKey(Event, Language);
    const int32 Index = PickVoice(VoiceSchedule, Key, Candidates, Now);
    if (Index == INDEX_NONE) { ++VoiceRejected; return false; }
    const FVoiceClip& Clip = VoiceBank[Index];
    if (!StartSound(FName(Clip.Name), EFrontlineAudioBus::Voices, Position, IsUrgent(Event) ? .76f : .68f, 1.f, false, Priority))
    { ++VoiceRejected; return false; }
    CommitVoice(VoiceSchedule, Key, Index, Now);
    CommitSpeech(VoiceSchedule, Event, Language, SquadId, Now);
    ++VoiceEvents; ++VoiceEventCounts[int32(Event)];
    UE_LOG(LogTemp, Display, TEXT("V20_AUDIO_VOICE event=%d german=%d squad=%d sample=%s source=%s active=%d"),
        int32(Event), Language, SquadId, Clip.Name, Clip.Source, GetActiveVoices());
    return true;
}


void UFrontlineAudioSubsystem::RunV26VoiceChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Allowed=FParse::Param(FCommandLine::Get(),TEXT("WFV26Review"));
    Check(TEXT("isolated_v26_voice_review"),Allowed);if(!Allowed)return;
    LoadBank();
    int32 NewCount=0;bool Loaded=true,Legacy=false;
    for(const FVoiceClip& Clip:VoiceBank)
    {
        const FString Name(Clip.Name),Source(Clip.Source);
        if(Name.StartsWith(TEXT("VO_V26_"))){++NewCount;Loaded&=Sounds.Contains(FName(Clip.Name));}
        Legacy|=Source.StartsWith(TEXT("kurt:unitselection")) || Source.StartsWith(TEXT("kurt:aknowledgement"))
            || Source==TEXT("senfft:012") || Source==TEXT("kenney:male:war_target_engaged");
    }
    Check(TEXT("twelve_new_urgent_performances_actually_loaded"),NewCount==12 && Loaded);
    Check(TEXT("calm_legacy_acknowledgements_absent_from_active_bank"),!Legacy);
    for(bool German:{false,true})
    {
        const FString Prefix=German?TEXT("german_"):TEXT("british_");
        for(EFrontlineVoiceEvent Event:{EFrontlineVoiceEvent::ArriveTrench,EFrontlineVoiceEvent::Advance,EFrontlineVoiceEvent::Contact})
        {
            const auto Candidates=VoiceCandidates(Event,German,VoiceSpeech);int32 Added=0;
            for(int32 I:Candidates)if(FString(VoiceBank[I].Name).StartsWith(TEXT("VO_V26_")))++Added;
            const int32 Expected=Event==EFrontlineVoiceEvent::ArriveTrench?3:Event==EFrontlineVoiceEvent::Advance?2:1;
            Check(Prefix+FString::Printf(TEXT("urgent_event_%d_has_new_pool"),int32(Event)),Added==Expected);
        }
        Check(Prefix+TEXT("no_nonfatal_pain_but_fatal_pool_retained"),
            VoiceCandidates(EFrontlineVoiceEvent::Wounded,German,VoicePain).IsEmpty()
            && VoiceCandidates(EFrontlineVoiceEvent::Wounded,German,VoiceFatal).Num()>=2);
    }
    const int32 Before=CasualtyEvents;
    Check(TEXT("wound_events_do_not_play_or_consume_cadence"),
        !PlayCasualty(false,FVector::ZeroVector,1,false,false,20,1)
        && !PlayVoice(EFrontlineVoiceEvent::Wounded,false,FVector::ZeroVector,1) && CasualtyEvents==Before);
}

void UFrontlineAudioSubsystem::RunV20VoiceChecks(TFunctionRef<void(const FString&, bool)> Check)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("WFV20Review"))
        && !FParse::Param(FCommandLine::Get(), TEXT("WFV24OfficerVerify")))
    {
        Check(TEXT("voice_scheduler_review_requires_explicit_isolated_entry"),false);
        UE_LOG(LogTemp, Error, TEXT("V20_VOICE_REVIEW_REJECTED requires isolated WFV20Review or WFV24OfficerVerify entry"));
        return;
    }
    const int32 BeforeEvents = VoiceEvents + CasualtyEvents;
    const double BeforeGlobal = VoiceSchedule.NextGlobal;
    const int32 BeforeSources = VoiceSchedule.NextSource.Num(), BeforeBags = VoiceSchedule.Bags.Num();
    bool MetadataValid = true, AssetsReady = true;
    static_assert(uint32(EFrontlineVoiceEvent::Count)<=32,"Voice event mask capacity");
    const uint32 AllowedEvents = uint32((uint64(1) << uint32(EFrontlineVoiceEvent::Count))-1);
    for (const FVoiceClip& Clip : VoiceBank)
    {
        MetadataValid &= FCString::Strlen(Clip.Source) > 6 && Clip.Kind <= VoiceBlastScream
            && Clip.Language >= -1 && Clip.Language <= 4 && (Clip.Events & ~AllowedEvents) == 0
            && (Clip.Kind != VoiceSpeech || (Clip.Events != 0 && (Clip.Language >= 0 || Clip.Events == VEvent(EFrontlineVoiceEvent::BurningPain))));
        AssetsReady &= Sounds.Contains(FName(Clip.Name));
    }
    Check(TEXT("voice_metadata_has_delivery_language_and_original_recording_id"), MetadataValid);
    Check(TEXT("voice_manifest_assets_are_actually_loaded"), AssetsReady);
    Check(TEXT("voice_bank_has_more_independent_recordings_than_legacy_eleven"), GetUniqueVoiceSourceCount() > 11);
    const auto EnglishAdvance = VoiceCandidates(EFrontlineVoiceEvent::Advance, false, VoiceSpeech);
    const auto GermanAdvance = VoiceCandidates(EFrontlineVoiceEvent::Advance, true, VoiceSpeech);
    Check(TEXT("both_nations_have_multiple_independent_advance_recordings"), EnglishAdvance.Num() >= 2 && GermanAdvance.Num() >= 2);
    Check(TEXT("actual_reload_and_friendly_grenade_events_have_english_recordings"),
        !VoiceCandidates(EFrontlineVoiceEvent::ReloadCover, false, VoiceSpeech).IsEmpty()
        && !VoiceCandidates(EFrontlineVoiceEvent::GrenadeThrowWarning, false, VoiceSpeech).IsEmpty());
    Check(TEXT("no_nonfatal_pain_and_available_fatal_pool"),
        VoiceCandidates(EFrontlineVoiceEvent::Wounded, false, VoicePain).IsEmpty()
        && VoiceCandidates(EFrontlineVoiceEvent::Wounded, false, VoiceFatal).Num() >= 2);

    FFrontlineVoiceSchedule Shuffle;
    Shuffle.Random.Initialize(202020);
    TSet<FName> CycleSources;
    bool CycleUnique = EnglishAdvance.Num() >= 2;
    int32 Last = INDEX_NONE;
    for (int32 I = 0; I < EnglishAdvance.Num(); ++I)
    {
        const int32 Selected = PickVoice(Shuffle, TEXT("advance"), EnglishAdvance, 100. + I*.1);
        if (Selected == INDEX_NONE) { CycleUnique = false; continue; }
        const FName Source(VoiceBank[Selected].Source);
        CycleUnique &= !CycleSources.Contains(Source);
        CycleSources.Add(Source); Last = Selected;
        CommitVoice(Shuffle, TEXT("advance"), Selected, 100. + I*.1);
    }
    Check(TEXT("real_advance_pool_shuffle_completes_once_without_repeating_a_source"), CycleUnique && CycleSources.Num() == EnglishAdvance.Num());
    Check(TEXT("exhausted_pool_stays_silent_while_all_sources_cool_down"), PickVoice(Shuffle, TEXT("advance"), EnglishAdvance, 101.) == INDEX_NONE);
    const int32 NextCycle = PickVoice(Shuffle, TEXT("advance"), EnglishAdvance, 200.);
    Check(TEXT("shuffle_cycle_boundary_avoids_the_previous_recording"), Last != INDEX_NONE && NextCycle != INDEX_NONE
        && FName(VoiceBank[Last].Source) != FName(VoiceBank[NextCycle].Source));

    // Exercise the same source table with two independent event bags. This is
    // deliberately a scheduler fixture, not a claim that these lines were heard.
    int32 SharedA = INDEX_NONE, SharedB = INDEX_NONE;
    for (int32 I = 0; I < UE_ARRAY_COUNT(VoiceBank) && SharedA == INDEX_NONE; ++I)
    {
        if (!Sounds.Contains(FName(VoiceBank[I].Name)) || VoiceBank[I].Kind != VoiceSpeech) continue;
        if (FMath::CountBits(VoiceBank[I].Events) > 1) { SharedA = SharedB = I; break; }
        for (int32 J = I+1; J < UE_ARRAY_COUNT(VoiceBank); ++J)
            if (FName(VoiceBank[I].Source) == FName(VoiceBank[J].Source)
                && VoiceBank[I].Events != VoiceBank[J].Events && Sounds.Contains(FName(VoiceBank[J].Name)))
            { SharedA = I; SharedB = J; break; }
    }
    bool CrossEventCooldown = false;
    if (SharedA != INDEX_NONE)
    {
        FFrontlineVoiceSchedule Shared;
        TArray<int32> One{SharedA}, Other{SharedB};
        const int32 Selected = PickVoice(Shared, TEXT("first-real-event"), One, 100.);
        if (Selected != INDEX_NONE)
        {
            CommitVoice(Shared, TEXT("first-real-event"), Selected, 100.);
            CrossEventCooldown = PickVoice(Shared, TEXT("second-real-event"), Other, 144.9) == INDEX_NONE
                && PickVoice(Shared, TEXT("second-real-event"), Other, 145.) != INDEX_NONE;
        }
    }
    Check(TEXT("same_original_recording_cools_down_across_real_event_pools"), CrossEventCooldown);
    Check(TEXT("shared_two_voice_channels_reserve_room_for_warning_without_cutting_speech"),
        VoiceSlots == 2 && HasVoiceSlot(0, 1) && !HasVoiceSlot(1, 1)
        && !HasVoiceSlot(1, 2) && HasVoiceSlot(1, 3) && !HasVoiceSlot(2, 3));

    FFrontlineVoiceSchedule Cadence;
    CommitSpeech(Cadence, EFrontlineVoiceEvent::Advance, false, 1, 100.);
    Check(TEXT("ordinary_speech_is_cooled_globally_and_across_same_squad_events"),
        !CanSpeak(Cadence, EFrontlineVoiceEvent::Cover, true, 2, 103.1)
        && CanSpeak(Cadence, EFrontlineVoiceEvent::Cover, true, 2, 103.2)
        && !CanSpeak(Cadence, EFrontlineVoiceEvent::Cover, false, 1, 112.8)
        && CanSpeak(Cadence, EFrontlineVoiceEvent::Cover, false, 1, 112.9));
    Check(TEXT("critical_warning_can_follow_an_ordinary_line_but_not_stack_every_frame"),
        !CanSpeak(Cadence, EFrontlineVoiceEvent::IncomingArtillery, true, 2, 101.2)
        && CanSpeak(Cadence, EFrontlineVoiceEvent::IncomingArtillery, true, 2, 101.3));

    FFrontlineVoiceSchedule Pain;
    const int32 LightDamage = DecideCasualty(Pain, false, 16, 1, false, false, 17.f, 100., 0.f, 0.f);
    const int32 SampleMiss = DecideCasualty(Pain, false, 16, 1, false, false, 25.f, 100., .19f, 0.f);
    const int32 RepeatedHit = DecideCasualty(Pain, false, 16, 1, false, false, 25.f, 100.1, 0.f, 0.f);
    const int32 AcceptedPain = DecideCasualty(Pain, false, 32, 2, false, false, 25.f, 100., .17f, 0.f);
    Check(TEXT("light_hits_and_failed_pain_sampling_do_not_retry_into_a_scream"),
        LightDamage == INDEX_NONE && SampleMiss == INDEX_NONE && RepeatedHit == INDEX_NONE && AcceptedPain == VoicePain);
    CommitCasualty(Pain, false, 2, false, false, 100.);
    Check(TEXT("successful_pain_respects_scaled_casualty_budget"),
        DecideCasualty(Pain, true, 48, 3, false, false, 25.f, 111.42, 0.f, 0.f) == INDEX_NONE
        && DecideCasualty(Pain, true, 48, 3, false, false, 25.f, 111.43, 0.f, 0.f) == VoicePain);
    FFrontlineVoiceSchedule Fatal;
    const bool FatalSampled = DecideCasualty(Fatal, false, 16, 1, true, false, 100.f, 100., .39f, 0.f) == INDEX_NONE
        && DecideCasualty(Fatal, false, 32, 2, true, false, 100.f, 100., .37f, 0.f) == VoiceFatal;
    CommitCasualty(Fatal, false, 2, true, false, 100.);
    Check(TEXT("fatal_voice_is_sampled_and_respects_scaled_casualty_budget"), FatalSampled
        && DecideCasualty(Fatal, true, 48, 3, true, false, 100.f, 108.57, 0.f, 0.f) == INDEX_NONE
        && DecideCasualty(Fatal, true, 48, 3, true, false, 100.f, 108.58, 0.f, 0.f) == VoiceFatal);
    FFrontlineVoiceSchedule Scream;
    const bool BlastOnly = DecideCasualty(Scream, false, 16, 1, true, false, 100.f, 100., 0.f, 0.f) == VoiceFatal
        && DecideCasualty(Scream, false, 32, 2, true, true, 100.f, 100., 0.f, .51f) == VoiceFatal
        && DecideCasualty(Scream, false, 48, 3, true, true, 100.f, 100., 0.f, .49f) == VoiceBlastScream;
    CommitCasualty(Scream, false, 3, true, true, 100.);
    const bool ScreamCooldown = DecideCasualty(Scream, true, 64, 4, true, true, 100.f, 128.57, 0.f, 0.f) == VoiceFatal
        && DecideCasualty(Scream, true, 80, 5, true, true, 100.f, 128.58, 0.f, 0.f) == VoiceBlastScream;
    Check(TEXT("blast_delivery_requires_blast_probability_and_scaled_refractory_gap"), BlastOnly && ScreamCooldown);
    Check(TEXT("isolated_scheduler_review_does_not_play_audio_or_mutate_live_cadence"),
        VoiceEvents + CasualtyEvents == BeforeEvents && VoiceSchedule.NextGlobal == BeforeGlobal
        && VoiceSchedule.NextSource.Num() == BeforeSources && VoiceSchedule.Bags.Num() == BeforeBags);
    UE_LOG(LogTemp, Display, TEXT("V20_VOICE_REVIEW mode=isolated-scheduler simulatedClock=1 audibleReview=0 clips=%d uniqueSources=%d"),
        GetLoadedVoiceClipCount(), GetUniqueVoiceSourceCount());
}

void UFrontlineAudioSubsystem::ClearOutcomeV45()
{
    OutcomeAtV32=-1.;OutcomeStageV32=0;OutcomeStartedV45=0.;OutcomeEncounterV45.Invalidate();
    OutcomePositionV32=FVector::ZeroVector;OutcomePlayedV45=OutcomeMissingV45=0;
    for(int32 I=EffectSlots;I<FMath::Min(AmbienceSlot,Components.Num());++I)
        if(Emitters.IsValidIndex(I)&&Emitters[I].bOutcomeV45)
        {if(Components[I])Components[I]->Stop();Emitters[I].bOutcomeV45=Emitters[I].bOutcomeSpeechV45=false;}
}

void UFrontlineAudioSubsystem::TickOutcomeV45()
{
    if(!OutcomeEncounterV45.IsValid()||!GetWorld())return;
    const auto* B=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    const bool Current=B&&B->GetEncounterId()==OutcomeEncounterV45&&B->BattleOutroWinner>=0
        &&(B->Phase==EFrontlinePhase::Victory||B->Phase==EFrontlinePhase::Defeat);
    // Results may remain visible after the seven-second field outro. Preserve
    // already begun tails there; changing encounter/menu cancels them at once.
    if(!Current){ClearOutcomeV45();return;}
    if(bMuted||Volumes[0]<=0.f||Volumes[2]<=0.f){OutcomeAtV32=-1.;return;}
    if(GetWorld()->IsPaused()||OutcomeAtV32<0.)return;
    if(OutcomeStageV32>0){OutcomeAtV32=-1.;return;}
    const double Now=GetWorld()->GetRealTimeSeconds();
    int32 Spoken=0;
    for(int32 I=EffectSlots;I<FMath::Min(AmbienceSlot,Components.Num());++I)
        if(Components[I]&&Components[I]->IsPlaying()&&Emitters[I].Bus==EFrontlineAudioBus::Voices)++Spoken;
    const auto Decision=OutcomeDecisionV46(Now,OutcomeStartedV45,OutcomeAtV32,Current,GetActiveVoices(),Spoken);
    if(Decision==EOutcomeDecisionV46::Cancel){OutcomeAtV32=-1.;return;}
    if(Decision==EOutcomeDecisionV46::Wait)return;
    const TCHAR* Nation=OutcomeLanguageV82==2?TEXT("FR"):OutcomeLanguageV82==3?TEXT("RU"):OutcomeLanguageV82==4?TEXT("AH"):OutcomeLanguageV82==1?TEXT("DE"):TEXT("UK");
    const FName Clip(*FString::Printf(TEXT("S_OutcomeEnsemble_%s_%02d_V%d"),Nation,OutcomeVariantV51+1,OutcomeLanguageV82>=2?82:51));
    if(!Sounds.Contains(Clip))
    {++OutcomeMissingV45;OutcomeAtV32=-1.;UE_LOG(LogTemp,Error,TEXT("V51_OUTCOME_AUDIO missing=%s"),*Clip.ToString());return;}
    if(!StartSound(Clip,EFrontlineAudioBus::Voices,OutcomePositionV32,OutcomeGermanV32?.88f:.93f,1.f,false,3,true,true))return;
    ++OutcomePlayedV45;OutcomeStageV32=1;OutcomeAtV32=-1.;
    UE_LOG(LogTemp,Display,TEXT("V51_OUTCOME_AUDIO germanWinner=%d ensemble=%s age=%.3f voices=%d"),
        OutcomeGermanV32,*Clip.ToString(),Now-OutcomeStartedV45,GetActiveVoices());

}

TSharedRef<FJsonObject> UFrontlineAudioSubsystem::DescribeOutcomeV45() const
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("pending"),OutcomeAtV32>=0.);
    J->SetStringField(TEXT("arrangement"),TEXT("one concentrated recorded male response with original jubilant calls and short human whistles; two alternating takes per army; ten-second instrumental entrance"));
    J->SetNumberField(TEXT("variant"),OutcomeVariantV51+1);
    J->SetNumberField(TEXT("instrumental_handoff"),OutcomeEncounterV45.IsValid()&&GetWorld()?OutcomeMusicHandoffV51(GetWorld()->GetRealTimeSeconds()-OutcomeStartedV45):1.f);
    J->SetNumberField(TEXT("expected_ensemble_starts"),1);
    J->SetNumberField(TEXT("cue_index"),OutcomeStageV32);J->SetNumberField(TEXT("played"),OutcomePlayedV45);
    J->SetNumberField(TEXT("missing"),OutcomeMissingV45);J->SetNumberField(TEXT("voice_capacity"),VoiceSlots);
    J->SetStringField(TEXT("encounter"),OutcomeEncounterV45.ToString());
    J->SetBoolField(TEXT("german_winner"),OutcomeGermanV32);
    J->SetNumberField(TEXT("winner_nation"),OutcomeLanguageV82);
    J->SetNumberField(TEXT("age_seconds"),OutcomeEncounterV45.IsValid()&&GetWorld()?GetWorld()->GetRealTimeSeconds()-OutcomeStartedV45:0.);
    J->SetStringField(TEXT("music"),FFrontlineMusicDirectorV31::AssetName(MusicDirectorV31.Current));
    int32 Playing=0,Speech=0;
    for(int32 I=EffectSlots;I<FMath::Min(AmbienceSlot,Components.Num());++I)
        if(Components[I]&&Components[I]->IsPlaying()&&Emitters[I].bOutcomeV45)
        {++Playing;if(Emitters[I].bOutcomeSpeechV45)++Speech;}
    J->SetNumberField(TEXT("active_outcome_voices"),Playing);J->SetNumberField(TEXT("active_outcome_sentences"),Speech);
    return J;
}

void UFrontlineAudioSubsystem::PlayOutcomeV32(bool bGermanWinner,const FVector& Position)
{
    if(!GetWorld() || Position.ContainsNaN() || bMuted || Volumes[0]<=0.f || Volumes[2]<=0.f)return;
    const auto* B=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());if(!B)return;
    if(OutcomeEncounterV45==B->GetEncounterId())return; // Repeated result calls never restart a response.
    ClearOutcomeV45();LoadBank();OutcomeGermanV32=bGermanWinner;OutcomePositionV32=Position;
    OutcomeLanguageV82=int32(bGermanWinner);
    if(WarfareFRRUCampaignV82::Active(B))
    {
        if(B->BattleOutroWinner<0||B->BattleOutroWinner>1)return;
        OutcomeLanguageV82=int32(WarfareFRRUCampaignV82::Nation(B,B->BattleOutroWinner));
        if(OutcomeLanguageV82<0||OutcomeLanguageV82>4)return;
    }
    OutcomeVariantV51=int32(OutcomeSequenceV51[bGermanWinner?1:0]++%2);
    OutcomeEncounterV45=B->GetEncounterId();OutcomeStartedV45=GetWorld()->GetRealTimeSeconds();
    // FinishBattle can stop the fighting bus later in the same frame. Defer the
    // first relief cue until the field has actually fallen quiet.
    OutcomeAtV32=OutcomeStartedV45+OutcomeStartV46;
}

void UFrontlineAudioSubsystem::RunV45OutcomeChecks(TFunctionRef<void(const FString&,bool)> Check)
{ RunV46OutcomeChecks(Check); }

void UFrontlineAudioSubsystem::RunV46OutcomeChecks(TFunctionRef<void(const FString&,bool)> Check)
{ RunV51OutcomeChecks(Check); }

void UFrontlineAudioSubsystem::RunV51OutcomeChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const auto* B=GetWorld()?Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode()):nullptr;
    FString UserDir;
    const bool Isolated=B&&B->IsProgressionVolatile()
        &&(FParse::Param(FCommandLine::Get(),TEXT("WFV51Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV46Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV45Review")))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
    Check(TEXT("v51_outcome_checks_use_isolated_volatile_world"),Isolated);if(!Isolated)return;
    LoadBank();bool Assets=true;TSet<const USoundWave*> Different;
    for(const TCHAR* Name:{TEXT("S_OutcomeEnsemble_UK_01_V51"),TEXT("S_OutcomeEnsemble_UK_02_V51"),
        TEXT("S_OutcomeEnsemble_DE_01_V51"),TEXT("S_OutcomeEnsemble_DE_02_V51")})
    {
        const auto* Found=Sounds.Find(FName(Name));const USoundWave* W=Found?Found->Get():nullptr;
        Assets&=W&&!Different.Contains(W)&&W->NumChannels==2&&!W->bLooping&&W->Duration>6.8f&&W->Duration<7.1f;Different.Add(W);
    }
    const FName Victory(FFrontlineMusicDirectorV31::AssetName(EFrontlineMusicCueV31::Victory));
    const auto* Music=Sounds.Find(Victory);
    Check(TEXT("v51_four_distinct_faction_takes_loaded_stereo_nonlooping_under_outro_length"),Assets);
    Check(TEXT("v51_victory_maps_to_separate_original_instrumental_score"),Victory==TEXT("S_Music_Victory_V46")
        &&Music&&*Music&&(*Music)->NumChannels==2&&(*Music)->bLooping&&FMath::IsNearlyEqual((*Music)->Duration,64.f,.01f));
    Check(TEXT("v51_ensemble_waits_for_sentence_without_stealing_voice"),
        OutcomeDecisionV46(.12,0,.12,true,1,1)==EOutcomeDecisionV46::Wait
        &&OutcomeDecisionV46(.11,0,.12,true,0,0)==EOutcomeDecisionV46::Wait
        &&OutcomeDecisionV46(.12,0,.12,true,0,0)==EOutcomeDecisionV46::Play);
    Check(TEXT("v51_blocked_or_stale_ensemble_never_replays_late"),
        OutcomeDecisionV46(3.5,0,.16,true,0,0)==EOutcomeDecisionV46::Cancel
        &&OutcomeDecisionV46(.3,0,.16,false,0,0)==EOutcomeDecisionV46::Cancel);
    Check(TEXT("v51_music_leaves_collective_cheer_in_front_then_recovers"),
        OutcomeMusicHandoffV51(.5)<.15f&&OutcomeMusicHandoffV51(3.)<.35f
        &&OutcomeMusicHandoffV51(7.5)>.8f&&OutcomeMusicHandoffV51(10.)==1.f);
    bool Continuous=true;float Previous=OutcomeMusicHandoffV51(0.);
    for(int32 I=1;I<=200;++I){const float V=OutcomeMusicHandoffV51(I*.05);Continuous&=V>=Previous&&V-Previous<.012f;Previous=V;}
    Check(TEXT("v51_instrumental_handoff_is_bounded_continuous_monotonic"),Continuous);
    auto* Scratch=NewObject<UFrontlineAudioSubsystem>(GetWorld());
    Scratch->OutcomeEncounterV45=FGuid::NewGuid();Scratch->OutcomeAtV32=100.;Scratch->OutcomeStageV32=1;
    Scratch->ResetEncounterAudio();
    Check(TEXT("v51_reset_cancels_pending_and_active_outcome_owner"),Scratch->OutcomeAtV32<0.
        &&!Scratch->OutcomeEncounterV45.IsValid()&&Scratch->OutcomeStageV32==0);
    Check(TEXT("v51_ensemble_retains_existing_two_voice_two_music_decoder_budget"),VoiceSlots==2&&MusicPlayersV31==2);
}

void UFrontlineAudioSubsystem::RunV32AudioChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Allowed=FParse::Param(FCommandLine::Get(),TEXT("WFV32Review"));
    Check(TEXT("v32_audio_isolated_entry"),Allowed);if(!Allowed)return;
    LoadBank();bool Loaded=true;TSet<FName> OriginalSources;
    for(const FVoiceClip& Clip:VoiceBank)
    {
        Loaded&=Sounds.Contains(FName(Clip.Name));
        if(Clip.Kind==VoiceBlastScream)OriginalSources.Add(FName(Clip.Source));
    }
    Check(TEXT("v32_actual_blast_pool_four_independent_sources_loaded"),Loaded&&OriginalSources.Num()==4);
    FFrontlineVoiceSchedule Cadence;
    CommitSpeech(Cadence,EFrontlineVoiceEvent::Cover,false,1,100.);
    Check(TEXT("v32_global_and_event_voice_budget_reduced_thirty_percent"),
        FMath::IsNearlyEqual((2.2/(Cadence.NextGlobal-100.)),.70,1.e-6)
        && FMath::IsNearlyEqual(10./(Cadence.NextEvent.FindRef(EventKey(EFrontlineVoiceEvent::Cover,false))-100.),.70,1.e-6));
    Check(TEXT("v32_urgent_warning_reserved_channel_retained"),
        !CanSpeak(Cadence,EFrontlineVoiceEvent::GasAlarm,true,2,101.)
        && CanSpeak(Cadence,EFrontlineVoiceEvent::GasAlarm,true,2,101.3)
        && HasVoiceSlot(1,3)&&!HasVoiceSlot(1,2)&&!HasVoiceSlot(2,3));
    FFrontlineVoiceSchedule Casualty;
    const bool Rifle=DecideCasualty(Casualty,false,16,1,true,false,100,100.,0,.0)==VoiceFatal;
    const bool Blast=DecideCasualty(Casualty,false,32,2,true,true,100,100.,0,.2)==VoiceBlastScream;
    CommitCasualty(Casualty,false,2,true,true,100.);
    const bool NoChorus=DecideCasualty(Casualty,true,48,3,true,true,100,101.,0,0)==INDEX_NONE;
    const bool ScreamGap=DecideCasualty(Casualty,true,64,4,true,true,100,128.,0,0)==VoiceFatal;
    Check(TEXT("v32_blast_delivery_not_bullet_or_gas_and_salvos_do_not_stack"),Rifle&&Blast&&NoChorus&&ScreamGap);
    bool OutcomeLoaded=true;
    for(const TCHAR* Name:{TEXT("VO_V32_UK_Relief_01"),TEXT("VO_V32_UK_Relief_02"),TEXT("VO_V32_DE_Relief_01"),
        TEXT("VO_V32_DE_Relief_02"),TEXT("S_ReliefWhistle_01_V32"),TEXT("S_ReliefWhistle_02_V32")})OutcomeLoaded&=Sounds.Contains(FName(Name));
    Check(TEXT("v32_bilingual_relief_and_whistle_actual_soundwaves_loaded"),OutcomeLoaded);
    Check(TEXT("v32_no_extra_vocal_emitters"),VoiceSlots==2);
}

void UFrontlineAudioSubsystem::RunV35AudioChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Allowed=FParse::Param(FCommandLine::Get(),TEXT("WFV35Review"));
    Check(TEXT("v35_audio_isolated_entry"),Allowed);if(!Allowed)return;
    FFrontlineVoiceSchedule Schedule;
    const bool MoreFatal=DecideCasualty(Schedule,false,351,351,true,false,100,100,.37f,0)==VoiceFatal;
    const bool LessThanCertain=DecideCasualty(Schedule,false,352,352,true,false,100,100,.39f,0)==INDEX_NONE;
    const bool Blast=DecideCasualty(Schedule,true,353,353,true,true,100,100,0,.49f)==VoiceBlastScream;
    CommitCasualty(Schedule,true,353,true,true,100);
    const bool NoChorus=DecideCasualty(Schedule,false,354,354,true,true,100,101,0,0)==INDEX_NONE;
    const bool Spaced=DecideCasualty(Schedule,false,355,355,true,true,100,128.57,0,0)==VoiceFatal
        &&DecideCasualty(Schedule,false,356,356,true,true,100,128.58,0,0)==VoiceBlastScream;
    Check(TEXT("v35_casualty_probability_slightly_increased_blasts_remain_spaced"),MoreFatal&&LessThanCertain&&Blast&&NoChorus&&Spaced);
}

#include "WarfareFrontlineMusicV31Runtime.inl"
