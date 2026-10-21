#include "WarfareFrontlineReviewV51.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineCampaignV51.h"
#include "WarfareFrontlineGameplayV51.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineWaterV51.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PipelineStateCache.h"
#include "RHI.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#endif

namespace
{
bool Allowed51(const AWarfareFrontlineGameMode* B)
{
    FString Dir;return B&&B->GetWorld()&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV51Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty()&&!FPaths::IsRelative(Dir);
}
struct FRenderReady51
{
    int32 Assets=0,Shaders=0,PSOs=0;
    bool Ready()const{return Assets==0&&Shaders==0&&PSOs==0;}
};
FRenderReady51 Compilation51()
{
    FRenderReady51 S;
#if WITH_EDITOR
    S.Assets=FAssetCompilingManager::Get().GetNumRemainingAssets();
    S.Shaders=GShaderCompilingManager&&GShaderCompilingManager->IsCompiling()?1:0;
#endif
    if(!GUsingNullRHI)S.PSOs=PipelineStateCache::GetNumActivePipelinePrecompileTasks();
    return S;
}
const TCHAR* ShotName51(int32 I)
{
    static const TCHAR* Names[]={TEXT("01-v51-flanders-mud.png"),TEXT("02-v51-chalk-shellfield.png"),
        TEXT("03-v51-village-rubble.png"),TEXT("04-v51-winter-patrol.png"),TEXT("05-v51-new-attachments-hud.png"),
        TEXT("06-v51-british-epilogue.png"),TEXT("07-v51-german-epilogue-zh.png"),TEXT("08-v51-training-and-respirators-zh.png")};
    return I>=0&&I<8?Names[I]:TEXT("invalid-scene.png");
}
}

struct FFrontlineReviewV51
{
    static bool Ready(AWarfareFrontlineGameMode* B)
    {return B&&B->HasActorBegunPlay()&&B->LoadedSoldier&&B->IdleAnimation&&B->AssetFailure.IsEmpty();}
    static bool Field(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Mission)
    {
        if(!Allowed51(B)||!FrontlineCampaignV41::PrepareReviewOperation(B,F,Mission))return false;
        B->BeginBattle();B->SetActorTickEnabled(false);UGameplayStatics::SetGamePaused(B,false);
        return B->Phase==EFrontlinePhase::Fighting&&B->AssetFailure.IsEmpty();
    }
    static void Clear(AWarfareFrontlineGameMode* B)
    {
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Empty();B->Squads.Empty();
        if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))V->ResetBattle();
        B->BattleOutroWinner=INDEX_NONE;B->Phase=EFrontlinePhase::Fighting;
    }
    static bool ArtScene(AWarfareFrontlineGameMode* B,int32 Index)
    {
        if(!Allowed51(B)||Index<0||Index>=8)return false;
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());if(!PC)return false;
        FrontlineLocalizationV35::SetLanguage(Index>=6?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
        if(Index>=5&&Index<=6)
        {
            if(!FrontlineCampaignV51::PrepareEndingReview(B,Index==5?EFrontlineFaction::British:EFrontlineFaction::German))return false;
            B->SetActorTickEnabled(false);FrontlineReviewUIV51::Ending(PC);FrontlineReviewUIV41::BeginAudit();return true;
        }
        if(Index==7)
        {
            if(!FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,0))return false;
            auto& Research=const_cast<FFrontlineArmyProgress&>(B->GetActiveResearch());Research.TechnologyMask=(1u<<3)|(1u<<10);Research.ResearchPoints=12;
            B->SetActorTickEnabled(false);FrontlineReviewUIV41::Technology(PC,1,16);FrontlineReviewUIV41::Pointer(FVector2D(486,390));
            FrontlineReviewUIV41::BeginAudit();return true;
        }
        constexpr int32 Missions[]={5,0,1,10,16};
        const auto F=Index==2||Index==3?EFrontlineFaction::German:EFrontlineFaction::British;
        if(!Field(B,F,Missions[Index]))return false;Clear(B);
        const int32 Home=B->GetHomeTrenchIndex(0);
        if(Index==4)
        {
            auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());if(!Run)return false;
            Run->RoleSelectionMask=(1u<<int32(EFrontlineRole::Rifle))|(1u<<int32(EFrontlineRole::Lewis))
                |(1u<<int32(EFrontlineRole::Pioneer))|(1u<<int32(EFrontlineRole::MobileGun))|(1u<<int32(EFrontlineRole::Signals));
        }
        const bool Spawned=B->SpawnSquad(0,EFrontlineRole::Rifle,Home)
            &&B->SpawnSquad(0,Index==4?EFrontlineRole::MobileGun:EFrontlineRole::Lewis,Home)
            &&B->SpawnSquad(0,Index==4?EFrontlineRole::Signals:EFrontlineRole::Mills,Home);
        FrontlineReviewUIV41::Clear(PC);
        FrontlineReviewUIV41::Camera(PC,Index==4?B->Trenches[Home].X:0.f,600,Index==4?5400.f:9000.f,650);
        if(auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
        {Settings->SetHDREnabled(false,false);Settings->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
        FrontlineReviewUIV41::BeginAudit();return Spawned;
    }
    static TSharedRef<FJsonObject> ArtState(AWarfareFrontlineGameMode* B,int32 Index)
    {
        auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("scene_index"),Index);
        if(const auto* M=B->GetSelectedCampaignMission())J->SetNumberField(TEXT("mission"),M->Id);
        J->SetStringField(TEXT("faction"),B->GetPlayerFaction()==EFrontlineFaction::German?TEXT("German"):TEXT("British"));
        J->SetNumberField(TEXT("trench_count"),B->Trenches.Num());J->SetNumberField(TEXT("soldiers"),B->Units.Num());
        int32 Tiles=0;for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
        {
            for(UStaticMeshComponent* C:It->BakedTerrainV43)if(C&&C->GetStaticMesh()&&C->IsVisible())++Tiles;
            TSharedPtr<FJsonObject> Terrain;auto Reader=TJsonReaderFactory<>::Create(It->DescribeTerrainV43());
            if(FJsonSerializer::Deserialize(Reader,Terrain)&&Terrain)J->SetObjectField(TEXT("terrain"),Terrain);
        }
        J->SetNumberField(TEXT("visible_bound_terrain_tiles"),Tiles);return J;
    }
};

