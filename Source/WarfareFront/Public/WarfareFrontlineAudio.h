#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WarfareFrontlineMusicV31.h"
#include "WarfareFrontlineArmourAudioV839.h"
#include "WarfareFrontlineAudio.generated.h"

class UAudioComponent;
class USoundWave;
class USubmixEffectDynamicsProcessorPreset;
class AWarfareFrontlineController;
class AWarfareFrontlineSoldier;

void StartFrontlineV14AudioVerification(AWarfareFrontlineController* Controller);
void StartFrontlineV18AudioVerification(AWarfareFrontlineController* Controller);

enum class EFrontlineAudioBus : uint8 { Master, Effects, Voices, Ambience, Music };
enum class EFrontlineVoiceEvent : uint8
{
    Advance, Contact, Cover, UnderMachinegun, Wounded,
    ArriveTrench, PassFullTrench, GrenadeWarning, SmokeCover,
    IncomingArtillery, ReloadCover, GrenadeThrowWarning,
    SniperReady, SniperContact, UnderSniper, StormAdvance, StormContact, GunTakeover,
    OfficerCommand, EnemyOfficer, OfficerDown,
    GasAlarm, AircraftInbound, ArmorSighted, FriendlyTankAdvance, TracksDisabled, TankBurning,
    GasChoking, MaskSealed, WireCut, BurningPain, BattleRelief, Count
};

// Scheduling data has no world clock: the production and isolated review paths
// use the same decisions, supplying real time or an explicitly simulated time.
struct FFrontlineVoiceSchedule
{
    struct FBag { TArray<int32> Remaining; FName PreviousSource; };
    TMap<FString, FBag> Bags;
    TMap<FName, double> NextSource;
    TMap<FString, double> NextEvent, NextSquad, NextUrgentSquad;
    TMap<FString, double> NextSevereSquadV43;
    TMap<int32, double> NextPain;
    FRandomStream Random{0x20C0D5};
    double NextGlobal = 0., NextUrgent = 0., NextCasualty = 0., NextScream = 0.;
    double NextSevereCasualtyV43=0.;
};

