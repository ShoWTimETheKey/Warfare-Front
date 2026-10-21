#include "WarfareFrontlineEnvironmentV40.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineMotionV40.h"
#include "WarfareFrontlinePrecipitation.h"
#include "DynamicRHI.h"
#include "RenderTimer.h"

// Focused production-actor review. Every mutation uses an isolated save directory.
namespace
{
void StartFrontlineV40Review(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV40Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    FString UserDir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
    struct FRun
    {
        int32 Stage=0,Shots=0;bool Passed=true,Actions=false,SmokeOnly=false;int32 Perf=0;
        double Next=FPlatformTime::Seconds()+3,Start=Next,SampleStart=0;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks;
        TArray<double> GPU[3],CPU[3];TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
        TWeakObjectPtr<AFrontlineSupportMissionV28> Gas;
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V40/Review"));
    R->SmokeOnly=FParse::Param(FCommandLine::Get(),TEXT("WFV40SmokeOnly"));if(R->SmokeOnly)R->Stage=14;
    IFileManager::Get().MakeDirectory(*R->Directory,true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    V27ImageAuditActive=V36TypographyAuditActive=true;V36TypographyAudit.Empty();V28ReviewPointer=FVector2D(1790,110);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float DT)
    {
        const double Now=FPlatformTime::Seconds();auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;if(!B||!HUD)return false;
        if(R->Actions)FrontlineMotionV40::TickFieldActionReview(B,DT);
        if(R->Perf&&Now-R->SampleStart>.6)
        {
            const double GPU=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
            if(GPU>0&&GPU<1500){R->GPU[R->Perf-1].Add(GPU);R->CPU[R->Perf-1].Add(FPlatformTime::ToMilliseconds(GGameThreadTime));}
        }
        if(Now<R->Next)return true;
        // Screenshot writes and first-use shader loads can stall wall time. Advance
        // the fixture to the actual pose phase before capturing, without skipping
        // the animation or mistaking a half-finished action for its final state.
        if(R->Actions&&R->Stage>=22&&R->Stage<=25&&Now-R->Start<=140)
        {
            const double PhaseSeconds[]={.35,1.30,2.30,4.80};
            if(FrontlineMotionV40::FieldActionReviewState(B)->GetNumberField(TEXT("review_game_seconds"))<PhaseSeconds[R->Stage-22])return true;
        }
        auto& S=ScreenState(PC);auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto CheckNamed=[&](const TCHAR* Name,bool Good){Check(Name,Good);};
        auto Shot=[&](const TCHAR* Name){++R->Shots;FScreenshotRequest::RequestScreenshot(R->Directory/(FString(Name)+TEXT(".png")),true,false);};
        auto Clear=[&]()
        {
            R->Actions=false;R->Perf=0;UGameplayStatics::SetGamePaused(PC,false);
            S.Menu=S.Help=S.Settings=S.Technology=S.UnitIntroduction=S.Preparing=S.Deployment=S.HistoricalDispatch=false;
            S.bPausedByOverlay=S.bPausedForAuxOverlay=false;PC->CancelTargeting();V28ReviewPointer=FVector2D(1790,110);
        };
        auto Select=[&](EFrontlineFaction F,int32 Mission)
        {
            Clear();
            for(TActorIterator<AActor> It(B->GetWorld());It;++It)
                if(It->ActorHasTag(TEXT("V40AirPhaseFixture"))||It->ActorHasTag(TEXT("V38ImpactPhaseFixture")))It->Destroy();
            B->AbandonBattleToOperations();B->StartNewCampaign(F,EFrontlineDifficulty::Veteran);
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))Run->CompletedMissions=Mission;
            Check(TEXT("select_operation_")+FString::FromInt(R->Stage),B->SelectCampaignMission(Mission));
            S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;PC->ResetCamera();
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){Run->RoleSelectionMask=0;Run->SupportSelectionMask=0;}
            B->BeginBattle();B->Elapsed=130;B->SetActorTickEnabled(false);
        };
        auto Environment=[&](const TCHAR* Key)
        {
            const auto J=FrontlineEnvironmentV40::RunChecks(B);R->Report->SetObjectField(Key,J);Check(FString(Key)+TEXT("_assets_instances_and_contact_policy"),J->GetBoolField(TEXT("success")));
        };
        if(Now-R->Start>140){Check(TEXT("bounded_review_timeout"),false);R->Stage=99;}
        if(R->SmokeOnly&&R->Stage==21)R->Stage=26;
        double Delay=.55;
        switch(R->Stage)
        {
        case 0:
            Check(TEXT("isolated_volatile_profile"),B->IsProgressionVolatile());if(!R->Passed){R->Stage=99;break;}
            FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);
            if(Render){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            Select(EFrontlineFaction::British,2);Environment(TEXT("wet_battlefield"));
            FFrontlineReviewV32::Camera(PC,0,0,6200,350);Delay=2;break;
        case 1:Shot(TEXT("01-wet-battlefield-normal-play-view"));break;
        case 2:FFrontlineReviewV32::Camera(PC,0,750,2200,100);Delay=1;break;
        case 3:Shot(TEXT("02-trench-bags-revetment-and-supply-bays"));break;
        case 4:
        {
            Select(EFrontlineFaction::German,4);auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());
            Check(TEXT("German_first_armour_operation_has_only_A7V"),V&&V->IsUnlocked(EFrontlineVehicle::A7V,0)&&!V->IsUnlocked(EFrontlineVehicle::BeuteMarkIV,0));
            Select(EFrontlineFaction::German,5);V=UFrontlineVehicleSubsystem::Get(B->GetWorld());
            if(V){const auto J=V->RunArmourChecksV40();R->Report->SetObjectField(TEXT("armour_readiness"),J);Check(TEXT("actual_shared_armour_queue"),J->GetBoolField(TEXT("success")));V->ResetReadinessForDebug(0);const auto Roster=V->GetRoster(0);Check(TEXT("actual_cooling_armour_HUD"),Roster.Num()==3&&V->Recruit(Roster[0].Type,0));}
            else Check(TEXT("armour_subsystem_available"),false);
            FFrontlineReviewV32::Camera(PC,0,0,6000,300);Delay=1;break;
        }
        case 5:Shot(TEXT("03-German-shared-armour-readiness"));break;
        case 6:Select(EFrontlineFaction::British,8);Environment(TEXT("operational_railway"));FFrontlineReviewV32::Camera(PC,0,-1130,4600,160);Delay=1;break;
        case 7:Shot(TEXT("04-worn-operational-rail-and-field"));break;
        case 8:
            Select(EFrontlineFaction::British,5);FrontlineSupportArtV40::RunChecks(B,CheckNamed);
            Check(TEXT("forward_strafing_visual_fixture"),FrontlineSupportArtV40::SetupFlightFixture(B,1.5f));
            FFrontlineReviewV32::Camera(PC,0,0,13000,900);Delay=1;break;
        case 9:Shot(TEXT("05-forward-downward-strafing"));break;
        case 10:Check(TEXT("pullout_visual_fixture"),FrontlineSupportArtV40::SetupFlightFixture(B,4.2f));FFrontlineReviewV32::Camera(PC,0,0,9000,1100);break;
        case 11:Shot(TEXT("06-bank-and-pullout"));break;
        case 12:
        {
            Check(TEXT("weathered_airframe_visual_fixture"),FrontlineSupportArtV40::SetupFlightFixture(B,1.9f));
            const auto F=FrontlineSupportArtV40::AttackFlight(FVector(-1250,-1150,AWarfareFrontlineGround::HeightAt(-1250,-1150)),1,1.9f,0);
            FFrontlineReviewV32::Camera(PC,F.Position.X,F.Position.Y,2100,F.Position.Z);Delay=1;break;
        }
        case 13:Shot(TEXT("07-airframe-canvas-paint-wood-metal-inspection"));break;
        case 14:
            if(R->SmokeOnly){Check(TEXT("isolated_volatile_profile"),B->IsProgressionVolatile());FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);if(Render){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}}
            Select(EFrontlineFaction::British,2);FrontlineImpactV40::RunChecks(B,CheckNamed);
            FFrontlineReviewV32::Camera(PC,0,150,5000,180);R->Perf=1;R->SampleStart=FPlatformTime::Seconds();Delay=3;break;
        case 15:R->Perf=0;Check(TEXT("impact_flash_fixture"),FrontlineImpactV40::SetupVisualFixture(B,.12f));break;
        case 16:Shot(TEXT("08-grenade-mortar-shell-impact"));break;
        case 17:Check(TEXT("impact_soil_fixture"),FrontlineImpactV40::SetupVisualFixture(B,.55f));break;
        case 18:Shot(TEXT("09-directed-earth-and-fragments"));break;
        case 19:Check(TEXT("impact_volume_fixture"),FrontlineImpactV40::SetupVisualFixture(B,1.8f));R->Perf=2;R->SampleStart=FPlatformTime::Seconds();Delay=3;break;
        case 20:R->Perf=0;Shot(TEXT("10-layered-volume-aftermath"));break;
        case 21:
            Select(EFrontlineFaction::British,5);FrontlineMotionV40::RunFocusedChecks(B,Check);
            if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->RunV40AudioChecks(Check);
            else Check(TEXT("actual_audio_subsystem_available"),false);
            FrontlineMotionV40::PrepareFieldActionReview(B);R->Actions=true;
            FFrontlineReviewV32::Camera(PC,0,0,2600,100);Delay=.45;break;
        case 22:R->Report->SetObjectField(TEXT("field_actions_start"),FrontlineMotionV40::FieldActionReviewState(B));Shot(TEXT("11-mask-and-wire-actions-start"));Delay=.55;break;
        case 23:
        {
            const auto J=FrontlineMotionV40::FieldActionReviewState(B);R->Report->SetObjectField(TEXT("field_actions_contact"),J);
            const auto& Rows=J->GetArrayField(TEXT("actors"));bool Contact=Rows.Num()==4;
            for(int32 I=0;I<FMath::Min(3,Rows.Num());++I)
            {const auto A=Rows[I]->AsObject();Contact&=A->GetNumberField(TEXT("action"))!=0&&A->GetNumberField(TEXT("action_seconds"))<2.2;}
            Check(TEXT("visual_action_capture_has_not_skipped_to_idle_during_loading"),Contact);
            Shot(TEXT("12-mask-seal-and-wire-cut"));Delay=.2;break;
        }
        case 24:R->Report->SetObjectField(TEXT("field_actions_struggle"),FrontlineMotionV40::FieldActionReviewState(B));Shot(TEXT("13-gas-casualty-struggle"));Delay=.2;break;
        case 25:
        {
            const auto J=FrontlineMotionV40::FieldActionReviewState(B);R->Report->SetObjectField(TEXT("field_actions_end"),J);
            for(const TCHAR* Key:{TEXT("don_complete_and_sealed"),TEXT("doff_complete_and_removed"),TEXT("wire_completed_and_consumed"),TEXT("wire_removed_after_completed_action")})Check(FString(TEXT("captured_"))+Key,J->GetBoolField(Key));
            const auto& Actors=J->GetArrayField(TEXT("actors"));const auto Gas=Actors.Num()==4?Actors[3]->AsObject():nullptr;
            Check(TEXT("captured_complete_gas_fall_and_valid_physics_handoff"),Gas&&Gas->GetBoolField(TEXT("gas_authored_fall_complete"))&&Gas->GetBoolField(TEXT("gas_physics_started"))&&Gas->GetBoolField(TEXT("gas_physics_finite")));
            Shot(TEXT("14-gas-casualty-settles"));R->Actions=false;break;
        }
        case 26:
        {
            Select(EFrontlineFaction::British,3);FFrontlineReviewV32::Camera(PC,0,0,5600,230);
            auto* Gas=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();R->Gas=Gas;
            const FVector Target(0,0,AWarfareFrontlineGround::HeightAt(0,0));
            Check(TEXT("actual_gas_round_mission_configured"),Gas&&Gas->Configure(B,EFrontlineSupport::Gas,Target,0));
            B->Elapsed+=B->FireSupportDelayRange(EFrontlineSupport::Gas,Target,0).Y+6.f;
            if(Gas)Gas->Tick(.01f);
            TArray<UStaticMeshComponent*> Proxies;if(Gas)Gas->GetComponents(Proxies);int32 Volumes=0;
            for(auto* C:Proxies)if(C->IsVisible()&&C->GetStaticMesh()&&C->GetStaticMesh()->GetName()==TEXT("Cube"))++Volumes;
            Check(TEXT("gas_has_six_visible_three_dimensional_proxies_and_actual_exposure_zone"),Volumes==6&&!B->SmokeZones.IsEmpty());
            R->Perf=3;R->SampleStart=FPlatformTime::Seconds();Delay=3;break;
        }
        case 27:R->Perf=0;Shot(TEXT("15-gas-volumes-normal-play-camera"));break;
        case 28:FFrontlineReviewV32::Camera(PC,220,550,2800,120);B->Elapsed+=.01f;if(R->Gas.IsValid())R->Gas->Tick(.01f);Delay=.8;break;
        case 29:Shot(TEXT("16-gas-depth-around-earthworks"));break;
        default:
        {
            R->Perf=0;R->Actions=false;Check(TEXT("all_changed-area_captures_requested"),R->Shots==(R->SmokeOnly?5:16));
            bool RainReady=false;for(TActorIterator<AWarfareFrontlinePrecipitation> It(B->GetWorld());It;++It)RainReady|=It->IsVisualReady();
            Check(TEXT("precipitation_materials_and_instance_pools_ready"),RainReady);
            TArray<TSharedPtr<FJsonValue>> Typography;int32 Bad=0,Truncated=0;
            for(const auto& E:V36TypographyAudit){Typography.Add(MakeShared<FJsonValueObject>(E.Value));Bad+=!E.Value->GetBoolField(TEXT("fits"));Truncated+=E.Value->GetBoolField(TEXT("truncated"));}
            Check(TEXT("affected_text_fits"),Bad==0);Check(TEXT("no_new_paragraph_ellipsis"),Truncated==0);
            Check(TEXT("visible_English_coverage"),FrontlineLocalizationV35::WriteCoverageReport());
            auto Perf=MakeShared<FJsonObject>();
            for(int32 I=0;I<3;++I)
            {
                auto J=MakeShared<FJsonObject>();auto Values=R->GPU[I];Values.Sort();double Mean=0,CPU=0;
                for(double V:Values)Mean+=V;for(double V:R->CPU[I])CPU+=V;
                J->SetNumberField(TEXT("samples"),Values.Num());J->SetNumberField(TEXT("gpu_mean_ms"),Mean/FMath::Max(1,Values.Num()));J->SetNumberField(TEXT("game_thread_mean_ms"),CPU/FMath::Max(1,R->CPU[I].Num()));
                J->SetNumberField(TEXT("gpu_p95_ms"),Values.IsEmpty()?0:Values[FMath::Min(Values.Num()-1,FMath::FloorToInt(Values.Num()*.95))]);
                Perf->SetObjectField(I==2?TEXT("six_gas_clouds"):I?TEXT("three_aftermath_volumes"):TEXT("same_field_without_impacts"),J);
                Check(TEXT("bounded_render_cost_sample_")+FString::FromInt(I),Values.Num()>30);
            }
            int32 W=0,H=0;PC->GetViewportSize(W,H);Perf->SetNumberField(TEXT("width"),W);Perf->SetNumberField(TEXT("height"),H);
            if(auto* Pool=IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.ResidentGeometryMemoryPoolSizeInMB")))Perf->SetNumberField(TEXT("ray_geometry_pool_mib"),Pool->GetInt());
            Perf->SetStringField(TEXT("scope"),TEXT("Short fixed gameplay-camera GPU render cost; FG off, real new field geometry/materials and three explosion aftermaths. Not a full-battle FPS or other-hardware guarantee."));
            R->Report->SetObjectField(TEXT("render_cost"),Perf);R->Report->SetArrayField(TEXT("typography"),Typography);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetBoolField(TEXT("success"),R->Passed);
            R->Report->SetNumberField(TEXT("screenshots_requested"),R->Shots);R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetBoolField(TEXT("smoke_material_only"),R->SmokeOnly);
            FString Text;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Directory/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            V28ReviewPointer.Reset();V27ImageAuditActive=V36TypographyAuditActive=false;FPlatformMisc::RequestExit(false);return false;
        }
        }
        ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
    }));
}
}
