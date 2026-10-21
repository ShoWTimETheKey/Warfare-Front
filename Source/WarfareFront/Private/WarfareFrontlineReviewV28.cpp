#include "WarfareFrontlineReviewV28.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "RenderTimer.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
bool IsolatedV28()
{
    FString UserDir;
    return FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
}
bool CompleteChecks(const TSharedRef<FJsonObject>& Report)
{
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;if(!Report->TryGetArrayField(TEXT("checks"),Rows)||!Rows||Rows->IsEmpty())return false;
    for(const auto& Row:*Rows)if(!Row.IsValid()||!Row->AsObject()->GetBoolField(TEXT("passed")))return false;
    return true;
}
void SaveReview(const TSharedRef<FJsonObject>& Report,const FString& Directory,const TCHAR* Name)
{
    IFileManager::Get().MakeDirectory(*Directory,true);FString Text;
    FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
    FFileHelper::SaveStringToFile(Text,*(Directory/Name),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
double MeanV28(const TArray<double>& Samples)
{double Sum=0;for(double Value:Samples)Sum+=Value;return Samples.IsEmpty()?0:Sum/Samples.Num();}
double PercentileV28(TArray<double> Samples,double P)
{if(Samples.IsEmpty())return 0;Samples.Sort();return Samples[FMath::Clamp(FMath::CeilToInt(Samples.Num()*P)-1,0,Samples.Num()-1)];}
}

TSharedRef<FJsonObject> RunFrontlineV28Checks(AWarfareFrontlineGameMode* B)
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
    auto Check=[&](const FString& Name,bool Pass,const FString& Detail=FString())
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Pass);Row->SetStringField(TEXT("detail"),Detail);
        Rows.Add(MakeShared<FJsonValueObject>(Row));UE_LOG(LogTemp,Display,TEXT("WF_V28_MECHANICS %s %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"),*Detail);
    };
    ON_SCOPE_EXIT
    {
        int32 Passed=0;for(const auto& R:Rows)Passed+=R->AsObject()->GetBoolField(TEXT("passed"))?1:0;
        Report->SetArrayField(TEXT("checks"),Rows);Report->SetNumberField(TEXT("passed"),Passed);Report->SetNumberField(TEXT("total"),Rows.Num());Report->SetBoolField(TEXT("success"),Passed==Rows.Num());
        Report->SetStringField(TEXT("scope"),TEXT("Isolated synchronous production-API fixtures. Explicit campaign victories, sampled soldier ticks and setup positions; not a natural campaign playthrough, save roundtrip or visual review. Child sections list their actual checks."));
    };
    const bool Allowed=B&&B->bProgressionVolatile&&IsolatedV28()&&FParse::Param(FCommandLine::Get(),TEXT("WFV28Review"));
    Check(TEXT("isolated_v28_progression_and_settings_guard"),Allowed);if(!Allowed)return Report;
    const auto SavedRuns=B->CampaignRuns;const auto SavedActive=B->ActiveCampaignId;const auto SavedFaction=B->PlayerFaction;
    const auto Army0=B->ArmyProgress[0],Army1=B->ArmyProgress[1];const auto Receipts=B->RewardedEncounterIds;
    const bool WasTicking=B->IsActorTickEnabled();B->SetActorTickEnabled(false);
    ON_SCOPE_EXIT
    {
        for(TActorIterator<AFrontlineSupportMissionV28> It(B->GetWorld());It;++It)It->Destroy();
        B->CampaignRuns=SavedRuns;B->ActiveCampaignId=SavedActive;B->PlayerFaction=SavedFaction;B->ArmyProgress[0]=Army0;B->ArmyProgress[1]=Army1;B->RewardedEncounterIds=Receipts;
        B->ResetEncounter();B->SetActorTickEnabled(WasTicking);
    };
    B->ResetEncounter();Check(TEXT("real_new_british_campaign_created"),B->StartNewCampaign(EFrontlineFaction::British));
    const EFrontlineSupport Types[]={EFrontlineSupport::Artillery,EFrontlineSupport::Smoke,EFrontlineSupport::Mortar,EFrontlineSupport::Gas,EFrontlineSupport::AntiTank,EFrontlineSupport::AirStrike};
    const int32 Unlock[]={0,1,2,3,3,4};bool Progression=true,EarlyBlocked=true;
    for(int32 Stage=0;Stage<6;++Stage)
    {
        B->BeginBattle();Progression&=B->Phase==EFrontlinePhase::Fighting&&B->GetEncounterCampaignMission()&&B->GetEncounterCampaignMission()->Id==Stage;
        for(int32 I=0;I<6;++I)Progression&=B->IsSupportUnlocked(Types[I])==(Stage>=Unlock[I]);
        if(Stage==0)
        {
            B->Elapsed=200;const float Supplies=B->Supplies[0],Command=B->CommandPoints[0];
            for(int32 I=1;I<6;++I)EarlyBlocked&=!B->RequestSupport(Types[I],FVector::ZeroVector)&&!B->SupportBlockReason(Types[I]).IsEmpty();
            EarlyBlocked&=Supplies==B->Supplies[0]&&Command==B->CommandPoints[0]&&B->ExtendedSupportCommands[0]==0&&B->ExtendedSupportCommands[1]==0&&B->ExtendedSupportCommands[2]==0;
        }
        if(Stage<5)
        {
            B->Elapsed=46;B->ShotsFired=8;B->FinishBattle(0,TEXT("V28 explicit campaign progression fixture"));
            Progression&=B->GetCurrentCampaign()&&B->GetCurrentCampaign()->CompletedMissions==Stage+1;
            B->RestartBattle();Progression&=B->SelectCampaignMission(Stage+1);
        }
    }
    Check(TEXT("all_six_support_unlocks_follow_real_campaign_victories"),Progression);
    Check(TEXT("unassigned_support_orders_are_rejected_without_cost_or_mission"),EarlyBlocked);
    bool Spawned=B->SpawnSquad(0,EFrontlineRole::Lewis,B->GetHomeTrenchIndex())&&B->SpawnSquad(1,EFrontlineRole::Lewis,B->GetHomeTrenchIndex(1))&&B->SpawnSquad(0,EFrontlineRole::Mills,B->GetHomeTrenchIndex());
    AWarfareFrontlineSoldier* BritishGun=nullptr;AWarfareFrontlineSoldier* GermanGun=nullptr;AWarfareFrontlineSoldier* Walker=nullptr;AWarfareFrontlineSoldier* Bomber=nullptr;
    for(AWarfareFrontlineSoldier* U:B->Units)if(U)
    {
        U->SetActorTickEnabled(false);U->bFieldEntered=false;
        if(U->IsMachineGunner())(U->Team?GermanGun:BritishGun)=U;
        if(U->Team==0&&U->Role==EFrontlineRole::Lewis&&U->MemberIndex==1)Walker=U;
        if(U->Team==0&&U->Role==EFrontlineRole::Mills&&U->MemberIndex==0)Bomber=U;
    }
    Spawned&=BritishGun&&GermanGun&&Walker&&Bomber&&Walker->GetRunMotion()!=nullptr;
    Check(TEXT("actual_soldier_weapon_and_run_assets_ready"),Spawned,B->AssetFailure);if(!Spawned)return Report;
    // The real Tick determines position and stride; no duplicate speed formula.
    Walker->bFieldEntered=true;Walker->ReturnToIdle();Walker->bWantedMovement=true;Walker->DepartureClock=0;Walker->FirePoseClock=0;Walker->ShotClock=100;
    const FVector Destination=B->SoldierDestination(Walker);const FVector Start=Destination-FVector(600,0,0);
    Walker->SetActorLocation(FVector(Start.X,Start.Y,AWarfareFrontlineGround::HeightAt(Start.X,Start.Y)+88));Walker->MotionSpeed=300;
    TArray<double> Before400,After400;
    for(int32 I=0;I<120;++I)
    {
        Walker->Tick(.02f);const float Remaining=FVector::Dist2D(Walker->Feet(),Destination);
        if(Remaining>400&&Remaining<=440)Before400.Add(Walker->GetMotionSpeed());
        if(Remaining<=400&&Remaining>=360)After400.Add(Walker->GetMotionSpeed());
        if(Remaining<350)break;
    }
    const double BeforeSpeed=MeanV28(Before400),AfterSpeed=MeanV28(After400);
    Check(TEXT("actual_translation_crosses_400cm_without_speed_step"),Before400.Num()>=2&&After400.Num()>=2&&BeforeSpeed>100&&AfterSpeed/BeforeSpeed>.90,
        FString::Printf(TEXT("before %.3f cm/s (%d samples), after %.3f (%d)"),BeforeSpeed,Before400.Num(),AfterSpeed,After400.Num()));
    TArray<double> Speeds;for(float Pressure:{.39f,.40f,.41f,.59f,.60f,.61f})
    {
        const FVector P=Destination-FVector(1100,0,0);Walker->SetActorLocation(FVector(P.X,P.Y,AWarfareFrontlineGround::HeightAt(P.X,P.Y)+88));
        Walker->bWantedMovement=true;Walker->DepartureClock=0;Walker->FirePoseClock=0;Walker->Suppression=Pressure;Walker->MotionSpeed=1000;Walker->Tick(.5f);Speeds.Add(Walker->GetMotionSpeed());
    }
    bool SameSpeed=Speeds[0]>0;for(int32 I=1;I<Speeds.Num();++I)SameSpeed&=FMath::IsNearlyEqual(Speeds[I],Speeds[0],.1);
    Check(TEXT("actual_translation_retains_same_speed_across_pressure_thresholds"),SameSpeed,FString::Printf(TEXT("p .39/.40/.41 = %.2f/%.2f/%.2f; p .59/.60/.61 = %.2f/%.2f/%.2f cm/s"),Speeds[0],Speeds[1],Speeds[2],Speeds[3],Speeds[4],Speeds[5]));
    Walker->bFieldEntered=false;BritishGun->bFieldEntered=GermanGun->bFieldEntered=true;
    for(int32 Team=0;Team<2;++Team)
    {
        auto* Shooter=Team?GermanGun:BritishGun;auto* Target=Team?BritishGun:GermanGun;
        const float Base=Team?5800.f:5450.f,Effective=Base*B->EncounterVisibilityScale();
        Shooter->SetActorLocation(FVector(0,0,1888));Target->SetActorLocation(FVector(B->GetTeamDirection(Team)*(Effective-1),0,1888));
        const bool Inside=B->HasSight(Shooter,Target)&&B->AcquireTarget(Shooter)==Target&&B->BulletHitChance(Shooter,Target)>0;
        Target->SetActorLocation(FVector(B->GetTeamDirection(Team)*(Effective+1),0,1888));
        const bool Outside=B->AcquireTarget(Shooter)==nullptr&&B->BulletHitChance(Shooter,Target)==0;
        Check(Team?TEXT("german_mg5800_actual_acquisition_and_fire_probability_boundary"):TEXT("british_mg5450_actual_acquisition_and_fire_probability_boundary"),
            FrontlineWeapons::ProfileFor(Shooter).FireRangeCm==Base&&Inside&&Outside,FString::Printf(TEXT("base %.0f cm, mission effective %.2f, tested +/-1 cm"),Base,Effective));
    }
    BritishGun->bFieldEntered=false;Bomber->bFieldEntered=true;Bomber->ReturnToIdle();Bomber->SetActorLocation(FVector(0,0,1888));
    GermanGun->SetActorLocation(FVector(1321,0,1888));const int32 Throws=Bomber->GrenadeThrowsStarted;Bomber->StartGrenadeThrow(GermanGun);const bool FarRejected=Bomber->GrenadeThrowsStarted==Throws;
    GermanGun->SetActorLocation(FVector(1319,0,1888));Bomber->StartGrenadeThrow(GermanGun);
    Check(TEXT("actual_grenade_preparation_obeys_1320cm_boundary"),FarRejected&&Bomber->GrenadeThrowsStarted==Throws+1&&Bomber->WeaponPhase==EFrontlineWeaponPhase::GrenadeThrow);
    for(AWarfareFrontlineSoldier* U:B->Units)if(U){U->ReturnToIdle();U->bFieldEntered=true;}
    B->Elapsed=220;
    for(int32 I=0;I<3;++I)
    {
        const auto Kind=Types[I+3];const float Supplies=B->Supplies[0],Command=B->CommandPoints[0];const int32 Before=B->ExtendedSupportCommands[I];
        const bool Accepted=B->RequestSupport(Kind,FVector(-1700+I*1700,0,0));const bool Rejected=!B->RequestSupport(Kind,FVector(500,0,0));
        Check(FString::Printf(TEXT("actual_%s_order_cooldown_and_no_wallet_charge"),I==0?TEXT("gas"):I==1?TEXT("antitank"):TEXT("airstrike")),
            Accepted&&Rejected&&B->ExtendedSupportCommands[I]==Before+1&&B->SupportCooldown(Kind)>0&&B->Supplies[0]==Supplies&&B->CommandPoints[0]==Command);
    }
    // Focused subsystem suites keep their individual assertions in child sections.
    auto Support=RunFrontlineV28SupportChecks(B);Report->SetObjectField(TEXT("support"),Support);Check(TEXT("actual_support_timing_visibility_and_encounter_checks"),CompleteChecks(Support));
    if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
    {auto Vehicle=Vehicles->RunFocusedChecks();Report->SetObjectField(TEXT("vehicles"),Vehicle);Check(TEXT("actual_four_vehicle_asset_and_module_checks"),CompleteChecks(Vehicle));}
    else Check(TEXT("actual_four_vehicle_asset_and_module_checks"),false,TEXT("Vehicle subsystem missing"));
    TArray<TSharedPtr<FJsonValue>> AudioRows;bool AudioPass=true;
    if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->RunV28AudioChecks([&](const FString& Name,bool Pass)
    {auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),Name);R->SetBoolField(TEXT("passed"),Pass);AudioRows.Add(MakeShared<FJsonValueObject>(R));AudioPass&=Pass;});
    auto Audio=MakeShared<FJsonObject>();Audio->SetArrayField(TEXT("checks"),AudioRows);Report->SetObjectField(TEXT("audio"),Audio);
    Check(TEXT("new_audio_events_and_vehicle_loops_actually_checked"),AudioPass&&AudioRows.Num()>=18);
    return Report;
}

