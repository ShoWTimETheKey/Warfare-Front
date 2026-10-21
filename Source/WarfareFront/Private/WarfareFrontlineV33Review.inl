#include "WarfareFrontlineGameplayV33.h"
namespace
{
void StartFrontlineV33Review(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV33Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    FString UserDir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
    struct FRun{int32 Stage=0,Shots=0;bool Passed=true;double Next=FPlatformTime::Seconds()+3,Start=Next,Prepared=0;FString Directory;
        TWeakObjectPtr<AWarfareFrontlineSoldier> FlameTarget;
        TArray<TSharedPtr<FJsonValue>> Checks;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();};
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V33/Review"));
    IFileManager::Get().MakeDirectory(*R->Directory,true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    V27ImageAuditActive=true;V28ReviewPointer=FVector2D(1740,180);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
    {
        const double Now=FPlatformTime::Seconds();auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;if(!B||!HUD)return false;if(Now<R->Next)return true;
        auto& S=ScreenState(PC);
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const TCHAR* Name){++R->Shots;FScreenshotRequest::RequestScreenshot(R->Directory/(FString(Name)+TEXT(".png")),true,false);};
        auto Clear=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.UnitIntroduction=S.Preparing=S.Deployment=S.HistoricalDispatch=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;PC->CancelTargeting();};
        auto Select=[&](EFrontlineFaction F,int32 Mission)
        {
            Clear();B->AbandonBattleToOperations();B->SetActorTickEnabled(true);B->StartNewCampaign(F,EFrontlineDifficulty::Veteran);
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){Run->CompletedMissions=9;Run->Research.TechnologyMask=(1u<<24)-1;Run->Research.ResearchPoints=25;}
            Check(TEXT("selected_operation_")+FString::FromInt(Mission)+FString::FromInt(int32(F)),B->SelectCampaignMission(Mission));
            S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;PC->ResetCamera();
        };
        auto Begin=[&]()
        {
            PC->SetActorTickEnabled(true);B->SetActorTickEnabled(true);
            for(int32 I=0;I<32&&!S.Preparing;++I){PC->BeginBattle();if(S.HistoricalDispatch)HUD->NotifyHitBoxClick(TEXT("V32Acknowledge"));if(S.Deployment)HUD->NotifyHitBoxClick(TEXT("V32DeploymentConfirm"));if(S.UnitIntroduction)HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));}
            R->Prepared=Now;
        };
        auto Ready=[&]()
        {
            // ClearPause is completed by the following world tick. A core
            // ticker must not submit recruitment during that pending frame.
            if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&Now-R->Prepared<35){R->Next=Now+.15;return false;}
            Check(TEXT("actual_operation_prepared_")+FString::FromInt(R->Stage),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty());
            B->SetActorTickEnabled(false);B->Elapsed=180;return true;
        };
        if(Now-R->Start>150){Check(TEXT("bounded_review_timeout"),false);R->Stage=99;}
        double Delay=1.1;
        switch(R->Stage)
        {
        case 0:
        {
            Check(TEXT("isolated_campaign"),B->GetProgressionSaveStatus()==TEXT("独立验证档案"));
            if(!R->Passed){R->Stage=99;break;}
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            FrontlineGameplayV33::RunFocusedChecks(B,Check);B->AbandonBattleToOperations();Clear();S.Dossier=0;
            Check(TEXT("v33_generated_skin_and_splash_loaded"),Artwork().CommandSkinV31&&Artwork().FlagSplashesV32&&Artwork().CommandSkinV31->GetPathName().Contains(TEXT("V33"))&&Artwork().FlagSplashesV32->GetSizeX()==1254);
            break;
        }
        case 1:Shot(TEXT("01-main-menu"));break;
        case 2:PC->ToggleSettings();S.SettingsTab=0;Shot(TEXT("02-settings"));break;
        case 3:Select(EFrontlineFaction::British,7);Shot(TEXT("03-operations-room"));break;
        case 4:S.Technology=true;S.TechnologyPage=1;V28ReviewPointer=FVector2D(482,377);Shot(TEXT("04-technology-tooltip"));break;
        case 5:Clear();Begin();Delay=.15;break;
        case 6:if(!Ready())return true;FFrontlineReviewV32::Camera(PC,0,0,6200);V28ReviewPointer=FVector2D(1740,180);Shot(TEXT("05-british-console"));break;
        case 7:PC->ToggleMenu();Shot(TEXT("06-battle-menu-abandon"));break;
        case 8:
        {
            const auto Before=*B->GetCurrentCampaign();HUD->NotifyHitBoxClick(TEXT("AbandonBattle"));
            Check(TEXT("menu_abandon_returns_operations_preserves_run"),HUD->GetPresentationPage()==1&&!S.Menu&&!UGameplayStatics::IsGamePaused(B)&&B->GetCurrentCampaign()->Id==Before.Id&&B->GetCurrentCampaign()->Research.TechnologyMask==Before.Research.TechnologyMask);
            Shot(TEXT("07-abandoned-operations-room"));Begin();Delay=.15;break;
        }
        case 9:
        {
            if(!Ready())return true;
            if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
            {
                const auto Result=Vehicles->RunFocusedChecks();
                R->Report->SetObjectField(TEXT("vehicles"),Result);
                for(const auto& Row:Result->GetArrayField(TEXT("checks")))Check(Row->AsObject()->GetStringField(TEXT("name")),Row->AsObject()->GetBoolField(TEXT("passed")));
            }
            else Check(TEXT("vehicle_subsystem_available"),false);
            FrontlineFieldV32::RunFocusedChecksV33(B,Check);
            for(EFrontlineRole Role:{EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Flamethrower,EFrontlineRole::Pioneer})FFrontlineReviewV32::Spawn(B,Role,0,1);
            FFrontlineReviewV32::Spawn(B,EFrontlineRole::Flamethrower,1,3);
            int32 N=0;for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsAlive())
            {const float X=(N%7-3)*230.f,Y=(N/7-1)*400.f;U->bFieldEntered=true;U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));U->SetActorTickEnabled(false);U->ReturnToIdle();++N;}
            FFrontlineSmokeZone Z;Z.bGas=true;Z.Centre=FVector::ZeroVector;Z.Radius=2500;Z.StartedAt=B->Elapsed;Z.ExpiresAt=B->Elapsed+20;B->SmokeZones.Add(Z);
            for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsAlive())FrontlineFieldV32::TickUnit(U,.2f);
            FFrontlineReviewV32::Camera(PC,0,0,2450,130);Delay=1.6;break;
        }
        case 10:Shot(TEXT("08-mask-and-equipment"));R->Report->SetObjectField(TEXT("equipment"),FrontlineFieldV32::Review(B));break;
        case 11:
        {
            B->SmokeZones.Empty();AWarfareFrontlineSoldier* F=nullptr,*T=nullptr;
            for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsAlive()){if(U->Team==0&&FrontlineFieldV32::IsFlameOperator(U))F=U;else if(U->Team==1&&FrontlineFieldV32::IsFlameOperator(U))T=U;}
            if(F&&T){R->FlameTarget=T;F->SetActorLocation(FVector(-330,160,AWarfareFrontlineGround::HeightAt(-330,160)+88));T->SetActorLocation(FVector(180,160,AWarfareFrontlineGround::HeightAt(180,160)+88));Check(TEXT("production_flame_discharge"),FrontlineFieldV32::FlameDischarge(F,T));F->SetActorTickEnabled(true);T->SetActorTickEnabled(true);B->SetActorTickEnabled(true);}
            else Check(TEXT("flame_target_fixture"),false);
            FFrontlineReviewV32::Camera(PC,0,0,2450,130);Delay=.45;break;
        }
        case 12:Shot(TEXT("09-flame-discharge"));R->Report->SetObjectField(TEXT("fire"),FrontlineFieldV32::Review(B));Delay=.55;break;
        case 13:
            // The opening jet uses normal hit probability. Guarantee the separate
            // casualty art fixture through the real fatal-hit entry, without
            // changing gameplay hit chances or replacing its death/rupture path.
            if(auto* T=R->FlameTarget.Get())if(T->IsActiveCombatant())T->ReceiveHit(1,.6f,0,FVector(1,0,0),EFrontlineDamageKind::Fire);
            R->Report->SetStringField(TEXT("burning_visual_fixture"),TEXT("Production fatal Fire hit, followed by authored fall and delayed fuel rupture; the preceding jet retains normal hit chance."));
            Delay=.27;break;
        case 14:
            Shot(TEXT("10-burning-casualty"));Check(TEXT("visible_fixture_enters_burning_death"),FrontlineFieldV32::Review(B)->GetNumberField(TEXT("burning_casualties_v33"))>0);Delay=.65;break;
        case 15:
            Shot(TEXT("11-fuel-secondary-burst"));R->Report->SetObjectField(TEXT("fuel"),FrontlineFieldV32::Review(B));
            Check(TEXT("visible_fixture_reaches_delayed_fuel_burst"),FrontlineFieldV32::Review(B)->GetNumberField(TEXT("fuel_bursts_v33"))>0);break;
        case 16:V32FlagSheet=true;V32FlagAge=3;Shot(TEXT("12-flag-absorbed"));break;
        case 17:V32FlagAge=.12f;Shot(TEXT("13-flag-arrival"));break;
        case 18:V32FlagSheet=false;Select(EFrontlineFaction::German,8);Begin();Delay=.15;break;
        case 19:
        {
            if(!Ready())return true;B->Elapsed=60;FrontlineFieldV32::Tick(B,.1f);
            auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());
            if(V){Check(TEXT("visual_fixture_a7v_recruited"),V->Recruit(EFrontlineVehicle::A7V));Check(TEXT("visual_fixture_beute_recruited"),V->Recruit(EFrontlineVehicle::BeuteMarkIV));Check(TEXT("visual_fixture_mark_iv_recruited"),V->Recruit(EFrontlineVehicle::MarkIVMale,1));Check(TEXT("visual_fixture_whippet_recruited"),V->Recruit(EFrontlineVehicle::Whippet,1));
                int32 N=0;for(AFrontlineTank* T:V->GetVehicles())if(T){const float X=T->IsRailConvoyV32()?0:(N++-2)*1300;T->SetActorLocation(FVector(X,T->IsRailConvoyV32()?-1130:320,28));Check(TEXT("authored_vehicle_assets_loaded_")+FString::FromInt(T->Serial),T->AreAssetsReady());}}
            FFrontlineReviewV32::Camera(PC,0,-100,6500,200);Delay=1.6;break;
        }
        case 20:Shot(TEXT("14-german-console-vehicles"));break;
        case 21:FFrontlineReviewV32::Camera(PC,0,-1130,3600,180);Shot(TEXT("15-armoured-rail"));break;
        case 22:Clear();B->AbandonBattleToOperations();S.Dossier=1917;S.Technology=true;S.TechnologyPage=0;V28ReviewPointer=FVector2D(1050,590);Shot(TEXT("16-german-research"));break;
        default:
        {
            bool Aspect=true;TArray<TSharedPtr<FJsonValue>> ArtRows;for(const auto& P:V27ImageAudit){ArtRows.Add(MakeShared<FJsonValueObject>(P.Value));Aspect&=P.Value->GetNumberField(TEXT("aspect_relative_error"))<.001;}
            Check(TEXT("all_art_maintains_native_aspect"),Aspect);Check(TEXT("affected_screens_captured"),R->Shots>=16);
            R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetArrayField(TEXT("art_draws"),ArtRows);R->Report->SetBoolField(TEXT("success"),R->Passed);
            R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);R->Report->SetNumberField(TEXT("screenshots_requested"),R->Shots);
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))R->Report->SetObjectField(TEXT("render"),Render->GetDiagnostics());
            R->Report->SetStringField(TEXT("scope"),TEXT("Isolated real runtime V33 focused gameplay checks, affected UI, authored masks, fire, vehicles and flag compositing. Art fixture positions. Separate visual review and performance capture required."));
            FString Out;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Out));FFileHelper::SaveStringToFile(Out,*(R->Directory/TEXT("review.json")));
            V32FlagSheet=false;V28ReviewPointer.Reset();V27ImageAuditActive=false;FPlatformMisc::RequestExit(false);return false;
        }
        }
        ++R->Stage;R->Next=Now+Delay;return true;
    }));
}
}
