#include "WarfareFrontlineReviewV48.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGameplayV48.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

namespace
{
bool Allowed48(const AWarfareFrontlineGameMode* B)
{
    FString Dir;
    return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV48Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty();
}
bool ShadersPending48()
{
#if WITH_EDITOR
    return !GUsingNullRHI&&GShaderCompilingManager&&GShaderCompilingManager->IsCompiling();
#else
    return false;
#endif
}
bool Success48(const TSharedRef<FJsonObject>& J)
{bool Good=false;return J->TryGetBoolField(TEXT("success"),Good)&&Good;}
TSharedRef<FJsonObject> Actors48(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Men,Tanks;
    for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))
    {
        auto M=MakeShared<FJsonObject>();M->SetStringField(TEXT("actor"),U->GetPathName());
        M->SetNumberField(TEXT("squad"),U->SquadId);M->SetNumberField(TEXT("member"),U->MemberIndex);M->SetNumberField(TEXT("team"),U->Team);
        M->SetBoolField(TEXT("alive"),U->IsAlive());M->SetBoolField(TEXT("entered"),U->bFieldEntered);M->SetBoolField(TEXT("hidden"),U->IsHidden());
        M->SetBoolField(TEXT("finite_transform"),U->GetActorTransform().IsValid()&&!U->GetActorTransform().ContainsNaN());
        M->SetStringField(TEXT("feet"),U->Feet().ToString());M->SetNumberField(TEXT("speed_cm_s"),U->GetMotionSpeed());M->SetNumberField(TEXT("shots"),U->ShotsDischarged);
        if(auto* G=FrontlineMachinegunsV30::Get(U)){M->SetBoolField(TEXT("supported"),G->IsSupported());M->SetNumberField(TEXT("deployments"),G->Deployments);}
        Men.Add(MakeShared<FJsonValueObject>(M));
    }
    if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* T:Fleet->GetVehicles())if(IsValid(T))
    {
        auto M=MakeShared<FJsonObject>();M->SetStringField(TEXT("actor"),T->GetPathName());M->SetNumberField(TEXT("team"),T->Team);
        M->SetBoolField(TEXT("operational"),T->IsOperational());M->SetBoolField(TEXT("entered"),T->HasEnteredField());
        M->SetBoolField(TEXT("finite_transform"),T->GetActorTransform().IsValid()&&!T->GetActorTransform().ContainsNaN());
        M->SetStringField(TEXT("position"),T->GetActorLocation().ToString());M->SetNumberField(TEXT("hull"),T->GetHullIntegrity());
        M->SetNumberField(TEXT("cannon_shots"),T->CannonShots);M->SetNumberField(TEXT("machinegun_shots"),T->MachinegunShots);Tanks.Add(MakeShared<FJsonValueObject>(M));
    }
    J->SetArrayField(TEXT("soldiers"),Men);J->SetArrayField(TEXT("tanks"),Tanks);J->SetNumberField(TEXT("game_seconds"),B->Elapsed);return J;
}
}

// All direct access is restricted to this review friend and an isolated process.
struct FFrontlineReviewV48
{
    static bool Ready(AWarfareFrontlineGameMode* B)
    {return B&&B->HasActorBegunPlay()&&B->LoadedSoldier&&B->IdleAnimation&&B->AssetFailure.IsEmpty();}
    static bool Field(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Mission)
    {
        if(!Allowed48(B)||!FrontlineCampaignV41::PrepareReviewOperation(B,F,Mission))return false;
        B->BeginBattle();B->SetActorTickEnabled(false);B->SetBattleSpeed(1);UGameplayStatics::SetGamePaused(B,false);
        return B->Phase==EFrontlinePhase::Fighting&&B->AssetFailure.IsEmpty();
    }
    static bool ArmourScene(AWarfareFrontlineGameMode* B,const TSharedRef<FJsonObject>& Detail)
    {
        if(!Field(B,EFrontlineFaction::German,19))return false;
        const int32 Trench=FrontlineBattlefield::CentreTrench;B->Trenches[Trench].Owner=0;
        if(!B->SpawnSquad(0,EFrontlineRole::AntiTankRifle,Trench))return false;
        for(auto& Squad:B->Squads)Squad.bHoldPosition=true;
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Fleet)return false;
        // Synchronize the new encounter through the normal subsystem path
        // before clearing readiness. Otherwise first Recruit resets the queue.
        Fleet->Tick(0.f);
        Fleet->ResetReadinessForDebug(0);Fleet->ResetReadinessForDebug(1);
        const FString GermanReason=Fleet->RecruitBlockReason(EFrontlineVehicle::A7V,0);
        const FString BritishReason=Fleet->RecruitBlockReason(EFrontlineVehicle::MarkIVMale,1);
        Detail->SetStringField(TEXT("german_recruit_block_reason"),GermanReason);Detail->SetStringField(TEXT("british_recruit_block_reason"),BritishReason);
        const bool German=Fleet->Recruit(EFrontlineVehicle::A7V,0),British=Fleet->Recruit(EFrontlineVehicle::MarkIVMale,1);
        Detail->SetBoolField(TEXT("german_recruited"),German);Detail->SetBoolField(TEXT("british_recruited"),British);
        if(!German||!British)return false;
        for(AFrontlineTank* T:Fleet->GetVehicles())if(IsValid(T))
        {
            const FVector P(T->Team?-1450.f:1450.f,T->Team?-300.f:300.f,0);T->SetActorLocation(FVector(P.X,P.Y,AWarfareFrontlineGround::HeightAt(P.X,P.Y)));
            T->Simulate(.001f);
        }
        return Fleet->GetVehicles().Num()==2;
    }
};