void StartFrontlineV51Review(AWarfareFrontlineGameMode* B)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV51Review")))return;
    struct FRun
    {
        TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;
        double Start=FPlatformTime::Seconds(),Next=0,QuietSince=0,SceneStart=0,SampleStart=0,SampleLast=0;
        TArray<double> FrameSamples,GPUSamples;
        int32 Stage=0,ArtIndex=0,ArtEnd=8;bool Good=true;
        FString Dir,Shot;TArray<TSharedPtr<FJsonValue>> Checks,Scenes;
    };
    auto R=MakeShared<FRun>();R->Battle=B;R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V51/Review"));
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV51PreviewPair")))R->ArtEnd=2;
    IFileManager::Get().MakeDirectory(*R->Dir,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
    {
        auto* B=R->Battle.Get();const double Now=FPlatformTime::Seconds();
        const auto Check=[&](const FString& Name,bool Passed)
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);
            R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Passed;
            UE_LOG(LogTemp,Display,TEXT("WF_V51_CHECK %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),*Name);
        };
        const auto Finish=[&]()
        {
            auto J=MakeShared<FJsonObject>();int32 Passed=0;for(const auto& C:R->Checks)Passed+=C->AsObject()->GetBoolField(TEXT("passed"));
            J->SetBoolField(TEXT("success"),R->Good);J->SetNumberField(TEXT("passed"),Passed);J->SetNumberField(TEXT("total"),R->Checks.Num());
            J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("art_scenes"),R->Scenes);
            J->SetBoolField(TEXT("null_rhi_logic_only"),GUsingNullRHI);J->SetBoolField(TEXT("art_only"),FParse::Param(FCommandLine::Get(),TEXT("WFV51Art")));
            J->SetNumberField(TEXT("wall_seconds"),Now-R->Start);
            J->SetStringField(TEXT("scope"),TEXT("Focused V51 production attachment, research, gas, campaign ending, outcome audio and terrain checks. Eight optional 4K art captures wait for actual asset/shader/PSO readiness. Volatile isolated profile; no player-window control or performance claim."));
            FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V51_REVIEW_DONE success=%d passed=%d total=%d"),R->Good,Passed,R->Checks.Num());FPlatformMisc::RequestExit(false);
        };
        if(!B||Now-R->Start>480){Check(TEXT("v51_bounded_world_and_time"),false);Finish();return false;}
        if(!FFrontlineReviewV51::Ready(B))return true;
        if(!Allowed51(B)){Check(TEXT("v51_requires_isolated_volatile_profile"),false);Finish();return false;}
        if(!FParse::Param(FCommandLine::Get(),TEXT("WFV51Art")))
        {
            const bool Field=FFrontlineReviewV51::Field(B,EFrontlineFaction::British,0);Check(TEXT("v51_production_world_ready"),Field);
            if(Field)
            {
                FFrontlineReviewV51::Clear(B);FrontlineGameplayV51::RunChecks(B,Check);
                AWarfareFrontlineGround::RunChecksV51(Check);
                UFrontlineWaterResponseV51::RunChecks(B->GetWorld(),Check);
                if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->RunV51OutcomeChecks(Check);
                else Check(TEXT("v51_audio_subsystem_exists"),false);
                FrontlineCampaignV51::RunChecksV51(B,Check);
            }
            Finish();return false;
        }
        if(GUsingNullRHI){Check(TEXT("v51_art_requires_renderer"),false);Finish();return false;}
        if(R->Stage==0)
        {
            if(R->ArtIndex>=R->ArtEnd){Finish();return false;}
            const bool Loaded=FFrontlineReviewV51::ArtScene(B,R->ArtIndex);
            Check(FString::Printf(TEXT("v51_art_%d_production_scene_ready"),R->ArtIndex),Loaded);if(!Loaded){Finish();return false;}
            R->SceneStart=FPlatformTime::Seconds();R->Next=R->SceneStart+6.;R->QuietSince=0;
            R->SampleStart=0;R->FrameSamples.Reset();R->GPUSamples.Reset();R->Stage=1;return true;
        }
        if(B->Phase==EFrontlinePhase::Fighting)B->Elapsed+=FMath::Clamp(DT,0.f,.05f);
        if(R->Stage==1)
        {
            const auto Pending=Compilation51();
            if(!Pending.Ready()){R->QuietSince=0;R->SampleStart=0;R->FrameSamples.Reset();R->GPUSamples.Reset();return true;}
            if(R->QuietSince==0)R->QuietSince=Now;
            if(Now<R->Next||Now-R->QuietSince<1.)return true;
            // A short fixed-scene render-cost sample accompanies the affected
            // field captures. No image readback occurs inside this interval.
            // The director is held; this is not a live-combat FPS benchmark.
            if(R->ArtIndex<5)
            {
                if(R->SampleStart==0){R->SampleStart=R->SampleLast=Now;return true;}
                R->FrameSamples.Add((Now-R->SampleLast)*1000.);R->SampleLast=Now;
                const double GPU=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());if(GPU>0)R->GPUSamples.Add(GPU);
                if(Now-R->SampleStart<4.)return true;
            }
            auto Scene=FFrontlineReviewV51::ArtState(B,R->ArtIndex);
            if(R->FrameSamples.Num())
            {
                auto Timing=MakeShared<FJsonObject>();double Total=0,GPUTotal=0;
                for(double V:R->FrameSamples)Total+=V;for(double V:R->GPUSamples)GPUTotal+=V;
                R->FrameSamples.Sort();
                Timing->SetNumberField(TEXT("frames"),R->FrameSamples.Num());Timing->SetNumberField(TEXT("seconds"),Total/1000.);
                Timing->SetNumberField(TEXT("mean_render_fps"),1000.*R->FrameSamples.Num()/Total);
                Timing->SetNumberField(TEXT("wall_p95_ms"),R->FrameSamples[FMath::Clamp(FMath::FloorToInt(.95*(R->FrameSamples.Num()-1)),0,R->FrameSamples.Num()-1)]);
                if(R->GPUSamples.Num())Timing->SetNumberField(TEXT("mean_gpu_ms"),GPUTotal/R->GPUSamples.Num());
                Timing->SetStringField(TEXT("scope"),TEXT("Four seconds, fixed camera, held battle director, active rendering/weather/actors; FG off; no readback. Not live-combat or foreground-play performance."));
                Scene->SetObjectField(TEXT("render_cost_sample"),Timing);
            }
            const auto Audit=FrontlineReviewUIV41::EndAudit();Scene->SetObjectField(TEXT("ui_audit"),Audit);
            Check(FString::Printf(TEXT("v51_art_%d_text_and_image_bounds"),R->ArtIndex),Audit->GetBoolField(TEXT("success")));
            if(R->ArtIndex<5)Check(FString::Printf(TEXT("v51_art_%d_uses_baked_visible_terrain"),R->ArtIndex),Scene->GetNumberField(TEXT("visible_bound_terrain_tiles"))>0);
            Scene->SetNumberField(TEXT("scene_ready_seconds"),Now-R->SceneStart);
            Scene->SetNumberField(TEXT("pending_asset_compiles"),Pending.Assets);Scene->SetNumberField(TEXT("pending_shader_compiles"),Pending.Shaders);Scene->SetNumberField(TEXT("pending_pso_compiles"),Pending.PSOs);
            R->Shot=R->Dir/ShotName51(R->ArtIndex);Scene->SetStringField(TEXT("capture"),R->Shot);R->Scenes.Add(MakeShared<FJsonValueObject>(Scene));
            FScreenshotRequest::RequestScreenshot(R->Shot,true,false);R->Stage=2;R->Next=Now+1.;return true;
        }
        if(Now<R->Next)return true;
        Check(FString::Printf(TEXT("v51_art_%d_capture_written"),R->ArtIndex),IFileManager::Get().FileSize(*R->Shot)>1024);
        ++R->ArtIndex;R->Stage=0;return true;
    }));
}
