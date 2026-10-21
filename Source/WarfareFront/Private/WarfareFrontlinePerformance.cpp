#include "WarfareFrontline.h"
#include "Containers/Ticker.h"
#include "CoreGlobals.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void AWarfareFrontlineGameMode::BeginPerformanceReview()
{
    struct FRun
    {
        double Started=FPlatformTime::Seconds(),Last=0,NextRecruit=1,NextAdvance=12;
        bool Requested=false,Begun=false; int32 Role=0,PeakLiving=0,Width=0,Height=0;
        int32 Recruited[AWarfareFrontlineGameMode::RoleCount]={};
        TArray<double> WallMs,GpuMs;
    };
    auto Run=MakeShared<FRun>(); TWeakObjectPtr<AWarfareFrontlineGameMode> Weak(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Run](float)
    {
        auto* B=Weak.Get(); if (!B) return false;
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());
        const double Now=FPlatformTime::Seconds(); double Age=Now-Run->Started;
        if (!PC || (!Run->Requested && Age<2)) return true;
        if (!Run->Requested)
        {
            if (!B->AssetFailure.IsEmpty()) { UE_LOG(LogTemp,Error,TEXT("WF_PROFILE_ASSET_FAILURE %s"),*B->AssetFailure); FPlatformMisc::RequestExit(false); return false; }
            PC->DisableInput(PC); PC->BeginBattle(); PC->BeginBattle(); B->SetBattleSpeed(1);
            Run->Requested=true;
        }
        if (!Run->Begun)
        {
            if (B->Phase!=EFrontlinePhase::Fighting)
            {
                if (Age>22) {UE_LOG(LogTemp,Error,TEXT("WF_PROFILE_PREPARATION_FAILED"));FPlatformMisc::RequestExit(false);return false;}
                return true;
            }
            Run->Begun=true;Run->Started=Now;Run->Last=Now;Age=0;PC->GetViewportSize(Run->Width,Run->Height);
        }
        if (B->Phase==EFrontlinePhase::Fighting)
        {
            const int32 NextRole=Run->Role%AWarfareFrontlineGameMode::RoleCount;
            if (B->Elapsed>=Run->NextRecruit && B->Recruit(EFrontlineRole(NextRole)))
            { ++Run->Recruited[NextRole]; ++Run->Role; Run->NextRecruit=B->Elapsed+1; }
            if (B->Elapsed>=Run->NextAdvance)
            { for (int32 I=0;I<3;++I) B->AdvanceTrench(I); Run->NextAdvance=B->Elapsed+12; }
            if (B->ArtilleryCooldown()<=0)
                for (AWarfareFrontlineSoldier* U:B->Units) if (U && U->Team==1 && U->IsAlive()) { B->RequestArtillery(U->Feet()); break; }
            if (B->Elapsed>=35 && B->SmokeCooldown()<=0)
                B->RequestSmoke(FVector(0,0,0));
            if (B->MortarCooldown()<=0)
                for (AWarfareFrontlineSoldier* U:B->Units) if (U && U->Team==1 && U->IsAlive()) { B->RequestMortar(U->Feet()); break; }
            Run->PeakLiving=FMath::Max(Run->PeakLiving,B->LivingSoldiers(0)+B->LivingSoldiers(1));
            if (Age>=8 && Run->Last>0)
            {
                Run->WallMs.Add((Now-Run->Last)*1000);
                const double Gpu=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
                if (Gpu>0) Run->GpuMs.Add(Gpu);
            }
        }
        Run->Last=Now;
        if (Age<80 && B->Phase==EFrontlinePhase::Fighting) return true;
        const auto Report=MakeShared<FJsonObject>();
        Report->SetStringField(TEXT("scope"),TEXT("V16 actual L_Frontline native camera at 1x speed; five-role rotation and legal advance/artillery/smoke/mortar orders. Up to eighty battle wall seconds or natural match completion; first eight excluded, preparation excluded. No screenshot capture during sample. Wall-frame samples include the configured frame cap; RHI GPU timing is delayed and may not represent identical wall frames."));
        Report->SetStringField(TEXT("adapter"),GRHIAdapterName);
        Report->SetNumberField(TEXT("viewport_width"),Run->Width); Report->SetNumberField(TEXT("viewport_height"),Run->Height);
        Report->SetNumberField(TEXT("peak_living_soldiers"),Run->PeakLiving); Report->SetNumberField(TEXT("shots"),B->ShotsFired);
        Report->SetNumberField(TEXT("game_seconds"),B->Elapsed); Report->SetNumberField(TEXT("wall_seconds"),Age);
        TArray<TSharedPtr<FJsonValue>> Roles;
        for (int32 Count:Run->Recruited) Roles.Add(MakeShared<FJsonValueNumber>(Count));
        Report->SetArrayField(TEXT("player_recruits_by_role"),Roles);
        Report->SetNumberField(TEXT("smoke_commands"),B->SmokeCommands);
        Report->SetNumberField(TEXT("mortar_commands"),B->MortarCommands);
        Report->SetNumberField(TEXT("mortar_rounds_resolved"),B->MortarRoundsResolved);
        auto Stats=[](TArray<double> Values)
        {
            auto S=MakeShared<FJsonObject>(); S->SetNumberField(TEXT("samples"),Values.Num());
            if (Values.Num())
            {
                double Sum=0; for (double X:Values) Sum+=X;
                Values.Sort();
                S->SetNumberField(TEXT("mean_ms"),Sum/Values.Num());
                S->SetNumberField(TEXT("median_ms"),Values[Values.Num()/2]);
                S->SetNumberField(TEXT("p95_ms"),Values[FMath::Min(Values.Num()-1,int32(Values.Num()*.95))]);
                S->SetNumberField(TEXT("p99_ms"),Values[FMath::Min(Values.Num()-1,int32(Values.Num()*.99))]);
                S->SetNumberField(TEXT("max_ms"),Values.Last());
            }
            return S;
        };
        Report->SetObjectField(TEXT("wall_frame_times"),Stats(Run->WallMs)); Report->SetObjectField(TEXT("rhi_gpu_times"),Stats(Run->GpuMs));
        auto Settings=MakeShared<FJsonObject>();
        for (const TCHAR* Name:{TEXT("r.ScreenPercentage"),TEXT("sg.ResolutionQuality"),TEXT("r.SecondaryScreenPercentage.GameViewport"),TEXT("r.RayTracing"),TEXT("r.Lumen.HardwareRayTracing"),TEXT("sg.GlobalIlluminationQuality"),TEXT("sg.ShadowQuality"),TEXT("r.Shadow.Virtual.Enable"),TEXT("r.Nanite"),TEXT("t.MaxFPS")})
            if (const IConsoleVariable* C=IConsoleManager::Get().FindConsoleVariable(Name)) Settings->SetStringField(Name,C->GetString());
        Report->SetObjectField(TEXT("live_settings"),Settings);
        const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V14/Profiles"));
        IFileManager::Get().MakeDirectory(*Dir,true);
        const FString File=Dir/FString::Printf(TEXT("profile-%dx%d-%s.json"),Run->Width,Run->Height,*FGuid::NewGuid().ToString());
        FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json)); FFileHelper::SaveStringToFile(Json,*File);
        UE_LOG(LogTemp,Display,TEXT("WF_PROFILE_COMPLETE %s"),*File); FPlatformMisc::RequestExit(false); return false;
    }));
}
