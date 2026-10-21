// Focused, isolated V31 UI / combat appearance / score acceptance.
#include "WarfareFrontlineFieldKitV31.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineAudio.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"

namespace
{
void StartFrontlineV31Review(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV31Review")))return;
    FString UserDirectory;
    if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDirectory)||UserDirectory.IsEmpty()||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    struct FRun
    {
        int32 Stage=0,Shots=0,Frames=0;bool Passed=true;double Next=FPlatformTime::Seconds()+2,Started=Next,Prepared=0,FrameSum=0;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
        TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Crew,Casualties;
        TWeakObjectPtr<AWarfareFrontlineImpact> Grenade,Shell;
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V31/Review"));
    IFileManager::Get().MakeDirectory(*R->Directory,true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    V27ImageAuditActive=true;V28ReviewPointer=FVector2D(1750,180);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float Delta)
    {
        const double Now=FPlatformTime::Seconds();auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;if(!B||!HUD)return false;
        if(R->Stage>=20 && R->Stage<=26 && Delta>0 && Delta<.2f){R->FrameSum+=Delta;++R->Frames;}
        if(Now<R->Next)return true;
        auto& S=ScreenState(PC);auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();
        auto Check=[&](const FString& Name,bool Good){auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(C));R->Passed&=Good;};
        auto Shot=[&](const FString& Name){++R->Shots;FScreenshotRequest::RequestScreenshot(R->Directory/(Name+TEXT(".png")),true,false);};
        auto ClearOverlays=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Settings=S.Technology=S.Help=S.UnitIntroduction=S.Preparing=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;PC->CancelTargeting();};
        auto Begin=[&](){PC->SetActorTickEnabled(true);B->SetActorTickEnabled(true);PC->BeginBattle();for(int32 I=0;I<32 && S.UnitIntroduction;++I)HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));R->Prepared=Now;};
        auto Camera=[&](float X,float Width)
        {
            PC->SetActorTickEnabled(false);const FVector Focus(X,0,150),From=Focus+FVector(0,14000,4000);
            PC->BattleCamera->SetActorLocation(From);PC->BattleCamera->SetActorRotation((Focus-From).Rotation());PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;PC->SetViewTarget(PC->BattleCamera);
            if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();
            for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(It->ActorHasTag(TEXT("WF_DistantMatte_V12")))
            {
                const double Centre=.6*X,WidthMatte=FMath::Max(12500.,Width+2*FMath::Abs(X-Centre)+400.),Height=4200.*WidthMatte/12500.;
                It->SetActorLocation(FVector(Centre,-5200,180.-.4*Height));It->SetActorScale3D(FVector(WidthMatte/100.,1,Height/100.));
            }
        };
        auto Ready=[&]()->bool
        {
            if(S.Preparing && Now-R->Prepared<35){R->Next=Now+.15;return false;}
            Check(TEXT("operation_prepared_in_real_level_" )+FString::FromInt(R->Stage),B->Phase==EFrontlinePhase::Fighting && B->AssetFailure.IsEmpty() && !S.Preparing);
            B->SetActorTickEnabled(false);B->Elapsed=90;return true;
        };
        auto SpawnFlash=[&](bool Grenade,const FVector& At)
        {
            auto* FX=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);
            if(FX){FX->ConfigureBlast(Grenade,false);FX->SetActorTickEnabled(false);FX->Tick(0);FX->Tick(Grenade?.040f:.065f);}
            // Frozen only for this authored-light screenshot. Resume the same
            // production effect on the following stage; no gameplay time edit.
            return FX;
        };
        if(Now-R->Started>115){Check(TEXT("focused_run_completed_before_timeout"),false);R->Stage=90;}
        double Delay=1.05;
        switch(R->Stage)
        {
        case 0:
        {
            Check(TEXT("volatile_campaign_guard"),B->GetProgressionSaveStatus()==TEXT("独立验证档案"));
            if(!R->Passed){R->Stage=90;break;}
            bool ArtReady=true;TArray<TSharedPtr<FJsonValue>> Assets;
            auto Asset=[&](UTexture2D* T,const FString& Expected)
            {
                const bool OK=T && T->GetPathName()==Expected && T->GetSizeX()>0 && T->GetSizeY()>0 && T->GetResource();ArtReady&=OK;
                auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("expected"),Expected);A->SetStringField(TEXT("actual"),T?T->GetPathName():TEXT(""));A->SetBoolField(TEXT("passed"),OK);A->SetNumberField(TEXT("width"),T?T->GetSizeX():0);A->SetNumberField(TEXT("height"),T?T->GetSizeY():0);Assets.Add(MakeShared<FJsonValueObject>(A));
            };
            const TCHAR* Names[]={TEXT("T_Map_Arras"),TEXT("T_Map_Messines"),TEXT("T_Map_Ypres"),TEXT("T_Map_Cambrai"),TEXT("T_Map_Michael"),TEXT("T_Map_Amiens")};
            for(int32 I=0;I<6;++I)Asset(Artwork().MapsV30[I],FString::Printf(TEXT("/Game/Warfare/UI/V31/Maps/%s.%s"),Names[I],Names[I]));
            Asset(Artwork().CambraiCounterV31,TEXT("/Game/Warfare/UI/V31/Maps/T_Map_Cambrai_Counterattack.T_Map_Cambrai_Counterattack"));
            Asset(Artwork().CommandSkinV31,TEXT("/Game/Warfare/UI/V31/T_CommandSkin_V31.T_CommandSkin_V31"));Asset(Artwork().ControlFontV31,TEXT("/Game/Warfare/UI/V31/T_ControlGlyphs_V31.T_ControlGlyphs_V31"));
            Check(TEXT("nine_exact_v31_map_console_and_font_resources"),ArtReady);R->Report->SetArrayField(TEXT("ui_assets"),Assets);
            if(Audio)Audio->RunV31MusicChecks(Check);else Check(TEXT("audio_subsystem_present"),false);
            ClearOverlays();S.Dossier=0;Shot(TEXT("01-main-console-theme"));break;
        }
        case 1:PC->ToggleSettings();S.SettingsTab=0;V28ReviewPointer=FVector2D(1740,180);Shot(TEXT("02-settings"));break;
        case 2:PC->ToggleSettings();S.Dossier=1918;Shot(TEXT("03-campaign-selection"));break;
        case 3:HUD->NotifyHitBoxClick(TEXT("FactionBritish"));V28ReviewPointer=FVector2D(213,729);Shot(TEXT("04-british-operation-commander"));break;
        case 4:V28ReviewPointer=FVector2D(748,729);Shot(TEXT("05-opposing-commander-biography"));break;
        case 5:PC->ToggleTechnology();V28ReviewPointer=FVector2D(1650,220);Shot(TEXT("06-british-research"));break;
        case 6:PC->ToggleTechnology();V28ReviewPointer=FVector2D(1750,180);Begin();Delay=.15;break;
        case 7:
            if(!Ready())return true;
            Check(TEXT("british_opening_has_no_initial_squads"),B->ActiveSquads(0)==0 && B->ActiveSquads(1)==0);
            Camera(B->Trenches[B->GetHomeTrenchIndex()].X+1100,5500);Shot(TEXT("07-british-locked-roster-hud"));break;
        case 8:V28ReviewPointer=FVector2D(1844,926);Shot(TEXT("08-settings-icon-hover"));break;
        case 9:HUD->NotifyHitBoxClick(TEXT("Settings"));S.SettingsTab=3;V28ReviewPointer=FVector2D(1740,180);Shot(TEXT("09-field-settings-hdr-layout"));break;
        case 10:
            ClearOverlays();B->Phase=EFrontlinePhase::Victory;B->Elapsed=524;B->Casualties[0]=43;B->Casualties[1]=67;B->ShotsFired=427;B->Morale[0]=72;B->Morale[1]=0;Delay=5.;break;
        case 11:
            Shot(TEXT("10-victory-report"));Check(TEXT("actual_victory_phase_selects_victory_score"),Audio && Audio->GetMusicCueV31()==EFrontlineMusicCueV31::Victory);break;
        case 12:B->Phase=EFrontlinePhase::Defeat;B->Morale[0]=0;B->Morale[1]=54;Delay=5.;break;
        case 13:Shot(TEXT("11-defeat-report"));Check(TEXT("actual_defeat_phase_selects_defeat_score"),Audio && Audio->GetMusicCueV31()==EFrontlineMusicCueV31::Defeat);break;
        case 14:
            ClearOverlays();B->Phase=EFrontlinePhase::Briefing;Check(TEXT("new_german_campaign_created"),B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran));S.Dossier=1917;V28ReviewPointer=FVector2D(748,729);Shot(TEXT("12-german-operation"));break;
        case 15:V28ReviewPointer=FVector2D(1750,180);Begin();Delay=.15;break;
        case 16:
            if(!Ready())return true;
            Check(TEXT("german_opening_right_and_no_initial_squads"),B->GetTeamDirection(0)<0 && B->ActiveSquads(0)==0 && B->ActiveSquads(1)==0);
            Camera(B->Trenches[B->GetHomeTrenchIndex()].X-1100,5500);Shot(TEXT("13-german-locked-roster-hud"));break;
        case 17:
            ClearOverlays();B->Phase=EFrontlinePhase::Briefing;if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))Run->CompletedMissions=5;
            Check(TEXT("isolated_amiens_mission_selected"),B->SelectCampaignMission(5));S.Dossier=1917;Shot(TEXT("14-amiens-painted-deployment-map"));break;
        case 18:Begin();Delay=.15;break;
        case 19:
        {
            if(!Ready())return true;
            B->Elapsed=180;Camera(0,3800);
            for(int32 Team=0;Team<2;++Team)for(int32 I=0;I<6;++I)
            {
                const EFrontlineRole Role=I<2?EFrontlineRole::Rifle:I<4?EFrontlineRole::Mills:EFrontlineRole::Lewis;
                const int32 Id=31000+Team*100+I,Member=I==4?0:(I==5?1:I%3);
                const float X=(Team?-1.f:1.f)*(160.f+180.f*I),Y=float(I%3-1)*66.f;
                const FVector P(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88);
                auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(P,FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));
                if(!U)continue;U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->ReturnToIdle();
                B->Units.Add(U);R->Crew.Add(U);FFrontlineSquad Squad;Squad.Id=Id;Squad.Team=Team;Squad.Role=Role;Squad.Slot=I;Squad.Members.Add(U);B->Squads.Add(Squad);
            }
            V28ReviewPointer=FVector2D(1750,180);Delay=2.;break;
        }
        case 20:
        {
            Shot(TEXT("15-actual-field-kit-variants"));const auto Kit=FrontlineFieldKitV31::Review(B->GetWorld());R->Report->SetObjectField(TEXT("field_kit_before"),Kit);
            Check(TEXT("production_blast_policy_and_live_kit_variation"),Kit->GetBoolField(TEXT("success")) && Kit->GetNumberField(TEXT("kits"))>=12 && Kit->GetNumberField(TEXT("hair_variants"))>=3 && Kit->GetNumberField(TEXT("bayonets"))>=3 && Kit->GetNumberField(TEXT("covered_or_painted_helmets"))>=1);
            int32 Refined=0;for(auto& W:R->Crew)if(auto* U=W.Get())for(int32 I=0;I<U->Soldier->GetNumMaterials();++I)
                if(auto* M=U->Soldier->GetMaterial(I);M && M->GetBaseMaterial()->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V31/Materials/")))++Refined;
            Check(TEXT("actual_soldiers_bind_compiled_v31_materials"),Refined>=24);R->Report->SetNumberField(TEXT("v31_bound_material_slots"),Refined);break;
        }
        case 21:
        {
            const FVector P(-680,450,AWarfareFrontlineGround::HeightAt(-680,450));R->Grenade=SpawnFlash(true,P);
            Check(TEXT("grenade_uses_bounded_18_fragment_profile_and_visible_flash"),R->Grenade.IsValid() && R->Grenade->IsVisualReady() && R->Grenade->IsFlashVisible() && R->Grenade->GetBallisticFragmentCount()==18);
            Shot(TEXT("16-grenade-frozen-40ms-flash"));break;
        }
        case 22:if(auto* FX=R->Grenade.Get())FX->SetActorTickEnabled(true);Delay=.65;break;
        case 23:Shot(TEXT("17-grenade-local-dust"));break;
        case 24:
        {
            const FVector P(540,440,AWarfareFrontlineGround::HeightAt(540,440));R->Shell=SpawnFlash(false,P);
            Check(TEXT("shell_uses_bounded_44_fragment_profile_and_shadowed_flash"),R->Shell.IsValid() && R->Shell->IsVisualReady() && R->Shell->IsFlashVisible() && R->Shell->GetBallisticFragmentCount()==44);
            Shot(TEXT("18-heavy-shell-frozen-65ms-light"));break;
        }
        case 25:
            if(auto* FX=R->Shell.Get())FX->SetActorTickEnabled(true);
            for(int32 I:{0,6})if(R->Crew.IsValidIndex(I))if(auto* U=R->Crew[I].Get())
            {
                const float X=I?575.f:495.f,Y=430;U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));
                U->SetActorTickEnabled(true);R->Casualties.Add(U);U->ReceiveHit(1,1,1-U->Team,FVector(I?1:-1,0,0),EFrontlineDamageKind::Artillery,1);
            }
            Delay=.42;break;
        case 26:
        {
            Shot(TEXT("19-heavy-impact-ragdoll-helmets"));const auto Kit=FrontlineFieldKitV31::Review(B->GetWorld());R->Report->SetObjectField(TEXT("field_kit_after"),Kit);
            Check(TEXT("real_casualties_detach_exact_helmet_meshes"),Kit->GetNumberField(TEXT("detached_helmets"))>=1);
            bool Physical=R->Casualties.Num()==2;TArray<TSharedPtr<FJsonValue>> Deaths;
            for(auto& W:R->Casualties)if(auto* U=W.Get())
            {
                const auto& D=U->GetDeathState();Physical&=!U->IsAlive() && D.bPhysicsStarted && D.PublishedPoseSamples>0 && !D.bNonFinite;
                auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("squad"),U->SquadId);J->SetNumberField(TEXT("published_pose_samples"),D.PublishedPoseSamples);J->SetNumberField(TEXT("maximum_rise_cm"),D.MaximumPelvisRiseCm);J->SetNumberField(TEXT("maximum_speed_cm_s"),D.MaximumBodySpeedCmS);Deaths.Add(MakeShared<FJsonValueObject>(J));
            }
            Check(TEXT("blast_casualties_complete_real_finite_physics_solve"),Physical);R->Report->SetArrayField(TEXT("casualty_physics"),Deaths);Delay=3.2;break;
        }
        case 27:Shot(TEXT("20-settled-aftermath"));break;
        case 28:HUD->NotifyHitBoxClick(TEXT("Help"));Shot(TEXT("21-field-manual"));break;
        case 29:PC->ToggleHelp();PC->ToggleTechnology();Shot(TEXT("22-german-research"));break;
        case 30:PC->ToggleTechnology();PC->ToggleMenu();Shot(TEXT("23-field-menu"));break;
        default:
        {
            TArray<TSharedPtr<FJsonValue>> Images;bool Aspect=true;
            for(const auto& Pair:V27ImageAudit){Images.Add(MakeShared<FJsonValueObject>(Pair.Value));Aspect&=Pair.Value->GetNumberField(TEXT("aspect_relative_error"))<.001;}
            Check(TEXT("recorded_paintings_preserve_source_aspect"),Aspect);Check(TEXT("all_23_focused_views_requested"),R->Shots>=23);
            R->Report->SetArrayField(TEXT("art_draws"),Images);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetBoolField(TEXT("success"),R->Passed);
            R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Started);R->Report->SetNumberField(TEXT("snapshot_frame_samples"),R->Frames);R->Report->SetNumberField(TEXT("snapshot_average_frame_ms"),R->Frames?R->FrameSum*1000/R->Frames:0);
            if(auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))R->Report->SetObjectField(TEXT("render"),Settings->GetDiagnostics());
            R->Report->SetStringField(TEXT("scope"),TEXT("Actual 4K UI and game-level content; isolated campaign unlock/roster/result fixtures. Two explosion flash screenshots deliberately freeze production effects at 40/65 ms, then resume. Music policy/emitter and real outcome cue checks; no subjective listening or sustained performance benchmark."));
            FString Output;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Output));FFileHelper::SaveStringToFile(Output,*(R->Directory/TEXT("review.json")));
            V28ReviewPointer.Reset();V27ImageAuditActive=false;FPlatformMisc::RequestExit(false);return false;
        }
        }
        ++R->Stage;R->Next=Now+Delay;return true;
    }));
}
}