/** Match events only: there intentionally is no recruit/selection voice event. */
UCLASS()
class WARFAREFRONT_API UFrontlineAudioSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

    void SetVolume(EFrontlineAudioBus Bus, float Volume);
    float GetVolume(EFrontlineAudioBus Bus) const;
    void SetMuted(bool bValue);
    bool IsMuted() const { return bMuted; }
    void SetBattleActive(bool bActive);
    /** Idempotent cancellation for restart/abandon, including delayed relief. */
    void ResetEncounterAudio();
    void SetRainAmount(float Value) { RainAmount=FMath::Clamp(Value,0.f,1.f); }
    bool IsBattleActive() const { return bBattleActive; }
    void PlayWeapon(bool bAutomatic, bool bGerman, const FVector& Position, int32 SquadId);
    void PlayHeavyWeaponV8215(bool bGerman,const FVector& Position,int32 SquadId);
    void PlayImpact(bool bGrenade, const FVector& Position);
    bool PlayArmourContactV837(const FVector& Position,float Strength,bool bExplosive);
    int32 GetArmourContactEventsV837()const{return ArmourContactEventsV837;}
    bool ArmourContactAssetsReadyV837()const;
    bool PlayArmourContactV839(const FVector& Position,float Strength,EFrontlineArmourContactSoundV839 Kind);
    bool ArmourContactAssetsReadyV839()const;
    TSharedRef<class FJsonObject> DescribeArmourContactV839() const;
    void RunArmourAudioChecksV839(TFunctionRef<void(const FString&,bool)> Check);
    bool PlayVoice(EFrontlineVoiceEvent Event, bool bGerman, const FVector& Position, int32 SquadId);
    bool PlayCasualty(bool bGerman,const FVector& Position,int32 SoldierId,bool bFatal,bool bExplosion,float Damage,int32 SquadId=INDEX_NONE);
    bool PlayGasCasualty(bool bGerman,const FVector& Position,int32 SoldierId,int32 SquadId);
    bool PlayGasDistressV8215(bool bGerman,const FVector& Position,int32 SoldierId,int32 SquadId);
    bool PlayBurnCasualty(bool bGerman,const FVector& Position,int32 SoldierId,int32 SquadId);
    void RunV41AudioChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV43AudioChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV65AudioChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunFRRUV82AudioChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV44AudioChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV46OutcomeChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV51OutcomeChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV45OutcomeChecks(TFunctionRef<void(const FString&,bool)> Check);
    TSharedRef<class FJsonObject> DescribeOutcomeV45() const;
    void RunV40AudioChecks(TFunctionRef<void(const FString&,bool)> Check);
    void PlayIncomingShell(const FVector& Position);
    void PlayFootstep(const FVector& Position,int32 SoldierId);
    void UpdateTankAudio(int32 TankId,const FVector& Position,float SpeedFraction,bool bTracksIntact,bool bBurning,bool bActive=true);
    void PlayTankCannon(const FVector& Position);
    void UpdateAircraftAudio(int32 AircraftId,const FVector& Position,float SpeedFraction,bool bActive=true,const FVector& Velocity=FVector::ZeroVector);
    TSharedRef<FJsonObject> DescribeAircraftAudioV8218(int32 AircraftId) const;
    void RunV28AudioChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV30AudioChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV31MusicChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV828AudioChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV834SpatialMixChecks(TFunctionRef<void(const FString&,bool)> Check);
    /** Presentation-only preview. INDEX_NONE returns to the current campaign. */
    void SetMenuMusicNationV828(int32 Nation);
    int32 GetMusicNationV828() const { return MusicNationV828; }
    FName GetMusicAssetV828() const { return FFrontlineMusicDirectorV31::NationalAssetNameV828(MusicDirectorV31.Current,MusicNationV828); }
    void RunV32AudioChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV35AudioChecks(TFunctionRef<void(const FString&,bool)> Check);
    /** One authored stereo relief ensemble, owned by the current outcome. */
    void PlayOutcomeV32(bool bGermanWinner,const FVector& Position);
    EFrontlineMusicCueV31 GetMusicCueV31() const { return MusicDirectorV31.Current; }
    float GetMusicTensionV31() const { return MusicDirectorV31.Tension; }
    int32 GetCasualtyEvents() const { return CasualtyEvents; }
    int32 GetFatalEvents() const { return FatalEvents; }
    void RunV20VoiceChecks(TFunctionRef<void(const FString&, bool)> Check);
    void RunV26VoiceChecks(TFunctionRef<void(const FString&, bool)> Check);
    int32 GetLoadedVoiceClipCount() const;
    int32 GetUniqueVoiceSourceCount() const;
    int32 GetVoiceEventCount(EFrontlineVoiceEvent Event) const
    { return int32(Event) < int32(EFrontlineVoiceEvent::Count) ? VoiceEventCounts[int32(Event)] : 0; }
    int32 GetBlastScreamEvents() const { return BlastScreamEvents; }
    bool HasLimiter() const { return MasterLimiter != nullptr; }

    int32 GetLoadedAssetCount() const { return Sounds.Num(); }
    int32 GetWeaponEvents() const { return WeaponEvents; }
    int32 GetImpactEvents() const { return ImpactEvents; }
    int32 GetVoiceEvents() const { return VoiceEvents; }
    int32 GetVoiceRejected() const { return VoiceRejected; }
    int32 GetEffectsDropped() const { return EffectsDropped; }
    int32 GetActiveEffects() const;
    int32 GetActiveVoices() const;
    int32 GetActiveMusicPlayersV75() const;
    /** Geometric pan diagnostic (-1 left, +1 right), also used by live emitters. */
    float GetPanForPosition(const FVector& Position) const;

