#include "WarfareFrontlineFireV33.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Components/InstancedStaticMeshComponent.h"

namespace
{
void StartFrontlineV8214EffectsReview(AWarfareFrontlineController* Controller)
{
    FString User;
    if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8214EffectsReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||FPaths::IsRelative(User))return;
    const FString Allowed=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8214/"));
    if(!FPaths::ConvertRelativePathToFull(User).StartsWith(Allowed,ESearchCase::IgnoreCase))return;
    struct FRun
    {
        int32 Stage=0,Shots=0;bool Passed=true;
        double Start=FPlatformTime::Seconds(),Next=Start+3,Prepared=0,SceneAt=0,LastSample=0;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks,Phases,Samples;
        TWeakObjectPtr<AWarfareFrontlineImpact> Mine;
        TWeakObjectPtr<AWarfareFrontlineSoldier> Burning;
        TWeakObjectPtr<AFrontlineTank> Wreck;
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ProjectSavedDir()/TEXT("Gameplay/V8214/Effects");
    IFileManager::Get().MakeDirectory(*R->Directory,true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float Delta)
    {
        auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;auto* H=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        if(!B||!H)return false;
        if(R->Stage>=10)if(auto* T=R->Wreck.Get())T->Simulate(Delta*B->GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation());
        const double Now=FPlatformTime::Seconds();auto& S=ScreenState(PC);
        auto Check=[&](const FString& N,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),N);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        if(R->Stage>=10&&R->Stage<14&&R->LastSample>0)
        {auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("wall_ms"),(Now-R->LastSample)*1000.);J->SetNumberField(TEXT("game_ms"),FPlatformTime::ToMilliseconds(GGameThreadTime));J->SetNumberField(TEXT("render_ms"),FPlatformTime::ToMilliseconds(GRenderThreadTime));J->SetNumberField(TEXT("rhi_ms"),FPlatformTime::ToMilliseconds(GRHIThreadTime));J->SetNumberField(TEXT("gpu_ms"),FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));R->Samples.Add(MakeShared<FJsonValueObject>(J));}
        R->LastSample=Now;
        if(Now<R->Next)return true;
        auto Shot=[&](const TCHAR* Name)
        {
            ++R->Shots;FScreenshotRequest::RequestScreenshot(R->Directory/(FString(Name)+TEXT(".png")),true,false);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("capture"),Name);J->SetNumberField(TEXT("scene_age_wall_s"),Now-R->SceneAt);
            if(auto* M=R->Mine.Get()){J->SetNumberField(TEXT("mine_visual_age_s"),M->GetVisualAgeSeconds());J->SetNumberField(TEXT("mine_profile"),M->GetBlastProfileIndex());J->SetNumberField(TEXT("mine_fragments"),M->GetBallisticFragmentCount());J->SetNumberField(TEXT("mine_smoke_components_visible"),M->GetVisibleSmokeCount());}
            if(auto* F=AFrontlineFuelFireV33::Get(B->GetWorld())){const auto V=F->VisualStateV8214();J->SetNumberField(TEXT("clothing_particles"),V.Clothing);J->SetNumberField(TEXT("wreck_particles"),V.Wreck);J->SetNumberField(TEXT("embers"),V.Ember);J->SetNumberField(TEXT("soot"),V.Soot);J->SetNumberField(TEXT("attached_clothing"),V.AttachedClothing);J->SetNumberField(TEXT("pool_used"),V.PoolUsed);J->SetNumberField(TEXT("minimum_flipbook_rate"),V.MinFrameRate);J->SetNumberField(TEXT("maximum_flipbook_rate"),V.MaxFrameRate);Check(FString(Name)+TEXT("_bounded_fire_pool"),V.PoolUsed<=V.PoolCapacity);}
            R->Phases.Add(MakeShared<FJsonValueObject>(J));
        };
        double Delay=.12;
        if(Now-R->Start>75){Check(TEXT("bounded_effects_review_timeout"),false);R->Stage=99;}
        switch(R->Stage)
        {
        case 0:
            Check(TEXT("disposable_volatile_profile"),B->IsProgressionVolatile());
            if(auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Settings->SetHDREnabled(false,false);Settings->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            Check(TEXT("disposable_british_campaign_created"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){Run->CompletedMissions=14;Run->WonOperationMaskV52=(1u<<14)-1;Run->Research.TechnologyMask=0xffffffffu;}
            Check(TEXT("british_operation_selected"),B->SelectCampaignMission(7));S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;
            // This fixture tests effects, not the atlas flight or dispatch UI.
            // Enter the production preparation path for the selected operation.
            BeginOperationPreparation(PC,B);
            R->Prepared=Now;break;
        case 1:
            if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&Now-R->Prepared<35){R->Next=Now+.1;return true;}
            Check(TEXT("production_battle_ready"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&B->AssetFailure.IsEmpty());
            B->SetActorTickEnabled(false);B->Elapsed=0;
            FrontlineFieldV32::RunMineChecksV8214(B,Check);
            B->Elapsed=180;
            for(const auto& Unit:B->Units)if(auto* U=Unit.Get()){U->SetActorTickEnabled(false);U->SetActorHiddenInGame(true);}
            for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)if(IsValid(*I))I->Destroy();
            if(auto* F=AFrontlineFuelFireV33::Get(B->GetWorld()))F->Clear();
            FFrontlineReviewV32::Camera(PC,0,350,4800,130);Delay=2.;break;
        case 2:
        {
            const FVector At(960,700,AWarfareFrontlineGround::HeightAt(960,700)+10);
            auto* M=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);R->Mine=M;
            if(M)M->ConfigureMineV8214(600.f);
            Check(TEXT("actual_mine_profile_ready"),M&&M->IsVisualReady()&&M->GetBlastProfileIndex()==5);
            R->SceneAt=Now;Delay=.055;break;
        }
        case 3:Shot(TEXT("01-mine-flash"));Delay=.15;break;
        case 4:Shot(TEXT("02-mine-ejecta"));Delay=.4;break;
        case 5:Shot(TEXT("03-mine-dust-wave"));Delay=.9;break;
        case 6:Shot(TEXT("04-mine-rolling-smoke"));Delay=1.6;break;
        case 7:Shot(TEXT("05-mine-settling"));if(auto* M=R->Mine.Get())M->Destroy();Delay=.2;break;
        case 8:
        {
            const int32 Before=B->Units.Num();FFrontlineReviewV32::Spawn(B,EFrontlineRole::Rifle,0,1);
            for(int32 I=Before;I<B->Units.Num();++I)if(auto* U=B->Units[I].Get()){U->SetActorTickEnabled(false);if(!R->Burning.IsValid()){R->Burning=U;U->SetActorLocation(FVector(-420,380,AWarfareFrontlineGround::HeightAt(-420,380)+88));U->bFieldEntered=true;U->ReturnToIdle();}else U->SetActorHiddenInGame(true);}
            auto* T=B->GetWorld()->SpawnActor<AFrontlineTank>();R->Wreck=T;Check(TEXT("actual_mark_iv_configured"),T&&T->Configure(B,EFrontlineVehicle::MarkIVMale,0,99814));
            if(T){T->SetActorLocation(FVector(500,380,AWarfareFrontlineGround::HeightAt(500,380)+28));T->SetActorTickEnabled(false);}
            FFrontlineReviewV32::Camera(PC,40,380,4400,130);Delay=1.5;break;
        }
        case 9:
            if(auto* U=R->Burning.Get()){U->SetActorTickEnabled(true);U->ReceiveHit(1,.6f,1,FVector(1,0,0),EFrontlineDamageKind::Fire);}
            if(auto* T=R->Wreck.Get())T->ReceiveModuleHit(EFrontlineTankModule::Hull,T->GetHullIntegrity()+1);
            R->SceneAt=Now;Delay=.5;break;
        case 10:
            Shot(TEXT("06-clothing-and-wreck-ignition"));Check(TEXT("production_fire_death"),R->Burning.IsValid()&&!R->Burning->IsAlive());Check(TEXT("production_vehicle_wreck"),R->Wreck.IsValid()&&!R->Wreck->IsOperational());Delay=.9;break;
        case 11:Shot(TEXT("07-clothing-and-wreck-sustained"));Delay=2.;break;
        case 12:Shot(TEXT("08-clothing-and-wreck-fallen"));FFrontlineReviewV32::Camera(PC,40,380,2450,130);Delay=1.;break;
        case 13:Shot(TEXT("09-fire-close-detail"));Delay=.3;break;
        default:
        {
            auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Passed);J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("phases"),R->Phases);J->SetArrayField(TEXT("warm_compound_fire_frame_samples"),R->Samples);J->SetNumberField(TEXT("captures_requested"),R->Shots);
            J->SetStringField(TEXT("scope"),TEXT("Actual UK production mine damage/trigger checks plus 4K SDR phase captures of a cosmetic mine and real Fire casualty/Mark IV Hull wreck. Fixed-budget warm compound-fire frame counters, not foreground presentation FPS, natural campaign or HDR certification."));
            if(auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))J->SetObjectField(TEXT("render"),Settings->GetDiagnostics());
            FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(R->Directory/TEXT("review.json")));
            FPlatformMisc::RequestExit(false);return false;
        }
        }
        // A directly configured review tank is deliberately outside the normal
        // recruitment array. Its production effects still run at world time.
        R->Next=Now+Delay;++R->Stage;return true;
    }),0.f);
}
}
