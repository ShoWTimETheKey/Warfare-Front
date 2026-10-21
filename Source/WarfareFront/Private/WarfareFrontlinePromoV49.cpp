#include "WarfareFrontlinePromoV49.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineFXV47.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineSupportV32.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineTerrainV42.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
bool AllowedPromo49(const AWarfareFrontlineGameMode* B)
{
    FString Dir;
    return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFPromoV49"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty()&&!FPaths::IsRelative(Dir);
}
const TCHAR* SceneName49(int32 Scene)
{
    const TCHAR* Names[]={TEXT("british-combined-advance"),TEXT("german-armoured-assault"),
        TEXT("flame-engineers-and-gas"),TEXT("selle-railway-and-air-support")};
    return Names[FMath::Clamp(Scene,0,3)];
}
}

// This adapter changes only a disposable encounter's initial deployment and
// player camera. Production movement, weapons, damage and support clocks run.
struct FFrontlinePromoV49
{
    static bool Ready(AWarfareFrontlineGameMode* B)
    {return B&&B->HasActorBegunPlay()&&B->LoadedSoldier&&B->IdleAnimation&&B->AssetFailure.IsEmpty();}

    static bool Section(AWarfareFrontlineGameMode* B,int32 Team,EFrontlineRole Role,
                        float X,float Y,bool Advance,int32 Row)
    {
        if(!B->IsRoleUnlocked(Role,Team)||!B->SpawnSquad(Team,Role))return false;
        auto& S=B->Squads.Last();S.Trench=INDEX_NONE;S.bReinforcement=false;
        S.bAdvancing=Advance;S.bHoldPosition=!Advance;S.Slot=FMath::Clamp(Row,0,2);
        S.Destination=FrontlineBattlefield::CentreTrench;
        S.LastPassedTrench=FrontlineBattlefield::CentreTrench-B->GetTeamDirection(Team);
        const int32 Dir=B->GetTeamDirection(Team);
        for(int32 I=0;I<S.Members.Num();++I)if(auto* U=S.Members[I].Get())
        {
            // Normal complete roster and original per-man variation are retained.
            FVector P(X-Dir*(I%3)*67.f,Y+(I-(S.Members.Num()-1)*.5f)*105.f,0);
            P+=U->FormationOffset;P.Y=FMath::Clamp(P.Y,-1050.f,1050.f);
            P.Z=FrontlineInfantryV45::SurfaceHeight(B,P)+88.f;
            U->bFieldEntered=true;U->CommandReadyAt=B->Elapsed;
            U->SetActorHiddenInGame(false);U->SetActorLocation(P);
            U->SetActorRotation(FRotator(0,Dir>0?0.f:180.f,0));
        }
        return S.LivingCount()>0;
    }