private:
    friend struct FFrontlineGameplayV33Access;
    struct FFieldEmitter
    {
        FVector Position = FVector::ZeroVector;
        EFrontlineAudioBus Bus = EFrontlineAudioBus::Effects;
        float Gain = 1.f;
        double Started = 0.;
        bool bSpatial = true;
        uint8 VoicePriority = 0;
        bool bCasualty = false;
        bool bOutcomeV45=false,bOutcomeSpeechV45=false;
        int32 VehicleId = INDEX_NONE;
        double LastVehicleUpdate = 0.;
        FVector AircraftVelocityV8218=FVector::ZeroVector;
    };
    UPROPERTY(Transient) TMap<FName, TObjectPtr<USoundWave>> Sounds;
    UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Components;
    UPROPERTY(Transient) TObjectPtr<USubmixEffectDynamicsProcessorPreset> MasterLimiter;
    TArray<FFieldEmitter> Emitters;
    FFrontlineVoiceSchedule VoiceSchedule;
    struct FPendingBurnV44
    {
        TWeakObjectPtr<AWarfareFrontlineSoldier> Unit;
        FGuid Encounter;
        double Expires=0.;
        int32 Clip=INDEX_NONE,Person=INDEX_NONE,Squad=INDEX_NONE;
        int32 Language=0;
        bool IsPending() const { return Clip!=INDEX_NONE; }
    } PendingBurnV44;
    TMap<FString, int32> LastVariant;
    float Volumes[5] = { .8f, .85f, .85f, .5f, .45f };
    float EffectsDuck = 1.f;
    float MusicGain = 0.f;
    FFrontlineMusicDirectorV31 MusicDirectorV31;
    FFrontlineMusicObservationV31 MusicObservationV31;
    double MusicNextObservationV31=0.,MusicLastObservationV31=0.;
    int32 MusicLastShotsV31=0,MusicLastCasualtiesV31=0,MusicBlastEventsV31=0,MusicLastBlastsV31=0;
    uint32 MusicAppliedGenerationV31=0;
    int32 MusicActivePlayerV31=0;
    double MusicCueOriginV31[2]={0.,0.};
    EFrontlineMusicCueV31 MusicPlayerCueV31[2]={EFrontlineMusicCueV31::Vigil,EFrontlineMusicCueV31::Vigil};
    int32 MenuMusicNationV828=INDEX_NONE,MusicNationV828=INDEX_NONE;
    int32 MusicPlayerNationV828[2]={0,0};
    float RainAmount=0.f;
    double NextFootstepScan = 0.;
    double OutcomeAtV32=-1.;
    FVector OutcomePositionV32=FVector::ZeroVector;
    bool OutcomeGermanV32=false;
    int32 OutcomeLanguageV82=0;
    int32 OutcomeStageV32=0;
    double OutcomeStartedV45=0.;
    FGuid OutcomeEncounterV45;
    int32 OutcomePlayedV45=0,OutcomeMissingV45=0;
    int32 OutcomeVariantV51=0;
    uint32 OutcomeSequenceV51[2]={0,0};
    TMap<int32,double> NextStepTime;
    int32 CasualtyEvents=0, FatalEvents=0;
    bool bMuted = false;
    bool bBattleActive = false;
    bool bLoaded = false;
    int32 VoiceEventCounts[int32(EFrontlineVoiceEvent::Count)] = {};
    int32 BlastScreamEvents = 0;
    int32 WeaponEvents = 0;
    int32 ImpactEvents = 0;
    int32 ArmourContactEventsV837=0;
    int32 ArmourContactRequestsV839[3]={},ArmourContactPlayedV839[3]={};
    FName LastArmourContactSoundV839;
    double LastArmourContactRealTimeV839=-1,LastArmourContactGameTimeV839=-1;
    int32 VoiceEvents = 0;
    int32 VoiceRejected = 0;
    int32 EffectsDropped = 0;

    void LoadBank();
    void TickMusicV31(float DeltaSeconds);
    void TickPendingBurnV44();
    void TickOutcomeV45();
    void ClearOutcomeV45();
    bool bSettingsDirtyV834=false;
    double SettingsSaveAtV834=0.;
    void QueueSettingsSaveV834();
    void SaveSettings(bool bForce=false);
    void RefreshMix();
    // A stack snapshot belongs to one synchronous mix pass, never a later frame.
    struct FSpatialMixViewV834
    {
        bool bCaptured=false,bHasController=false,bHasCamera=false;
        int32 Captures=0;
        FVector Listener=FVector::ZeroVector,Forward=FVector::ZeroVector,Right=FVector::ZeroVector;
        FVector CameraLocation=FVector::ZeroVector,CameraRight=FVector::ZeroVector,AircraftFocus=FVector::ZeroVector;
        float Width=6000.f;
        float Pan(const FVector& Position) const
        {return bHasCamera?FMath::Clamp(float(FVector::DotProduct(Position-CameraLocation,CameraRight))/(Width*.5f),-.92f,.92f):0.f;}
        float Horizontal(const FVector& Position) const
        {return bHasCamera?FMath::Abs(float(FVector::DotProduct(Position-CameraLocation,CameraRight))):0.f;}
    };
    void CaptureSpatialMixViewV834(FSpatialMixViewV834& View) const;
    void UpdateEmitter(int32 Index,FSpatialMixViewV834* SharedView=nullptr);
    int32 ChooseVariant(const FString& Key, int32 Count);
    TArray<int32> VoiceCandidates(EFrontlineVoiceEvent Event, int32 bGerman, uint8 Kind) const;
    bool StartSound(FName Name, EFrontlineAudioBus Bus, const FVector& Position, float Gain, float Pitch = 1.f, bool bCasualty = false, uint8 VoicePriority = 0,bool bOutcome=false,bool bOutcomeSpeech=false);
};