void StartFrontlineV28Review(AWarfareFrontlineController* Controller)
{
    const bool Field=FParse::Param(FCommandLine::Get(),TEXT("WFV28FieldReview"));
    if(!Controller||(!Field&&!FParse::Param(FCommandLine::Get(),TEXT("WFV28Review"))))return;
    if(!IsolatedV28()){UE_LOG(LogTemp,Error,TEXT("WF_V28_REVIEW requires WFRenderNoSave and UserDir"));FPlatformMisc::RequestExit(false);return;}
    Controller->DisableInput(Controller);Controller->SetActorTickEnabled(false);
    struct FRun
    {
        int32 Stage=0,Shot=0,InitialShots=0,InitialLosses=0;double Started=FPlatformTime::Seconds(),Next=Started+2,FieldAt=0,LastSample=0,BurstAt=0;
        bool Ready=true,AirRequested=false;FString Directory,Failure;
        TArray<double> Wall,GPU;TArray<TSharedPtr<FJsonValue>> Captures;
        TArray<TWeakObjectPtr<AWarfareFrontlineImpact>> Effects;
        TArray<TWeakObjectPtr<AFrontlineTank>> Tanks;
        TWeakObjectPtr<AFrontlineSupportMissionV28> Air;
        int32 SampleStartShots=0,SampleEndShots=0,SampleEndAirRounds=0;
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V28")/
        (Field?TEXT("FieldReview"):TEXT("Review"))/FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*R->Directory,true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,R,Field](float)
    {
        auto* PC=Weak.Get();auto* World=PC?PC->GetWorld():nullptr;auto* B=World?Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode()):nullptr;
        if(!PC||!B){FPlatformMisc::RequestExit(false);return false;}
        const double Now=FPlatformTime::Seconds();
        if(Field&&R->FieldAt>0&&Now-R->FieldAt>=4&&Now-R->FieldAt<=12)
        {
            if(R->LastSample>0)
            {
                R->Wall.Add((Now-R->LastSample)*1000.);
                const double GPUms=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());if(GPUms>0)R->GPU.Add(GPUms);
            }
            else R->SampleStartShots=B->ShotsFired;
            R->LastSample=Now;R->SampleEndShots=B->ShotsFired;R->SampleEndAirRounds=R->Air.IsValid()?R->Air->RoundsFired:0;
        }
        if(Now<R->Next)return true;
        auto Finish=[&]()
        {
            auto J=MakeShared<FJsonObject>();int32 Written=0;for(const auto& C:R->Captures)Written+=IFileManager::Get().FileExists(*C->AsObject()->GetStringField(TEXT("file")))?1:0;
            int32 W=0,H=0;PC->GetViewportSize(W,H);const bool Sampled=R->Wall.Num()>=30&&R->GPU.Num()>0;
            J->SetBoolField(TEXT("fixture_ready"),R->Ready);J->SetBoolField(TEXT("success"),R->Ready&&Written==4&&Sampled&&W==3840&&H==2160);
            J->SetStringField(TEXT("failure"),R->Failure);J->SetNumberField(TEXT("width"),W);J->SetNumberField(TEXT("height"),H);J->SetArrayField(TEXT("captures"),R->Captures);
            J->SetNumberField(TEXT("natural_shots"),B->ShotsFired-R->InitialShots);J->SetNumberField(TEXT("natural_casualties"),B->Casualties[0]+B->Casualties[1]-R->InitialLosses);
            J->SetBoolField(TEXT("airstrike_requested_through_production_api"),R->AirRequested);
            auto P=MakeShared<FJsonObject>();P->SetNumberField(TEXT("frames"),R->Wall.Num());P->SetNumberField(TEXT("wall_ms_mean"),MeanV28(R->Wall));
            P->SetNumberField(TEXT("render_loop_fps"),MeanV28(R->Wall)>0?1000./MeanV28(R->Wall):0);P->SetNumberField(TEXT("wall_ms_p99"),PercentileV28(R->Wall,.99));
            P->SetNumberField(TEXT("gpu_ms_mean"),MeanV28(R->GPU));P->SetNumberField(TEXT("gpu_ms_p99"),PercentileV28(R->GPU,.99));
            P->SetNumberField(TEXT("shots_in_sample"),R->SampleEndShots-R->SampleStartShots);P->SetNumberField(TEXT("air_rounds_at_sample_end"),R->SampleEndAirRounds);
            P->SetStringField(TEXT("scope"),TEXT("One fixed-camera 8-second window after 4-second warmup; natural infantry/tanks/weather/audio. No screenshot inside this interval. Aircraft may still be en route; actual round count recorded. Delayed GPU counters, no full campaign or 4K90 guarantee; FPS is the game/render loop, not generated display frames."));
            J->SetObjectField(TEXT("short_performance_sample"),P);
            TArray<TSharedPtr<FJsonValue>> Vehicles;
            for(const auto& T:R->Tanks)if(T.IsValid())
            {
                auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("type"),int32(T->Type));V->SetNumberField(TEXT("team"),T->Team);V->SetBoolField(TEXT("operational"),T->IsOperational());
                V->SetNumberField(TEXT("cannon_shots"),T->CannonShots);V->SetNumberField(TEXT("machinegun_shots"),T->MachinegunShots);V->SetNumberField(TEXT("distance_cm"),T->DistanceTravelled);Vehicles.Add(MakeShared<FJsonValueObject>(V));
            }
            J->SetArrayField(TEXT("field_vehicles"),Vehicles);
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(World))J->SetObjectField(TEXT("renderer"),Render->GetDiagnostics());
            J->SetStringField(TEXT("scope"),TEXT("Actual L_Frontline at normal 6000cm camera. Explicit late-campaign victories and initial squad/tank placement, then natural AI, weapons, life and movement. Three cosmetic explosion profiles and one aircraft-calibre dust profile staged together for age comparison. Component presence is not image-quality approval; PNGs require visual inspection."));
            SaveReview(J,R->Directory,TEXT("field-review.json"));SaveReview(J,FPaths::ProjectSavedDir()/TEXT("Gameplay/V28"),TEXT("field-review.json"));
            UE_LOG(LogTemp,Display,TEXT("WF_V28_FIELD_COMPLETE ready=%d captures=%d frames=%d %s"),R->Ready,Written,R->Wall.Num(),*R->Directory);FPlatformMisc::RequestExit(false);
        };
        if(!B->bProgressionVolatile){R->Ready=false;R->Failure=TEXT("Volatile campaign guard missing");if(Field)Finish();else{auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),false);J->SetStringField(TEXT("failure"),R->Failure);SaveReview(J,R->Directory,TEXT("review.json"));FPlatformMisc::RequestExit(false);}return false;}
        if(!Field)
        {
            auto J=RunFrontlineV28Checks(B);SaveReview(J,R->Directory,TEXT("review.json"));SaveReview(J,FPaths::ProjectSavedDir()/TEXT("Gameplay/V28"),TEXT("review.json"));
            UE_LOG(LogTemp,Display,TEXT("WF_V28_REVIEW_COMPLETE passed=%.0f total=%.0f %s"),J->GetNumberField(TEXT("passed")),J->GetNumberField(TEXT("total")),*R->Directory);FPlatformMisc::RequestExit(false);return false;
        }
        if(Now-R->Started>65||!B->AssetFailure.IsEmpty())
        {R->Ready=false;R->Failure=B->AssetFailure.IsEmpty()?TEXT("Field fixture timeout"):B->AssetFailure;Finish();return false;}
        if(R->Stage==0)
        {
            // Real campaign API advancement is fixture setup, not won gameplay.
            B->SetActorTickEnabled(false);B->ResetEncounter();R->Ready&=B->StartNewCampaign(EFrontlineFaction::British);
            for(int32 I=0;I<5;++I)
            {B->BeginBattle();B->Elapsed=46;B->ShotsFired=8;B->FinishBattle(0,TEXT("V28 field setup progression fixture"));B->RestartBattle();R->Ready&=B->SelectCampaignMission(I+1);}
            B->BeginBattle();R->Ready&=B->Phase==EFrontlinePhase::Fighting&&B->GetEncounterYear()==1918;
            B->Trenches[1].Owner=0;B->Trenches[3].Owner=1;
            for(int32 Team=0;Team<2;++Team)for(auto Role:{EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills})
                R->Ready&=B->SpawnSquad(Team,Role,Team?3:1);
            for(auto& Squad:B->Squads)
            {Squad.Trench=INDEX_NONE;Squad.Destination=2;Squad.LastPassedTrench=Squad.Team?3:1;Squad.bAdvancing=true;Squad.bReinforcement=false;}
            for(AWarfareFrontlineSoldier* U:B->Units)if(U)
            {
                const float X=(U->Team?1.f:-1.f)*(1200+int32(U->Role)*220.f)+U->MemberIndex*58.f,Y=(int32(U->Role)-1)*175.f+(U->MemberIndex%2)*75.f;
                U->bFieldEntered=true;U->ReturnToIdle();U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));U->SetActorRotation(FRotator(0,U->Team?180:0,0));
            }
            B->Elapsed=100;auto* Vehicles=UFrontlineVehicleSubsystem::Get(World);R->Ready&=Vehicles!=nullptr;
            if(Vehicles)
            {
                for(int32 Team=0;Team<2;++Team)for(const auto& E:Vehicles->GetRoster(Team))R->Ready&=Vehicles->Recruit(E.Type,Team);
                for(AFrontlineTank* T:Vehicles->GetVehicles())if(T)
                {
                    const int32 I=int32(T->Type);const float Xs[]={-2400,-1300,1300,2400,-1850,1850};const float Y=(I%2)?455.f:-455.f;
                    T->SetActorLocation(FVector(Xs[I],Y,AWarfareFrontlineGround::HeightAt(Xs[I],Y)+4));R->Tanks.Add(T);
                }
                R->Ready&=R->Tanks.Num()==int32(EFrontlineVehicle::Count);
            }
            R->AirRequested=B->RequestSupport(EFrontlineSupport::AirStrike,FVector::ZeroVector);R->Ready&=R->AirRequested;
            for(TActorIterator<AFrontlineSupportMissionV28> It(World);It;++It)R->Air=*It;
            if(!PC->BattleCamera){R->Ready=false;R->Failure=TEXT("Battle camera unavailable");Finish();return false;}
            PC->BattleCamera->SetActorLocation(FVector(0,14000,4650));PC->BattleCamera->SetActorRotation((FVector(0,0,650)-FVector(0,14000,4650)).Rotation());
            PC->BattleCamera->GetCameraComponent()->OrthoWidth=6000;PC->SetViewTarget(PC->BattleCamera);
            for(TActorIterator<AActor> It(World);It;++It)if(It->ActorHasTag(TEXT("WF_DistantMatte_V12")))
            {It->SetActorLocation(FVector(0,-5200,-1500));It->SetActorScale3D(FVector(125,1,42));break;}
            R->InitialShots=B->ShotsFired;R->InitialLosses=B->Casualties[0]+B->Casualties[1];B->SetActorTickEnabled(true);
            R->FieldAt=FPlatformTime::Seconds();R->Next=R->FieldAt+13;R->Stage=1;return true;
        }
        if(R->Stage==1)
        {
            // Keep the real aircraft transmission/travel delay. No time fast-forward.
            if(R->Air.IsValid()&&R->Air->RoundsFired==0&&Now-R->FieldAt<32){R->Next=Now+.05;return true;}
            R->Ready&=R->Air.IsValid()&&R->Air->RoundsFired>0;
            if(B->Phase!=EFrontlinePhase::Fighting){R->Ready=false;R->Failure=TEXT("Natural encounter ended before effect comparison");}
            const float Xs[]={-1250,0,1250,2200};
            for(int32 I=0;I<4;++I)
            {
                const FVector P(Xs[I],600,AWarfareFrontlineGround::HeightAt(Xs[I],600));auto* Effect=World->SpawnActor<AWarfareFrontlineImpact>(P,FRotator::ZeroRotator);
                if(Effect){if(I<3)Effect->ConfigureBlast(I==0,I==1);else Effect->ConfigureBulletStrike();}
                R->Ready&=Effect&&Effect->IsVisualReady();R->Effects.Add(Effect);
            }
            R->BurstAt=FPlatformTime::Seconds();R->Next=R->BurstAt+.06;R->Stage=2;return true;
        }
        if(R->Stage==2&&R->Shot<4)
        {
            const double ExpectedAges[]={.06,.30,.90,2.20};
            // Screenshot encoding stalls wall time; schedule the next image
            // against the real effect clock, never count that stall as aging.
            const auto* AgeReference=R->Effects.IsValidIndex(2)?R->Effects[2].Get():nullptr;
            if(AgeReference&&AgeReference->GetVisualAgeSeconds()<ExpectedAges[R->Shot]){R->Next=Now+.01;return true;}
            const FString File=R->Directory/FString::Printf(TEXT("%02d-normal-scale-field-effects.png"),R->Shot+1);FScreenshotRequest::RequestScreenshot(File,true,false);
            auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("file"),File);C->SetNumberField(TEXT("requested_wall_age"),Now-R->BurstAt);C->SetNumberField(TEXT("battle_elapsed"),B->Elapsed);
            C->SetNumberField(TEXT("living_infantry"),B->LivingSoldiers(0)+B->LivingSoldiers(1));C->SetNumberField(TEXT("air_rounds"),R->Air.IsValid()?R->Air->RoundsFired:0);
            TArray<TSharedPtr<FJsonValue>> FX;
            for(int32 I=0;I<R->Effects.Num();++I)
            {
                auto Item=MakeShared<FJsonObject>();auto* Effect=R->Effects[I].Get();Item->SetStringField(TEXT("fixture_profile"),I==0?TEXT("grenade"):I==1?TEXT("mortar"):I==2?TEXT("shell"):TEXT("aircraft_calibre_dust"));Item->SetBoolField(TEXT("alive"),Effect!=nullptr);
                if(Effect){Item->SetNumberField(TEXT("actual_profile_index"),Effect->GetBlastProfileIndex());Item->SetNumberField(TEXT("actual_effect_age"),Effect->GetVisualAgeSeconds());Item->SetNumberField(TEXT("visible_smoke_components"),Effect->GetVisibleSmokeCount());Item->SetBoolField(TEXT("flash_component_visible"),Effect->IsFlashVisible());}
                FX.Add(MakeShared<FJsonValueObject>(Item));
            }
            C->SetArrayField(TEXT("effects"),FX);R->Captures.Add(MakeShared<FJsonValueObject>(C));++R->Shot;
            R->Next=R->Shot<4?Now+.01:Now+.8;
            if(R->Shot==4)R->Stage=3;return true;
        }
        Finish();return false;
    }));
}