    static bool Vehicle(AWarfareFrontlineGameMode* B,EFrontlineVehicle Type,int32 Team,float X,float Y)
    {
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Fleet)return false;
        Fleet->ResetReadinessForDebug(Team);
        if(!Fleet->Recruit(Type,Team))return false;
        AFrontlineTank* T=Fleet->GetVehicles().Last().Get();if(!T)return false;
        T->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)));
        T->Simulate(.001f);return T->AreAssetsReady();
    }

    static bool Prepare(AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,int32 Scene,bool ArtifactProbe=false)
    {
        if(!AllowedPromo49(B))return false;
        UGameplayStatics::SetGamePaused(B,false);
        const bool German=Scene==1||Scene==3;const int32 Mission=Scene==3?17:16;
        if(!FrontlineCampaignV41::PrepareReviewOperation(B,German?EFrontlineFaction::German:EFrontlineFaction::British,Mission))return false;
        // Six available communication choices, selected before entering battle.
        // This is a legal late-operation loadout, not a bypass of chapter locks.
        for(auto& R:B->CampaignRuns)if(R.Id==B->ActiveCampaignId)
            R.SupportSelectionMask=(1u<<int32(EFrontlineSupport::Artillery))|(1u<<int32(EFrontlineSupport::Smoke))
                |(1u<<int32(EFrontlineSupport::Mortar))|(1u<<int32(EFrontlineSupport::Gas))
                |(1u<<int32(EFrontlineSupport::AirStrike))|(1u<<int32(EFrontlineSupport::AirBombing));
        B->BeginBattle();B->SetBattleSpeed(1.f);B->Elapsed=45.f;
        FrontlineReviewUIV41::Clear(PC);FrontlineReviewUIV41::Pointer(FVector2D(-10000,-10000));
        PC->bShowMouseCursor=false;PC->SetActorTickEnabled(false);
        const int32 D=B->GetTeamDirection(0);bool Good=B->Phase==EFrontlinePhase::Fighting&&B->AssetFailure.IsEmpty();
        // Stage existing deployed squads; do not retain queued off-map openings.
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Empty();B->Squads.Empty();
        if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld())){Fleet->ResetBattle();Fleet->Tick(0.f);}
        Good&=Section(B,0,EFrontlineRole::Rifle,-1250*D,-650,true,0);
        Good&=Section(B,0,EFrontlineRole::Lewis,-900*D,-170,false,1);
        Good&=Section(B,0,EFrontlineRole::Mills,-680*D,270,true,2);
        Good&=Section(B,0,EFrontlineRole::Officer,-1550*D,420,true,2);
        Good&=Section(B,1,EFrontlineRole::Rifle,1100*D,-580,false,0);
        Good&=Section(B,1,EFrontlineRole::Lewis,1430*D,60,false,1);
        Good&=Section(B,1,EFrontlineRole::Mills,1550*D,550,false,2);
        if(Scene==2)
        {
            Good&=Section(B,0,EFrontlineRole::Flamethrower,-400*D,-620,true,0);
            Good&=Section(B,0,EFrontlineRole::Pioneer,-1150*D,720,true,2);
            Good&=Section(B,1,EFrontlineRole::Rifle,430*D,-720,false,0);
        }
        else if(Scene==3)
        {
            Good&=Section(B,0,EFrontlineRole::Pioneer,-1200*D,830,true,2);
            const auto* M=B->GetEncounterCampaignMission();Good&=M&&M->bArmoredTrain;
            if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
            {
                auto* Train=M&&M->bArmoredTrain?Fleet->SpawnScenarioRailV32(M->TrainTeam):nullptr;
                Good&=Train!=nullptr;
                if(Train){Train->SetActorLocation(FVector(1100,-1130,28));Train->Simulate(.001f);}
            }
        }
        else
        {
            Good&=Vehicle(B,ArtifactProbe?EFrontlineVehicle::MarkIVFemale:German?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale,0,-1350*D,820);
            Good&=Vehicle(B,German?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V,1,1700*D,840);
            Good&=Section(B,0,German?EFrontlineRole::AntiTankRifle:EFrontlineRole::RifleGrenadier,-1600*D,-880,true,0);
        }
        // Initial orders are ready in this staged engagement; only subsequent
        // real support calls create salvos, projectiles, damage and visuals.
        for(int32 Team=0;Team<2;++Team)
        {
            B->ArtilleryReadyAt[Team]=B->SmokeReadyAt[Team]=B->MortarReadyAt[Team]=B->Elapsed;
            for(float& V:B->ExtendedSupportReadyAt[Team])V=B->Elapsed;
            FrontlineSupportV32::ResetReadinessForDebug(B,Team);
        }
        B->SetActorTickEnabled(true);UGameplayStatics::SetGamePaused(B,true);return Good;
    }

    static void Camera(AWarfareFrontlineController* PC,float Width,float PanY)
    {
        if(!PC||!PC->BattleCamera)return;
        int32 W=0,H=0;PC->GetViewportSize(W,H);
        Width=FMath::Clamp(Width,3400.f,9000.f);
        PanY=FMath::Clamp(PanY,-FrontlineTerrainV42::CameraVerticalLimitV43(Width,H>0?float(W)/H:16.f/9.f),
                                  FrontlineTerrainV42::CameraVerticalLimitV43(Width,H>0?float(W)/H:16.f/9.f));
        PC->CameraFocus=FVector(0,PanY,0);PC->CameraWidth=PC->TargetCameraWidth=Width;
        PC->CameraPanVelocity=PC->CameraVerticalVelocity=0;
        const FVector Focus=PC->CameraFocus+FVector(0,0,650),Location=PC->CameraFocus+FVector(0,14000,4650);
        PC->BattleCamera->SetActorLocation(Location);PC->BattleCamera->SetActorRotation((Focus-Location).Rotation());
        PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;
        PC->SetViewTarget(PC->BattleCamera);
        FrontlineTerrainV42::UpdateStreaming(PC->GetWorld(),Location,Focus,Width,W);
        // Exactly the normal player-camera backdrop update; no custom placement,
        // scale, material or reflection override is introduced for photography.
        if(PC->DistantMatte.IsValid())FrontlineTerrainV42::PlaceBackdropV43(
            PC->DistantMatte.Get(),PC->CameraFocus,Width,H>0?float(W)/H:16.f/9.f);
    }

    static TSharedRef<FJsonObject> Describe(AWarfareFrontlineGameMode* B)
    {
        auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Units,Vehicles;
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))
        {
            auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("team"),U->Team);V->SetNumberField(TEXT("role"),int32(U->Role));
            V->SetBoolField(TEXT("alive"),U->IsAlive());V->SetNumberField(TEXT("shots"),U->ShotsDischarged);
            V->SetStringField(TEXT("feet"),U->Feet().ToString());V->SetBoolField(TEXT("finite"),U->GetActorTransform().IsValid());Units.Add(MakeShared<FJsonValueObject>(V));
        }
        if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* T:Fleet->GetVehicles())if(IsValid(T))
        {
            auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("team"),T->Team);V->SetBoolField(TEXT("rail"),T->IsRailConvoyV32());
            V->SetNumberField(TEXT("type"),int32(T->Type));V->SetBoolField(TEXT("assets_ready"),T->AreAssetsReady());
            V->SetBoolField(TEXT("female"),T->Type==EFrontlineVehicle::MarkIVFemale||T->Type==EFrontlineVehicle::BeuteMarkIVFemale);
            V->SetNumberField(TEXT("cannon_shots"),T->CannonShots);V->SetNumberField(TEXT("mg_shots"),T->MachinegunShots);
            V->SetNumberField(TEXT("hull"),T->GetHullIntegrity());V->SetStringField(TEXT("position"),T->GetActorLocation().ToString());Vehicles.Add(MakeShared<FJsonValueObject>(V));
        }
        J->SetArrayField(TEXT("units"),Units);J->SetArrayField(TEXT("vehicles"),Vehicles);J->SetNumberField(TEXT("elapsed"),B->Elapsed);
        J->SetNumberField(TEXT("phase"),int32(B->Phase));J->SetNumberField(TEXT("live_strikes"),B->Strikes.Num());
        J->SetNumberField(TEXT("living_player_soldiers"),B->LivingSoldiers(0));J->SetNumberField(TEXT("living_enemy_soldiers"),B->LivingSoldiers(1));
        J->SetNumberField(TEXT("firearm_shots"),B->ShotsFired);J->SetNumberField(TEXT("artillery_orders"),B->ArtilleryCommands);
        J->SetNumberField(TEXT("mortar_orders"),B->MortarCommands);J->SetNumberField(TEXT("mortar_rounds_resolved"),B->MortarRoundsResolved);
        int32 SupportRounds=0;for(TActorIterator<AFrontlineSupportMissionV28> It(B->GetWorld());It;++It)SupportRounds+=It->RoundsFired;
        J->SetNumberField(TEXT("extended_support_rounds_fired"),SupportRounds);
        if(const auto* M=B->GetEncounterCampaignMission()){J->SetNumberField(TEXT("operation"),M->Id);J->SetStringField(TEXT("operation_title"),M->Title);}
        return J;
    }
};