void StartFrontlineV48Review(AWarfareFrontlineGameMode* Battle)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV48Review")))return;
    if(!Allowed48(Battle))
    {
        UE_LOG(LogTemp,Error,TEXT("WF_V48_REVIEW_REFUSED: volatile progression, WFRenderNoSave and isolated UserDir are required"));
        FString UserDir;
        if(FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty())
        {
            const FString Dir=FPaths::ConvertRelativePathToFull(UserDir/TEXT("Saved/Gameplay/V48/Review"));IFileManager::Get().MakeDirectory(*Dir,true);
            FFileHelper::SaveStringToFile(TEXT("{\"success\":false,\"error\":\"V48 startup refused: missing volatile progression, WFRenderNoSave or isolated UserDir\"}"),*(Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        }
        FPlatformMisc::RequestExit(false);return;
    }
    struct FRun
    {
        TWeakObjectPtr<AWarfareFrontlineGameMode> B;FString Dir;int32 Stage=0,W=3840,H=2160;
        bool Passed=true,Logic=false,Advance=false;double Started=FPlatformTime::Seconds(),Next=Started+.25;float SceneTime=0;
        TArray<FString> Shots;TArray<TSharedPtr<FJsonValue>> Checks,Views;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->B=Battle;R->Logic=GUsingNullRHI||FParse::Param(FCommandLine::Get(),TEXT("nullrhi"));
    FParse::Value(FCommandLine::Get(),TEXT("ResX="),R->W);FParse::Value(FCommandLine::Get(),TEXT("ResY="),R->H);
    R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V48/Review"));IFileManager::Get().MakeDirectory(*R->Dir,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
    {
        auto* B=R->B.Get();const double Now=FPlatformTime::Seconds();
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto* PC=B&&B->GetWorld()?Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController()):nullptr;
        auto Finish=[&]()
        {
            R->Advance=false;if(B)B->SetActorTickEnabled(false);
            bool Files=R->Shots.Num()==(R->Logic?0:2);for(const auto& S:R->Shots)Files&=IFileManager::Get().FileSize(*S)>1024;
            Check(R->Logic?TEXT("null_rhi_does_not_request_images"):TEXT("two_affected_images_written"),Files);
            R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetBoolField(TEXT("null_rhi_logic_only"),R->Logic);
            R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetArrayField(TEXT("captures"),R->Views);
            R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Started);R->Report->SetStringField(TEXT("rhi"),GDynamicRHI?GDynamicRHI->GetName():TEXT("unavailable"));
            R->Report->SetStringField(TEXT("adapter"),GRHIAdapterName);
            if(!R->Logic&&PC){int32 W=0,H=0;PC->GetViewportSize(W,H);R->Report->SetNumberField(TEXT("actual_width"),W);R->Report->SetNumberField(TEXT("actual_height"),H);}
            if(!R->Logic&&B)if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))R->Report->SetObjectField(TEXT("render_settings"),Render->GetDiagnostics());
            R->Report->SetStringField(TEXT("scope"),R->Logic?TEXT("V48 isolated NullRHI: three focused production logic/actor check groups for cooldown/firearm protection, AT stance and armour combat. No graphics, visual-quality or performance acceptance; no player-window interaction."):TEXT("V48 isolated cooldown/firearm protection, AT stance and armour combat checks; two actual views of a placed German AT squad and opposing tanks, using normal live soldier/vehicle ticks. No first-recruit investigation, old terrain/FX/performance suites or wall-time benchmark. Image review remains separate from assertions."));
            FString JSON;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&JSON));
            const bool Saved=FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V48_REVIEW_DONE success=%d checks=%d shots=%d saved=%d"),R->Passed,R->Checks.Num(),R->Shots.Num(),Saved);
            FPlatformMisc::RequestExit(false);
        };
        if(!B||Now-R->Started>180.){Check(TEXT("world_alive_and_review_within_180_seconds"),false);Finish();return false;}
        if(!FFrontlineReviewV48::Ready(B)||(!R->Logic&&(!PC||!PC->GetHUD())))
        {if(Now-R->Started>60.){Check(TEXT("map_and_required_assets_ready_within_60_seconds"),false);Finish();return false;}return true;}
        if(R->Advance){const float Step=FMath::Clamp(DT,0.f,.05f);B->Elapsed+=Step;R->SceneTime+=Step;}
        if(Now<R->Next)return true;
        auto Capture=[&](const TCHAR* Name)
        {
            const FString File=R->Dir/(FString(Name)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(File,true,false);
            R->Shots.Add(File);auto J=Actors48(B);J->SetStringField(TEXT("file"),File);R->Views.Add(MakeShared<FJsonValueObject>(J));
        };
        switch(R->Stage)
        {
        case 0:
            if(ShadersPending48()){R->Next=Now+.25;break;}
            Check(TEXT("isolated_volatile_profile"),Allowed48(B));UGameplayStatics::SetGamePaused(B,false);
            {const auto J=FrontlineGameplayV48::RunChecks(B,Check);R->Report->SetObjectField(TEXT("gameplay"),J);Check(TEXT("gameplay_group"),Success48(J));}
            {const auto J=FrontlineInfantryV45::RunAntiTankStanceChecksV48(B,Check);R->Report->SetObjectField(TEXT("anti_tank_stance"),J);Check(TEXT("anti_tank_stance_group"),Success48(J));}
            if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
            {const auto J=V->RunArmourCombatChecksV48();R->Report->SetObjectField(TEXT("armour_combat"),J);Check(TEXT("armour_combat_group"),Success48(J));}
            else Check(TEXT("armour_subsystem_available"),false);
            if(R->Logic){Finish();return false;}
            FrontlineReviewUIV41::Clear(PC);
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            R->Stage=1;R->Next=FPlatformTime::Seconds()+.1;break;
        case 1:
            {
                const auto Detail=MakeShared<FJsonObject>();const bool Prepared=FFrontlineReviewV48::ArmourScene(B,Detail);
                R->Report->SetObjectField(TEXT("visual_scene_preparation"),Detail);Check(TEXT("real_anti_tank_and_armour_scene"),Prepared);
                if(!Prepared){Finish();return false;}
            }
            FrontlineReviewUIV41::Clear(PC);FrontlineReviewUIV41::Camera(PC,0,-620,2300,110);
            R->SceneTime=0;R->Advance=true;R->Stage=2;R->Next=Now+.25;break;
        case 2:
            {
                bool Supported=false;
                for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->Role==EFrontlineRole::AntiTankRifle&&U->MemberIndex==0)
                    if(auto* G=FrontlineMachinegunsV30::Get(U))Supported|=U->IsActiveCombatant()&&!U->IsHidden()&&U->Soldier->IsVisible()&&G->IsSupported()&&U->GetMotionSpeed()<8.f;
                if(R->SceneTime<2.5f||(!Supported&&R->SceneTime<8.f)){R->Next=Now+.1;break;}
                Check(TEXT("captured_AT_is_alive_visible_supported_and_stationary"),Supported);
                Capture(TEXT("01-anti-tank-supported-contact"));R->Stage=3;R->Next=Now+.75;break;
            }
        case 3:
            FrontlineReviewUIV41::Camera(PC,0,100,4200,180);R->SceneTime=0;R->Stage=4;R->Next=Now+.25;break;
        case 4:
            {
                int32 Count=0;bool Fired=true,Damaged=true,Finite=true;
                if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* T:Fleet->GetVehicles())if(IsValid(T))
                {++Count;Fired&=T->CannonShots>0;Damaged&=T->GetHullIntegrity()<T->GetMaximumHullIntegrity();Finite&=T->GetActorTransform().IsValid()&&!T->GetActorTransform().ContainsNaN();}
                const bool Engaged=Count==2&&Fired&&Damaged&&Finite;
                if(R->SceneTime<4.f||(!Engaged&&R->SceneTime<12.f)){R->Next=Now+.1;break;}
                Check(TEXT("captured_two_tanks_fired_and_received_real_hull_damage"),Engaged);
                Capture(TEXT("02-opposing-armour-combat"));R->Advance=false;R->Stage=5;R->Next=Now+1.;break;
            }
        default:Finish();return false;
        }
        return true;
    }));
}
