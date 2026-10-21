#include "WarfareFrontlineFieldReviewV23.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineImpact.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"
#include "UnrealClient.h"

void StartFrontlineV23FieldReview(AWarfareFrontlineController* Controller)
{
    if(!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV23FieldReview")))return;
    // The fixture owns this opt-in process only. Raw edge-hover reads do not
    // respect IgnoreMoveInput, so stop PlayerTick as well as action input before
    // any waiting/setup. Explicit camera placement below preserves the normal view.
    Controller->DisableInput(Controller);
    Controller->SetActorTickEnabled(false);
    struct FReview
    {
        int32 Stage=0,Capture=0;bool Ready=true;
        double Started=FPlatformTime::Seconds(),Next=Started+3,BurstAt=0;
        FString Folder,Failure;
        TArray<TWeakObjectPtr<AWarfareFrontlineImpact>> Bursts;
        TArray<TSharedPtr<FJsonValue>> Captures;
        TSharedPtr<FJsonObject> Ground=MakeShared<FJsonObject>();
    };
    auto Run=MakeShared<FReview>();
    Run->Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V23/FieldReview")/FGuid::NewGuid().ToString());
    IFileManager::Get().MakeDirectory(*Run->Folder,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Run](float)
    {
        auto* PC=Weak.Get();auto* World=PC?PC->GetWorld():nullptr;
        auto* B=World?Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode()):nullptr;
        const double Now=FPlatformTime::Seconds();
        if(!PC || !World || !B){FPlatformMisc::RequestExit(false);return false;}
        if(Now<Run->Next)return true;
        if(!B->AssetFailure.IsEmpty() || Now-Run->Started>65)
        {Run->Ready=false;Run->Failure=B->AssetFailure.IsEmpty()?TEXT("Field fixture timed out"):B->AssetFailure;Run->Stage=99;}
        auto SpawnProfiles=[&]()
        {
            Run->Bursts.Reset();
            const float Xs[]={-4000.f,-2500.f,-1000.f};
            for(int32 I=0;I<3;++I)
            {
                const FVector At(Xs[I],0,AWarfareFrontlineGround::HeightAt(Xs[I],0));
                auto* FX=World->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);
                if(FX)FX->ConfigureBlast(I==0,I==1);
                Run->Ready&=IsValid(FX)&&FX->IsVisualReady();Run->Bursts.Add(FX);
            }
        };
        auto Snap=[&](const FString& Name)
        {
            const FString Path=Run->Folder/(Name+TEXT(".png"));FScreenshotRequest::RequestScreenshot(Path,true,false);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("file"),Path);
            J->SetNumberField(TEXT("wall_after_review_burst"),Now-Run->BurstAt);
            if(PC->BattleCamera)
            {J->SetNumberField(TEXT("camera_x"),PC->BattleCamera->GetActorLocation().X);J->SetNumberField(TEXT("camera_width"),PC->BattleCamera->GetCameraComponent()->OrthoWidth);}
            TArray<TSharedPtr<FJsonValue>> Profiles;
            for(int32 I=0;I<Run->Bursts.Num();++I)
            {
                const auto* FX=Run->Bursts[I].Get();auto P=MakeShared<FJsonObject>();
                P->SetStringField(TEXT("profile"),I==0?TEXT("grenade_left_trench"):(I==1?TEXT("mortar_centre_ground"):TEXT("artillery_right_ground")));
                P->SetBoolField(TEXT("alive"),IsValid(FX));
                if(IsValid(FX))
                {
                    P->SetNumberField(TEXT("actual_visual_age"),FX->GetVisualAgeSeconds());
                    P->SetNumberField(TEXT("visible_smoke_components"),FX->GetVisibleSmokeCount());
                    P->SetBoolField(TEXT("flash_component_visible"),FX->IsFlashVisible());
                    P->SetNumberField(TEXT("profile_index"),FX->GetBlastProfileIndex());
                    P->SetNumberField(TEXT("actor_scale"),FX->GetActorScale3D().GetAbsMax());
                }
                Profiles.Add(MakeShared<FJsonValueObject>(P));
            }
            J->SetArrayField(TEXT("effects"),Profiles);Run->Captures.Add(MakeShared<FJsonValueObject>(J));
        };
        switch(Run->Stage)
        {
        case 0:
            PC->Restart();Run->Ready&=B->SelectPlayerFaction(EFrontlineFaction::British);
            if(!B->Trenches.IsValidIndex(2)){Run->Ready=false;Run->Failure=TEXT("Field fixture requires existing trenches 1 and 2");Run->Stage=99;break;}
            B->Trenches[1].Owner=0;B->Trenches[2].Owner=1;B->BeginBattle();
            B->SetActorTickEnabled(false);PC->SelectTrench(1);
            if(!PC->BattleCamera){Run->Ready=false;Run->Failure=TEXT("Field fixture camera missing");Run->Stage=99;break;}
            {
                const double CentreX=B->Trenches[1].X+1800.;
                const FVector Focus(CentreX,0,650),Location(CentreX,14000,4650);
                PC->BattleCamera->SetActorLocation(Location);
                PC->BattleCamera->SetActorRotation((Focus-Location).Rotation());
                PC->BattleCamera->GetCameraComponent()->OrthoWidth=6000.f;
                PC->SetViewTarget(PC->BattleCamera);
                // Same matte placement as normal PlayerTick, now frozen to
                // this camera rather than to the OS cursor's previous position.
                const double MatteCentre=.6*CentreX;
                const double MatteWidth=FMath::Max(12500.,6000.+2*FMath::Abs(CentreX-MatteCentre)+400.);
                const double MatteHeight=4200.*MatteWidth/12500.;
                for(TActorIterator<AActor> It(World);It;++It)if(It->ActorHasTag(TEXT("WF_DistantMatte_V12")))
                {It->SetActorLocation(FVector(MatteCentre,-5200,180.-.4*MatteHeight));It->SetActorScale3D(FVector(MatteWidth/100.,1,MatteHeight/100.));break;}
            }
            for(int32 Team=0;Team<2;++Team)Run->Ready&=B->SpawnSquad(Team,EFrontlineRole::Rifle,Team?2:1);
            for(AWarfareFrontlineSoldier* U:B->Units)if(U)U->SetActorTickEnabled(false);
            Run->Ready&=B->Phase==EFrontlinePhase::Fighting;
            Run->Stage=1;Run->Next=Now+3;break;
        case 1:
            // The fixed normal camera/matte has settled; warm the three profiles.
            SpawnProfiles();
            Run->Stage=2;Run->Next=Now+10.8;break;
        case 2:
            for(TActorIterator<AWarfareFrontlineGround> It(World);It;++It)
            {
                UStaticMesh* Mesh=It->Surface?It->Surface->GetStaticMesh():nullptr;
                if(Mesh&&Mesh->GetRenderData()&&Mesh->GetRenderData()->LODResources.Num())
                {
                    const auto& LOD=Mesh->GetRenderData()->LODResources[0];
                    Run->Ground->SetNumberField(TEXT("vertices"),LOD.GetNumVertices());Run->Ground->SetNumberField(TEXT("triangles"),LOD.GetNumTriangles());
                    auto* Material=It->Surface->GetMaterial(0);Run->Ground->SetStringField(TEXT("material"),Material?Material->GetPathName():TEXT("missing"));
                }
                break;
            }
            if(!Run->Ground->HasField(TEXT("triangles"))){Run->Ready=false;Run->Failure=TEXT("Ground render data missing");}
            Run->BurstAt=Now;Snap(TEXT("00-ground-before-review"));
            Run->Stage=3;Run->Next=Now+1;break;
        case 3:
            SpawnProfiles();Run->BurstAt=Now;Run->Stage=4;Run->Next=Now+.045;break;
        case 4:
        {
            const double Times[]={.045,.40,1.20,3.30,8.0};
            const TCHAR* Names[]={TEXT("01-flash"),TEXT("02-dirt-kick"),TEXT("03-local-dust"),TEXT("04-grenade-cleared"),TEXT("05-shell-tail")};
            Snap(Names[Run->Capture]);++Run->Capture;
            if(Run->Capture<5)Run->Next=Run->BurstAt+Times[Run->Capture];else{Run->Stage=5;Run->Next=Now+1;}
            break;
        }
        default:
        {
            auto Report=MakeShared<FJsonObject>();int32 Written=0;
            for(const auto& Image:Run->Captures)if(IFileManager::Get().FileExists(*Image->AsObject()->GetStringField(TEXT("file"))))++Written;
            Report->SetBoolField(TEXT("fixture_ready"),Run->Ready&&Written==6);Report->SetStringField(TEXT("failure"),Run->Failure);
            Report->SetNumberField(TEXT("screenshots_written"),Written);Report->SetArrayField(TEXT("captures"),Run->Captures);Report->SetObjectField(TEXT("ground"),Run->Ground);
            Report->SetStringField(TEXT("scope"),TEXT("Opt-in isolated visual fixture; normal 6000cm camera, two frozen real rifle squads for scale, warm-up burst followed by grenade/mortar/artillery visual profiles. No damage, AI, balance, death-physics or performance acceptance. Component state is not proof of visible pixels. Snapshot metadata is recorded at screenshot request; render can be one frame later. Root must inspect actual six images, especially short trench flash and near-ground smoke, and review emitted terrain triangle budget. Run with a separate UserDir; no campaign victory is synthesized."));
            FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Folder/TEXT("field-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V23_FIELD_REVIEW_DONE ready=%d images=%d %s"),int32(Run->Ready),Written,*Run->Folder);
            FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