void StartFrontlinePromoV49(AWarfareFrontlineGameMode* Battle)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFPromoV49")))return;
    if(!AllowedPromo49(Battle)){UE_LOG(LogTemp,Error,TEXT("WF_PROMO_REFUSED: isolated absolute UserDir and WFRenderNoSave required"));FPlatformMisc::RequestExit(false);return;}
    struct FRun
    {
        TWeakObjectPtr<AWarfareFrontlineGameMode> B;int32 First=0,Last=3,Scene=0,Stage=0,Frame=0;
        TWeakObjectPtr<AWarfareFrontlineSoldier> FlameOperator;
        TWeakObjectPtr<AFrontlineSupportMissionV28> AircraftMission;
        int32 LastFlameShots=0;float FlameDischargeAt=-1,FirstAircraftShotAt=-1;
        float Width=6000,PanY=400,GameStart=0,LastShot=-10;double Start=FPlatformTime::Seconds(),Next=Start;
        bool Good=true,ArtifactProbe=false,ProbeStayedFighting=true,ProbeStayedUnpaused=true;float ProbeStart=0;
        FString Dir;TArray<FString> Files;TArray<TSharedPtr<FJsonValue>> Captures,Orders;
    };
    auto R=MakeShared<FRun>();R->B=Battle;int32 Scene=-1;FParse::Value(FCommandLine::Get(),TEXT("WFPromoScene="),Scene);
    if(Scene>=0&&Scene<=3)R->First=R->Last=Scene;R->Scene=R->First;
    R->ArtifactProbe=FParse::Param(FCommandLine::Get(),TEXT("WFArtifactProbe"));
    if(R->ArtifactProbe)R->First=R->Last=R->Scene=0;
    FParse::Value(FCommandLine::Get(),TEXT("WFPromoWidth="),R->Width);FParse::Value(FCommandLine::Get(),TEXT("WFPromoPanY="),R->PanY);
    if(!FMath::IsFinite(R->Width)||!FMath::IsFinite(R->PanY)){FPlatformMisc::RequestExit(false);return;}
    R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Promotion/V49"));
    if(R->ArtifactProbe)R->Dir/=TEXT("ArtifactProbe");
    IFileManager::Get().MakeDirectory(*R->Dir,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float)
    {
        auto* B=R->B.Get();auto* PC=B?Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController()):nullptr;
        const double Now=FPlatformTime::Seconds();if(!B)return false;
        auto Finish=[&]()
        {
            auto J=MakeShared<FJsonObject>();bool Files=true;for(const FString& F:R->Files)Files&=IFileManager::Get().FileSize(*F)>1024;
            J->SetBoolField(TEXT("success"),R->Good&&Files&&!R->Files.IsEmpty());J->SetArrayField(TEXT("captures"),R->Captures);J->SetArrayField(TEXT("orders"),R->Orders);
            J->SetNumberField(TEXT("wall_seconds"),Now-R->Start);J->SetNumberField(TEXT("requested_width_cm"),R->Width);J->SetNumberField(TEXT("requested_pan_y_cm"),R->PanY);
            J->SetStringField(TEXT("scope"),TEXT("Private in-engine promotional staging on existing late campaign maps. Original production actors, art, materials, camera geometry, weapon damage and support delays. Actual temporal frames with HUD; no composite image, artificial blast, invulnerability or performance certification. Published payload unchanged."));
            if(R->ArtifactProbe)
            {
                J->SetBoolField(TEXT("artifact_probe"),true);
                J->SetBoolField(TEXT("success"),R->Good&&Files&&R->Frame==5&&R->ProbeStayedFighting&&R->ProbeStayedUnpaused);
                J->SetNumberField(TEXT("requested_unpaused_seconds"),25);
                J->SetNumberField(TEXT("observed_probe_world_seconds"),R->Stage>=2?B->GetWorld()->GetTimeSeconds()-R->ProbeStart:0);
                J->SetBoolField(TEXT("stayed_fighting"),R->ProbeStayedFighting);
                J->SetBoolField(TEXT("stayed_unpaused"),R->ProbeStayedUnpaused);
                J->SetBoolField(TEXT("inserted_frame_output_validated"),false);
                J->SetBoolField(TEXT("hdr_display_output_validated"),false);
                J->SetStringField(TEXT("limitations"),TEXT("Five engine-rendered output frames only, with per-request render diagnostics. Screenshots do not capture DLSS-generated presentation frames or certify HDR scan-out, absence of intermittent artifacts, or performance. FG is focus-dependent; consult each capture's effective mode and presented-frame count. Diagnostics precede asynchronous screenshot processing, not an atomic present sample. HDR screenshots retain linear scRGB in EXR. Probe preserves loaded render preferences and does not force FG, RR, HDR or resolution."));
            }
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))J->SetObjectField(TEXT("render_settings"),Render->GetDiagnostics());
            FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Dir/TEXT("capture.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);
        };
        if(Now-R->Start>270){R->Good=false;Finish();return false;}
        if(!PC||!PC->GetHUD()||!FFrontlinePromoV49::Ready(B)||!PC->BattleCamera)return true;
        float CameraWidth=R->Width,CameraPan=R->PanY;
        if(R->ArtifactProbe&&R->Stage>=2)
        {
            const float T=FMath::Clamp((B->GetWorld()->GetTimeSeconds()-R->ProbeStart)/25.f,0.f,1.f);
            CameraWidth*=1.f-.16f*FMath::Sin(T*2.f*PI);CameraPan+=420.f*FMath::Sin(T*2.f*PI);
            R->ProbeStayedFighting&=B->Phase==EFrontlinePhase::Fighting;
            R->ProbeStayedUnpaused&=!UGameplayStatics::IsGamePaused(B);
        }
        FFrontlinePromoV49::Camera(PC,CameraWidth,CameraPan);PC->bShowMouseCursor=false;
        // Observe real producer events every tick, not only when a photograph
        // is due. Counters precede valve opening and aircraft field entry.
        if(R->Stage>=2)
        {
            if(auto* U=R->FlameOperator.Get();U&&U->IsAlive()&&U->ShotsDischarged>R->LastFlameShots)
            {R->LastFlameShots=U->ShotsDischarged;R->FlameDischargeAt=B->Elapsed;}
            if(auto* A=R->AircraftMission.Get();A&&A->RoundsFired>0&&R->FirstAircraftShotAt<0)
                R->FirstAircraftShotAt=B->Elapsed;
        }
        if(FScreenshotRequest::IsScreenshotRequested()||Now<R->Next)return true;
        if(R->Stage==0)
        {
            if(!R->ArtifactProbe)if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))Render->SetHDREnabled(false,false);
            FrontlineSupportArtV36::PrewarmVisuals();FrontlineFXV47::Prewarm();
            R->Good&=FFrontlinePromoV49::Prepare(B,PC,R->Scene,R->ArtifactProbe);
            if(!R->Good){Finish();return false;}
            R->Frame=0;R->LastShot=-10;R->FlameOperator.Reset();R->AircraftMission.Reset();
            R->LastFlameShots=0;R->FlameDischargeAt=R->FirstAircraftShotAt=-1;
            for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->Team==0&&U->Role==EFrontlineRole::Flamethrower&&U->MemberIndex==0)
                {R->FlameOperator=U;R->LastFlameShots=U->ShotsDischarged;break;}
            R->Stage=1;R->Next=Now+7;return true;
        }
        if(R->Stage==1)
        {
            UGameplayStatics::SetGamePaused(B,false);R->GameStart=B->Elapsed;
            R->ProbeStart=B->GetWorld()->GetTimeSeconds();
            const int32 D=B->GetTeamDirection(0);
            auto Order=[&](EFrontlineSupport S,const FVector& P)
            {
                const bool Accepted=B->RequestSupport(S,P,0);auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("scene"),R->Scene);
                if(R->ArtifactProbe)R->Good&=Accepted;
                J->SetNumberField(TEXT("support"),int32(S));J->SetStringField(TEXT("target"),P.ToString());J->SetBoolField(TEXT("accepted"),Accepted);
                if(!Accepted)J->SetStringField(TEXT("reason"),B->SupportBlockReason(S));R->Orders.Add(MakeShared<FJsonValueObject>(J));
                if(Accepted&&S==EFrontlineSupport::AirStrike)
                {
                    UStaticMesh* Aircraft=FrontlineSupportArtV36::Aircraft(B->GetFactionForTeam(0)==EFrontlineFaction::German);
                    for(TActorIterator<AFrontlineSupportMissionV28> It(B->GetWorld());It;++It)
                    {
                        TArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
                        for(UStaticMeshComponent* C:Components)if(C&&C->GetStaticMesh()==Aircraft)R->AircraftMission=*It;
                    }
                }
            };
            Order(EFrontlineSupport::Artillery,FVector(750*D,700,0));
            if(R->Scene==2){Order(EFrontlineSupport::Gas,FVector(1150*D,350,0));Order(EFrontlineSupport::Smoke,FVector(100*D,850,0));}
            else if(R->Scene==3)Order(EFrontlineSupport::AirStrike,FVector(400*D,-350,0));
            else Order(EFrontlineSupport::Mortar,FVector(1400*D,700,0));
            if(R->ArtifactProbe)Order(EFrontlineSupport::Smoke,FVector(350*D,350,0));
            R->Stage=2;return true;
        }
        if(R->ArtifactProbe)
        {
            // Remain unpaused through readback: pause would suspend the actual FG
            // mode under investigation. This observes a real battle, not a frozen pose.
            if(R->Frame>=5){Finish();return false;}
            const float ProbeAge=B->GetWorld()->GetTimeSeconds()-R->ProbeStart;
            const float CaptureAt[]={2.f,7.f,12.f,18.f,25.f};
            if(ProbeAge<CaptureAt[R->Frame])return true;
            auto J=FFrontlinePromoV49::Describe(B);
            J->SetNumberField(TEXT("probe_world_seconds"),ProbeAge);
            J->SetNumberField(TEXT("battle_seconds_since_start"),B->Elapsed-R->GameStart);
            J->SetBoolField(TEXT("paused"),UGameplayStatics::IsGamePaused(B));
            J->SetNumberField(TEXT("camera_width_cm"),CameraWidth);
            J->SetStringField(TEXT("camera_focus"),FVector(0,CameraPan,0).ToString());
            if(auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD()))J->SetNumberField(TEXT("hud_page"),HUD->GetPresentationPage());
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))J->SetObjectField(TEXT("render_settings"),Render->GetDiagnostics());
            auto* Viewport=B->GetWorld()->GetGameViewport();
            const bool HDR=Viewport&&Viewport->Viewport&&Viewport->Viewport->GetSceneHDREnabled();
            const FString Path=R->Dir/FString::Printf(TEXT("artifact-probe-%02d.%s"),R->Frame+1,HDR?TEXT("exr"):TEXT("png"));
            J->SetStringField(TEXT("file"),Path);J->SetBoolField(TEXT("viewport_hdr"),HDR);
            J->SetBoolField(TEXT("native_rendered_frame_only"),true);
            R->Captures.Add(MakeShared<FJsonValueObject>(J));R->Files.Add(Path);
            FScreenshotRequest::RequestScreenshot(Path,true,false,HDR);
            ++R->Frame;R->Next=Now+.35;return true;
        }
        const float Age=B->Elapsed-R->GameStart;
        if(R->Stage==3){UGameplayStatics::SetGamePaused(B,false);R->Stage=2;}
        if(R->Frame>=4)
        {
            if(R->Scene>=R->Last){Finish();return false;}
            ++R->Scene;R->Stage=0;R->Next=Now+.5;return true;
        }
        const float Standard[]={2.8f,12.5f,18.f,25.f},Flame[]={.65f,1.3f,20.f,25.f},Rail[]={3.f,18.f,22.f,25.f};
        float Wanted=R->Scene==2?Flame[R->Frame]:R->Scene==3?Rail[R->Frame]:Standard[R->Frame];
        if(R->Scene==2&&R->Frame<2&&Age<8.f&&B->Phase==EFrontlinePhase::Fighting)
        {
            if(auto* U=R->FlameOperator.Get();U&&U->IsAlive())
            {
                const float Delay=R->Frame==0?.75f:1.05f;FVector Direction;
                if(R->FlameDischargeAt<0||B->Elapsed-R->FlameDischargeAt<Delay
                    ||!FrontlineFieldV32::FlameBraceDirectionV33(U,Direction))return true;
                Wanted=Age;
            }
        }
        if(R->Scene==3&&(R->Frame==1||R->Frame==2)&&Age<27.f&&B->Phase==EFrontlinePhase::Fighting)
        {
            if(R->AircraftMission.IsValid())
            {
                if(R->FirstAircraftShotAt<0)return true;
                Wanted=R->FirstAircraftShotAt-R->GameStart+(R->Frame==1?.90f:1.75f);
            }
        }
        bool Impact=false;
        if(R->Frame==1&&Age>8&&R->Scene<2)for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)
            if(It->GetVisualAgeSeconds()>.04f&&It->GetVisualAgeSeconds()<.22f&&FMath::Abs(It->GetActorLocation().X)<R->Width*.40f){Impact=true;break;}
        // If the real battle ends early, capture that truthful outcome rather
        // than forcing the scene back into combat or restoring casualties.
        if(Age<Wanted&&!Impact&&B->Phase==EFrontlinePhase::Fighting)return true;
        if(Age-R->LastShot<(R->Scene==2&&R->Frame<2?.20f:.32f)&&B->Phase==EFrontlinePhase::Fighting)return true;
        auto J=FFrontlinePromoV49::Describe(B);J->SetNumberField(TEXT("scene"),R->Scene);J->SetNumberField(TEXT("scene_game_seconds"),Age);
        J->SetBoolField(TEXT("observed_fresh_impact"),Impact);
        J->SetNumberField(TEXT("flame_discharge_at"),R->FlameDischargeAt);
        J->SetNumberField(TEXT("first_aircraft_shot_at"),R->FirstAircraftShotAt);
        const FString Name=FString::Printf(TEXT("%02d-%s-%02d.png"),R->Scene+1,SceneName49(R->Scene),R->Frame+1),Path=R->Dir/Name;
        J->SetStringField(TEXT("file"),Path);R->Captures.Add(MakeShared<FJsonValueObject>(J));R->Files.Add(Path);
        UGameplayStatics::SetGamePaused(B,true);FScreenshotRequest::RequestScreenshot(Path,true,false);
        ++R->Frame;R->LastShot=Age;R->Stage=3;R->Next=Now+.35;return true;
    }));
}
