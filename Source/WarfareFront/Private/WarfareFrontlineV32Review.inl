#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineWeather.h"
#include "Engine/PostProcessVolume.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"

struct FFrontlineReviewV32
{
    static float HonoursWheelV82(AWarfareFrontlineController* PC,int32 Direction=0)
    {
        FString UserDir;
        if(!PC||!FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty()||FPaths::IsRelative(UserDir))return -1;
        if(Direction<0)PC->ZoomIn();else if(Direction>0)PC->ZoomOut();
        return PC->TargetCameraWidth;
    }
    static void Camera(AWarfareFrontlineController* PC,float X,float Y,float Width,float FocusHeight=650,bool Cut=true)
    {
        PC->SetActorTickEnabled(false);PC->CameraFocus=FVector(X,Y,0);PC->CameraWidth=PC->TargetCameraWidth=Width;
        const FVector Focus(X,Y,FocusHeight),From=FVector(X,Y,0)+FVector(0,14000,4650);
        PC->BattleCamera->SetActorLocation(From);PC->BattleCamera->SetActorRotation((Focus-From).Rotation());
        PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;PC->SetViewTarget(PC->BattleCamera);
        // The fixture freezes controller ticks, so also place its production
        // parallax backdrop; otherwise the previous deployment camera leaves
        // a false black seam at the opposite viewport edge.
        int32 VW=0,VH=0;PC->GetViewportSize(VW,VH);
        if(PC->DistantMatte.IsValid())FrontlineTerrainV42::PlaceBackdropV43(PC->DistantMatte.Get(),PC->CameraFocus,Width,VH>0?float(VW)/VH:16.f/9.f);
        if(Cut&&PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();
    }
    static void Finish(AWarfareFrontlineGameMode* B,int32 Winner){B->FinishBattle(Winner,TEXT("前线停止抵抗。"));}
    static void Spawn(AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Team,int32 Trench)
    {B->SpawnSquad(Team,Role,Trench);}
};
namespace
{
void StartFrontlineV32Review(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV32Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    FString UserDir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
    struct FRun
    {
        int32 Stage=0,Shots=0,ShotsAtEnd=0;bool Passed=true;
        double Next=FPlatformTime::Seconds()+3,Start=Next,Prepared=0;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V32/Review"));
    IFileManager::Get().MakeDirectory(*R->Directory,true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    V27ImageAuditActive=true;V28ReviewPointer=FVector2D(1750,180);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
    {
        const double Now=FPlatformTime::Seconds();auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;if(!B||!HUD)return false;if(Now<R->Next)return true;
        auto& S=ScreenState(PC);auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const TCHAR* Name){++R->Shots;FScreenshotRequest::RequestScreenshot(R->Directory/(FString(Name)+TEXT(".png")),true,false);};
        auto Clear=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.UnitIntroduction=S.Preparing=S.Deployment=S.HistoricalDispatch=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;PC->CancelTargeting();};
        auto Select=[&](EFrontlineFaction F,int32 Mission)
        {
            Clear();B->Phase=EFrontlinePhase::Briefing;B->SetActorTickEnabled(true);B->StartNewCampaign(F,EFrontlineDifficulty::Veteran);
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){Run->CompletedMissions=9;Run->Research.TechnologyMask=(1u<<24)-1;Run->Research.ResearchPoints=25;}
            Check(TEXT("selected_operation_")+FString::FromInt(Mission)+FString::FromInt(int32(F)),B->SelectCampaignMission(Mission));
            S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;PC->ResetCamera();
        };
        auto Begin=[&]()
        {
            PC->SetActorTickEnabled(true);B->SetActorTickEnabled(true);
            for(int32 I=0;I<32&&!S.Preparing;++I){PC->BeginBattle();if(S.UnitIntroduction)HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));}
            R->Prepared=Now;
        };
        auto Ready=[&]()
        {
            if(S.Preparing&&Now-R->Prepared<35){R->Next=Now+.15;return false;}
            Check(TEXT("actual_operation_prepared_")+FString::FromInt(R->Stage),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&B->AssetFailure.IsEmpty());
            B->SetActorTickEnabled(false);B->Elapsed=180;return true;
        };
        if(Now-R->Start>130){Check(TEXT("bounded_focused_review_timeout"),false);R->Stage=99;}
        double Delay=1.1;
        switch(R->Stage)
        {
        case 0:
        {
            Check(TEXT("isolated_campaign_and_render_settings"),B->GetProgressionSaveStatus()==TEXT("独立验证档案"));
            if(!R->Passed){R->Stage=99;break;}
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            FrontlineCampaignV32::RunFocusedChecks(B,Check);
            if(Audio)Audio->RunV32AudioChecks(Check);else Check(TEXT("audio_subsystem_present"),false);
            bool Art=Artwork().IllustrationsV32&&Artwork().ArchivePhotosV32&&Artwork().FlagSplashesV32;
            for(auto& T:Artwork().LateMapsV32)Art&=T&&T->GetSizeX()>=1500&&T->GetResource();
            Check(TEXT("original_v32_portraits_photos_four_detailed_maps_loaded"),Art);
            Clear();S.Dossier=0;FrontlineFilmV32::SetEnabled(true,false);break;
        }
        case 1:Shot(TEXT("01-main-menu-no-key-legends"));break;
        case 2:PC->ToggleSettings();S.SettingsTab=0;Shot(TEXT("02-settings-film-grain-on"));break;
        case 3:
            HUD->NotifyHitBoxClick(TEXT("FilmGrain"));Check(TEXT("grain_toggle_changes_real_saved_preference"),!FrontlineFilmV32::IsEnabled());V28ReviewPointer=FVector2D(1100,710);Shot(TEXT("03-film-grain-toggle-tooltip"));break;
        case 4:
        {
            bool NativeOff=false;for(TActorIterator<APostProcessVolume> It(B->GetWorld());It;++It)if(It->Priority==100)NativeOff=It->Settings.bOverride_FilmGrainIntensity&&It->Settings.FilmGrainIntensity==0;
            Check(TEXT("grain_disabled_in_actual_weather_postprocess"),NativeOff);
            FrontlineFilmV32::SetEnabled(true,false);Select(EFrontlineFaction::British,7);V28ReviewPointer=FVector2D(214,724);Shot(TEXT("04-canal-operation-commanders"));break;
        }
        case 5:V28ReviewPointer=FVector2D(1750,180);PC->BeginBattle();Check(TEXT("historical_event_precedes_deployment"),S.HistoricalDispatch);Shot(TEXT("05-historical-dispatch"));break;
        case 6:HUD->NotifyHitBoxClick(TEXT("V32Acknowledge"));Check(TEXT("overflow_opens_roster_selection"),S.Deployment&&!S.HistoricalDispatch);Shot(TEXT("06-order-of-battle"));break;
        case 7:
            HUD->NotifyHitBoxClick(TEXT("V32Role2"));HUD->NotifyHitBoxClick(TEXT("V32Role5"));HUD->NotifyHitBoxClick(TEXT("V32Role3"));HUD->NotifyHitBoxClick(TEXT("V32Role6"));
            Check(TEXT("selected_specialists_reach_actual_field_slots"),FieldRolesV32(B).Contains(EFrontlineRole::Flamethrower)&&FieldRolesV32(B).Contains(EFrontlineRole::Pioneer)&&FieldRolesV32(B).Num()==5);
            V28ReviewPointer=FVector2D(1360,430);Shot(TEXT("07-roster-specialist-tooltip"));break;
        case 8:S.Deployment=false;S.Technology=true;S.TechnologyPage=1;V28ReviewPointer=FVector2D(482,377);Shot(TEXT("08-respirator-research"));break;
        case 9:S.Technology=false;S.Deployment=true;HUD->NotifyHitBoxClick(TEXT("V32DeploymentConfirm"));V28ReviewPointer=FVector2D(1750,180);Check(TEXT("new_unit_orders_follow_assignment"),S.UnitIntroduction);Shot(TEXT("09-flamethrower-introduction"));break;
        case 10:Begin();Delay=.15;break;
        case 11:
            if(!Ready())return true;
            FFrontlineReviewV32::Camera(PC,0,0,6200);Shot(TEXT("10-canal-traversed-battlefield"));break;
        case 12:
        {
            const int32 Before=B->Units.Num();
            FFrontlineReviewV32::Spawn(B,EFrontlineRole::Rifle,0,1);FFrontlineReviewV32::Spawn(B,EFrontlineRole::Pioneer,0,1);
            FFrontlineReviewV32::Spawn(B,EFrontlineRole::Flamethrower,0,1);FFrontlineReviewV32::Spawn(B,EFrontlineRole::Flamethrower,1,3);
            Check(TEXT("production_specialist_squads_spawned"),B->Units.Num()>Before+10);
            float MinY=MAX_flt,MaxY=-MAX_flt;
            for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==0&&U->IsAlive()){MinY=FMath::Min(MinY,float(U->Feet().Y));MaxY=FMath::Max(MaxY,float(U->Feet().Y));}
            Check(TEXT("actual_three_squad_formation_uses_expanded_depth"),MaxY-MinY>1100.f);
            FrontlineFieldV32::RunFocusedChecks(B,Check);
            int32 N=0;for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsAlive())
            {
                const float X=(N%7-3)*240.f,Y=(N/7-1)*440.f;U->bFieldEntered=true;U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));
                U->SetActorTickEnabled(false);U->ReturnToIdle();++N;
            }
            FFrontlineReviewV32::Camera(PC,0,350,3400,160);Shot(TEXT("11-expanded-formation-and-specialists"));break;
        }
        case 13:
        {
            FFrontlineSmokeZone Z;Z.bGas=true;Z.Centre=FVector(0,0,0);Z.Radius=2400;Z.StartedAt=B->Elapsed;Z.ExpiresAt=B->Elapsed+20;B->SmokeZones.Add(Z);
            for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsAlive())FrontlineFieldV32::TickUnit(U,.2f);
            R->Report->SetObjectField(TEXT("field_respirators"),FrontlineFieldV32::Review(B));
            Check(TEXT("visible_masks_and_french_helmets_in_real_squads"),FrontlineFieldV32::Review(B)->GetNumberField(TEXT("visible_respirators"))>0 && FrontlineFieldV32::Review(B)->GetNumberField(TEXT("french_adrian_helmets"))>0);
            FFrontlineReviewV32::Camera(PC,0,0,2450,130);Shot(TEXT("12-respirators-field-equipment"));break;
        }
        case 14:
        {
            B->SmokeZones.Empty();AWarfareFrontlineSoldier* F=nullptr,*T=nullptr;
            for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsAlive()){if(U->Team==0&&FrontlineFieldV32::IsFlameOperator(U))F=U;else if(U->Team==1)T=U;}
            if(F&&T){F->SetActorLocation(FVector(-280,120,AWarfareFrontlineGround::HeightAt(-280,120)+88));T->SetActorLocation(FVector(170,120,AWarfareFrontlineGround::HeightAt(170,120)+88));Check(TEXT("real_flamethrower_discharge_accepted"),FrontlineFieldV32::FlameDischarge(F,T));FrontlineFieldV32::TickUnit(F,.035f);}
            else Check(TEXT("flame_target_fixture_available"),false);
            R->Report->SetObjectField(TEXT("field_flame"),FrontlineFieldV32::Review(B));
            Check(TEXT("flame_discharge_enables_five_real_world_cards"),FrontlineFieldV32::Review(B)->GetNumberField(TEXT("live_flame_cards"))==5);
            Shot(TEXT("13-flamethrower-discharge"));break;
        }
        case 15:
        {
            FFrontlineReviewV32::Camera(PC,0,850,4300);
            PC->ConfirmSupportAtWorldPoint(FVector(0,850,0)); // No active target must issue no mission.
            const float Before=B->MortarReadyAt[0];B->MortarReadyAt[0]=0;
            const bool Accepted=B->RequestMortar(FVector(900,850,AWarfareFrontlineGround::HeightAt(900,850)));
            bool TwoDim=Accepted;for(const auto& Strike:B->Strikes)if(Strike.bMortar)TwoDim&=FMath::Abs(Strike.Target.Y)<=FrontlineBattlefield::HalfFrontage;
            Check(TEXT("vertical_mortar_aim_preserved_and_impacts_bounded"),TwoDim);
            PC->SupportTargetMode=4;PC->bPreviewValid=true;PC->ArtilleryPreview=FVector(900,850,AWarfareFrontlineGround::HeightAt(900,850));
            Shot(TEXT("14-vertical-camera-fire-support"));B->MortarReadyAt[0]=Before;break;
        }
        case 16:PC->CancelTargeting();V32FlagSheet=true;V32FlagAge=3;Shot(TEXT("15-distinct-national-flag-splatter"));break;
        case 17:V32FlagAge=.12f;Shot(TEXT("16-flag-liquid-arrival"));break;
        case 18:V32FlagSheet=false;Select(EFrontlineFaction::German,8);V28ReviewPointer=FVector2D(1770,180);Shot(TEXT("17-rail-operation-map"));break;
        case 19:Begin();Delay=.15;break;
        case 20:
            if(!Ready())return true;
            B->Elapsed=44;FrontlineFieldV32::Tick(B,.1f);
            if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* T:V->GetVehicles())if(T&&T->IsRailConvoyV32())T->SetActorLocation(FVector(0,-1130,28));
            FFrontlineReviewV32::Camera(PC,0,-600,6500);Delay=.4;break;
        case 21:Shot(TEXT("18-armoured-rail-sector"));R->Report->SetObjectField(TEXT("field_rail"),FrontlineFieldV32::Review(B));Check(TEXT("scenario_spawns_real_armoured_rail_convoy"),FrontlineFieldV32::Review(B)->GetNumberField(TEXT("rail_convoys"))==1);break;
        case 22:
        {
            FFrontlineReviewV32::Spawn(B,EFrontlineRole::Rifle,0,2);FFrontlineReviewV32::Spawn(B,EFrontlineRole::Rifle,1,3);
            int32 N=0;for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsAlive())
            {
                const float X=(N%6-3)*210.f,Y=(N/6)*380.f-300;U->bFieldEntered=true;U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));U->SetActorTickEnabled(true);++N;
            }
            FFrontlineReviewV32::Camera(PC,0,100,3500,160);B->ShotsFired=250;B->Elapsed=410;R->ShotsAtEnd=B->ShotsFired;
            FFrontlineReviewV32::Finish(B,0);B->SetActorTickEnabled(true);
            Check(TEXT("victory_starts_field_outro_before_report"),B->IsBattleOutroActive()&&HUD->GetPresentationPage()==3);
            Delay=2.6;break;
        }
        case 23:
            Check(TEXT("outro_freezes_weapons_while_bodies_continue"),B->ShotsFired==R->ShotsAtEnd&&B->IsBattleOutroActive());
            Check(TEXT("surrender_releases_physical_weapons"),FrontlineFieldV32::Review(B)->GetNumberField(TEXT("physical_dropped_weapons"))>0);
            Shot(TEXT("19-victory-surrender-field-outro"));R->Report->SetObjectField(TEXT("field_outcome"),FrontlineFieldV32::Review(B));Delay=5.3;break;
        case 24:Check(TEXT("field_report_waits_for_seven_second_outro"),!B->IsBattleOutroActive()&&HUD->GetPresentationPage()==6);Shot(TEXT("20-delayed-field-report"));break;
        case 25:Clear();B->Phase=EFrontlinePhase::Briefing;B->SelectCampaignMission(9);S.Dossier=1917;Shot(TEXT("21-sambre-final-operation"));break;
        case 26:S.Technology=true;S.TechnologyPage=1;V28ReviewPointer=FVector2D(1050,590);Shot(TEXT("22-german-late-research"));break;
        default:
        {
            bool Aspect=true;TArray<TSharedPtr<FJsonValue>> ArtRows;
            for(const auto& P:V27ImageAudit){ArtRows.Add(MakeShared<FJsonValueObject>(P.Value));Aspect&=P.Value->GetNumberField(TEXT("aspect_relative_error"))<.001;}
            Check(TEXT("all_contained_art_keeps_native_aspect"),Aspect);
            Check(TEXT("focused_screens_requested"),R->Shots>=22);
            R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetArrayField(TEXT("art_draws"),ArtRows);R->Report->SetBoolField(TEXT("success"),R->Passed);
            R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);R->Report->SetNumberField(TEXT("screenshots_requested"),R->Shots);
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))R->Report->SetObjectField(TEXT("render"),Render->GetDiagnostics());
            R->Report->SetStringField(TEXT("scope"),TEXT("Isolated real 4K runtime: campaign/event/loadout transactions, newly cooked art, native grain, gas/flame kits, vertically targeted mortar, rail sector and production seven-second outcome. Authored fixture positions used for close equipment/rail/outro captures; no user-save writes, no sustained performance or subjective listening claim."));
            FString Out;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Out));FFileHelper::SaveStringToFile(Out,*(R->Directory/TEXT("review.json")));
            V32FlagSheet=false;V28ReviewPointer.Reset();V27ImageAuditActive=false;FPlatformMisc::RequestExit(false);return false;
        }
        }
        ++R->Stage;R->Next=Now+Delay;return true;
    }));
}
}
