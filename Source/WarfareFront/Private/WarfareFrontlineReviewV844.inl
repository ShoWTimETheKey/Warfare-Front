#include "WarfareFrontlineEffectsSafetyV844.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "Widgets/SWindow.h"
#include "Misc/App.h"

namespace
{
bool V844Bool(const TSharedRef<FJsonObject>& J,const TCHAR* Key){bool V=false;J->TryGetBoolField(Key,V);return V;}
double V844Number(const TSharedRef<FJsonObject>& J,const TCHAR* Key){double V=0;J->TryGetNumberField(Key,V);return V;}
bool V844Marker(const FString& File,const FString& RunId,const FString& Nonce,bool Armed)
{
    FString Text;if(!FFileHelper::LoadFileToString(Text,*File))return false;
    TSharedPtr<FJsonObject> J;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J)||!J.IsValid())return false;
    FString Id,Token;if(!J->TryGetStringField(TEXT("run_id"),Id)||!J->TryGetStringField(TEXT("nonce"),Token)||Id!=RunId||Token!=Nonce)return false;
    if(!Armed){bool Dismissed=false;return J->TryGetBoolField(TEXT("overlay_dismissed"),Dismissed)&&Dismissed;}
    FString Expires;FDateTime UTC;if(!J->TryGetStringField(TEXT("expires_utc"),Expires)||!FDateTime::ParseIso8601(*Expires,UTC))return false;
    const double Left=(UTC-FDateTime::UtcNow()).GetTotalSeconds();return Left>=0&&Left<=45;
}
void StartFrontlineReviewV844(AWarfareFrontlineController* Controller)
{
    FString User,Nonce;
    if(!Controller||!FrontlineUltimateArchiveV8215::ReviewUserDir(User)
        ||!FParse::Value(FCommandLine::Get(),TEXT("WFV844TestNonce="),Nonce)||Nonce.Len()!=32)return;
    struct FRun
    {
        FString Dir,Id,Nonce;int32 Stage=0,FXStage=0,UIPage=0,Shots=0;
        bool Native=false,Good=true,Interrupted=false,SawFocusOff=false,SawDrain=false,ResumedWhileDraining=false;
        double Started=FPlatformTime::Seconds(),Next=Started+2,Prepared=0,Warm=0,CaptureReady=0,NextLiveBurst=0;
        uint64 BoundFrame=0;FVector2D Mouse=FVector2D::ZeroVector;bool MouseKnown=false;
        TArray<TSharedPtr<FJsonValue>> Checks,Events;FString PreviousState;
    };
    auto R=MakeShared<FRun>();R->Dir=User;R->Nonce=Nonce;R->Id=FPaths::GetCleanFilename(User);
    R->Native=FParse::Param(FCommandLine::Get(),TEXT("WFV844Native"));
    IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
    {
        auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
        auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Dir/Name));};
        auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;};
        auto Finish=[&](bool Exit=true)
        {
            auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good&&!R->Interrupted);J->SetBoolField(TEXT("human_control_preserved"),R->Interrupted);
            J->SetStringField(TEXT("mode"),R->Native?TEXT("native-presentation"):TEXT("mechanics"));J->SetStringField(TEXT("run_id"),R->Id);
            J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("presentation_events"),R->Events);J->SetNumberField(TEXT("captures_requested"),R->Shots);
            J->SetNumberField(TEXT("wall_seconds"),Now-R->Started);J->SetStringField(TEXT("scope"),TEXT("Disposable UK operation17 production API and cosmetic render fixtures. Not natural gameplay, an independent presentation-rate measurement or a universal reproduction of the reported artifact."));
            if(B)if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))J->SetObjectField(TEXT("final_render_diagnostics"),Render->GetDiagnostics());
            Write(TEXT("checks.json"),J);if(Exit)FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;
        };
        if(!B||!PC||!PC->BattleCamera){if(Now-R->Started<30)return true;Check(TEXT("v844_world_available"),false);return Finish();}
        auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
        if(!Render){Check(TEXT("v844_render_subsystem_available"),false);return Finish();}
        auto* Client=B->GetWorld()->GetGameViewport();const auto* Viewport=Client?Client->Viewport:nullptr;
        const bool Foreground=FApp::HasFocus()&&Viewport&&Viewport->IsForegroundWindow();
        if(R->Native&&Foreground)
        {
            const bool Armed=V844Marker(R->Dir/TEXT("capture-armed.json"),R->Id,R->Nonce,true);
            bool Human=false;
#if PLATFORM_WINDOWS
            for(int32 Key=1;Key<255;++Key)if(GetAsyncKeyState(Key)&0x8000)
            {
                const bool CaptureKey=Key==VK_SNAPSHOT||Key==VK_ESCAPE||Key==VK_LWIN||Key==VK_RWIN||Key==VK_SHIFT||Key==VK_LSHIFT||Key==VK_RSHIFT||Key=='S';
                if(!Armed||!CaptureKey){Human=true;break;}
            }
#endif
            float X=0,Y=0;if(PC->GetMousePosition(X,Y))
            {if(R->MouseKnown&&!Armed&&(FMath::Abs(X-R->Mouse.X)>2.f||FMath::Abs(Y-R->Mouse.Y)>2.f))Human=true;R->Mouse=FVector2D(X,Y);R->MouseKnown=true;}
            if(Human){R->Interrupted=true;auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("preserved"),true);J->SetStringField(TEXT("reason"),TEXT("Unmarked input in this owned game window; automation stopped without closing or controlling it."));Write(TEXT("human-control-preserved.json"),J);return Finish(false);}
        }
        if(Now-R->Started>180){Check(TEXT("v844_bounded_review_completed"),false);return Finish();}
        if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
        auto& S=ScreenState(PC);
        auto Clear=[&](){S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
            S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.Dossier=1917;S.FactionSelectionV82=FFrontlineFactionSelectionV82();
            MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::CloseV836(PC);UltimateArchiveUIV8215::Close(PC);V28ReviewPointer=FVector2D(-10000,-10000);
            PC->CancelTargeting();UGameplayStatics::SetGamePaused(PC,false);FrontlineMapSceneV61::Update(PC,false,FVector2D(.5,.5),1,1,0,0,0);FactionSelectionV82::CoveredWorldV826().Update(PC,B);};
        auto Shot=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(Name+TEXT(".png")),true,false);++R->Shots;};
        const auto Diagnostic=Render->GetDiagnostics();
        const bool Draining=V844Bool(Diagnostic,TEXT("fg_stop_rhi_drain_pending"))||V844Bool(Diagnostic,TEXT("frame_limit_waiting_for_fg_drain"))||V844Number(Diagnostic,TEXT("fg_deferred_output_changes_v844"))>0;
        const bool FG=Foreground&&Render->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::X2
            &&V844Number(Diagnostic,TEXT("frames_presented_per_rendered"))==2&&V844Bool(Diagnostic,TEXT("rr_enabled_api"))
            &&V844Number(Diagnostic,TEXT("observed_dlss_frames"))>0&&V844Number(Diagnostic,TEXT("observed_rr_frames"))>0&&!Draining;
        if(R->Native&&R->Stage>=2)
        {
            const FString State=FString::Printf(TEXT("%d|%d|%d|%.0f"),int32(Foreground),int32(Render->GetEffectiveFrameGeneration()),int32(Draining),V844Number(Diagnostic,TEXT("frames_presented_per_rendered")));
            if(State!=R->PreviousState){R->PreviousState=State;auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("wall_seconds"),Now-R->Started);J->SetBoolField(TEXT("foreground"),Foreground);J->SetObjectField(TEXT("diagnostics"),Diagnostic);R->Events.Add(MakeShared<FJsonValueObject>(J));}
            if(R->Stage>=3&&!Foreground&&Render->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off)
            {if(!R->SawFocusOff){Write(TEXT("focus-off.json"),Diagnostic);R->SawFocusOff=true;}R->SawDrain|=Draining;}
            if(R->Stage>=3&&Draining&&Render->GetEffectiveFrameGeneration()!=EFrontlineFrameGeneration::Off)R->ResumedWhileDraining=true;
        }
        if(R->Native&&R->Stage>=2&&R->Stage<=4&&Now>=R->NextLiveBurst)
        {
            Check(TEXT("v844_native_live_burst_rebind"),FrontlineEffectsSafetyV844::SetupFixture(B,1));
            Check(TEXT("v844_native_live_burst_production_tick"),FrontlineEffectsSafetyV844::SetFixtureLiveV844(B,true));
            R->NextLiveBurst=Now+.65;
        }
        if(R->Stage==0)
        {
            Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);
            if(R->Native&&FParse::Param(FCommandLine::Get(),TEXT("WFV844Borderless")))
            {
                const auto ApplyBorderless=[](){if(auto* G=UGameUserSettings::GetGameUserSettings())
                    {G->SetScreenResolution(FIntPoint(3840,2160));G->SetFullscreenMode(EWindowMode::WindowedFullscreen);G->ApplyResolutionSettings(false);}};
                if(!Render->DeferOutputChangeV844(TEXT("NativeReview"),ApplyBorderless))ApplyBorderless();
            }
            Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);if(!R->Native)Render->SetHDREnabled(false,false);
            Check(TEXT("v844_public_uk_campaign_start"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
            // Explicit earlier-route fixture only; no present combat evidence.
            if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))
            {const uint32 Bypasses=(1u<<3)|(1u<<7)|(1u<<13);P->CompletedMissions=17;P->WonOperationMaskV52=((1u<<17)-1)&~Bypasses;P->BypassedOperationMaskV52=Bypasses;P->DispatchResolvedMaskV52=(1u<<18)-1;}
            Check(TEXT("v844_public_operation17_selected"),B->SelectCampaignMission(17));if(!R->Good)return Finish();S.AcceptedLoadoutMission=INDEX_NONE;
            BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;R->Next=Now+.1;return true;
        }
        if(R->Stage==1)
        {
            if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<65){R->Next=Now+.1;return true;}
            Check(TEXT("v844_actual_operation_prepared"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&B->AssetFailure.IsEmpty()&&S.PreparationError.IsEmpty());if(!R->Good)return Finish();
            Clear();B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
            const FVector Focus(0,80,AWarfareFrontlineGround::HeightAt(0,80)+150);
            PC->BattleCamera->SetActorLocation(Focus+FVector(0,8500,2950));PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());PC->BattleCamera->GetCameraComponent()->OrthoWidth=6000;
            PC->SetViewTarget(PC->BattleCamera);if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0);
            Check(TEXT("v844_world_unpaused_and_normal_camera"),!UGameplayStatics::IsGamePaused(B)&&PC->BattleCamera->GetCameraComponent()->OrthoWidth==6000);
            if(!R->Native)
            {
                FrontlineAchievementsV76::RunAllQuietChecksV8310(B,Check);FrontlineLoadingPageV82::RunCaptionChecksV844(Check);
                Write(TEXT("trench-fire.json"),FrontlineInfantryV45::RunTrenchFireChecksV844(B,Check));
                if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
                {auto J=Fleet->RunSupportArmourBalanceChecksV844();Write(TEXT("armour-support.json"),J);Check(TEXT("v844_armour_support_production_checks"),V844Bool(J,TEXT("success")));}
                else Check(TEXT("v844_actual_fleet_available"),false);
                Write(TEXT("effects-checks.json"),FrontlineEffectsSafetyV844::RunChecks(B,Check));
            }
            else
            {Check(TEXT("v844_native_x2_supported"),Render->GetCapabilities().FrameGenerationModes.Contains(EFrontlineFrameGeneration::X2));Render->SetFrameGeneration(EFrontlineFrameGeneration::X2,false);if(!R->Good)return Finish();}
            Check(TEXT("v844_fx_stage0_setup"),FrontlineEffectsSafetyV844::SetupFixture(B,0));R->BoundFrame=GFrameCounter;R->Warm=0;R->Stage=2;R->Next=Now+.15;return true;
        }
        if(R->Stage==2)
        {
            if(GFrameCounter<=R->BoundFrame)return true;
            if(!R->Native){Write(TEXT("fx-stage-0.json"),FrontlineEffectsSafetyV844::DescribeFixture(B));Shot(TEXT("fx-stage-0"));R->FXStage=1;R->Stage=6;R->Next=Now+.15;return true;}
            if(!FG){R->Warm=0;R->Next=Now+.05;return true;}if(R->Warm==0)R->Warm=Now;if(Now-R->Warm<3){R->Next=Now+.05;return true;}
            int32 W=0,H=0;PC->GetViewportSize(W,H);Check(TEXT("v844_native_actual_4k_foreground_x2_rr"),W==3840&&H==2160&&Foreground&&FG);
            const auto NativeWindow=Client?Client->GetWindow():TSharedPtr<SWindow>();
            const int32 WindowMode=NativeWindow.IsValid()?int32(NativeWindow->GetWindowMode()):INDEX_NONE;
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV844Borderless")))
                Check(TEXT("v844_actual_native_borderless_mode_matches_requested"),WindowMode==int32(EWindowMode::WindowedFullscreen));
            if(!R->Good)return Finish();
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("run_id"),R->Id);J->SetStringField(TEXT("nonce"),R->Nonce);J->SetObjectField(TEXT("render_diagnostics"),Diagnostic);
            J->SetNumberField(TEXT("actual_native_window_mode"),WindowMode);J->SetNumberField(TEXT("actual_viewport_width"),W);J->SetNumberField(TEXT("actual_viewport_height"),H);
            J->SetStringField(TEXT("required_action"),TEXT("Write capture-armed.json with matching run_id/nonce and expires_utc within45s, then use the real system screenshot key and dismiss its overlay. Write capture-attempted.json with overlay_dismissed=true."));
            Write(TEXT("ready-for-capture.json"),J);Write(TEXT("fx-stage-0.json"),FrontlineEffectsSafetyV844::DescribeFixture(B));R->CaptureReady=Now;R->Stage=3;R->Next=Now+.025;return true;
        }
        if(R->Stage==3)
        {
            if(!V844Marker(R->Dir/TEXT("capture-attempted.json"),R->Id,R->Nonce,false)){R->Next=Now+.025;return true;}
            Check(TEXT("v844_system_capture_actual_focus_loss_observed"),R->SawFocusOff);Check(TEXT("v844_x2_not_resumed_during_off_rhi_drain"),!R->ResumedWhileDraining);
            R->Warm=0;R->Stage=4;R->Next=Now+.025;return true;
        }
        if(R->Stage==4)
        {
            if(!FG){R->Warm=0;R->Next=Now+.05;return true;}if(R->Warm==0)R->Warm=Now;if(Now-R->Warm<3){R->Next=Now+.05;return true;}
            Check(TEXT("v844_actual_focus_restored_and_x2_after_draining"),Foreground&&FG&&!R->ResumedWhileDraining);
            Write(TEXT("focus-resumed.json"),Diagnostic);
            Check(TEXT("v844_owned_live_fx_frozen_before_controlled_frame_sequence"),FrontlineEffectsSafetyV844::SetFixtureLiveV844(B,false));
            R->FXStage=1;R->Stage=6;R->Next=Now+.15;return true;
        }
        if(R->Stage==6)
        {
            if(R->FXStage>5)
            {
                Write(TEXT("render-final.json"),Diagnostic);if(R->Native)return Finish();
                FrontlineEffectsSafetyV844::ClearFixture(B);R->Stage=8;R->Next=Now+.1;return true;
            }
            Check(TEXT("v844_fx_setup_stage")+FString::FromInt(R->FXStage),FrontlineEffectsSafetyV844::SetupFixture(B,R->FXStage));
            R->BoundFrame=GFrameCounter;R->Stage=7;R->Next=Now+.15;return true;
        }
        if(R->Stage==7)
        {
            if(GFrameCounter<=R->BoundFrame)return true;
            auto J=FrontlineEffectsSafetyV844::DescribeFixture(B);Write(TEXT("fx-stage-")+FString::FromInt(R->FXStage)+TEXT(".json"),J);
            Shot(TEXT("fx-stage-")+FString::FromInt(R->FXStage));++R->FXStage;R->Stage=6;R->Next=Now+.15;return true;
        }
        if(R->Stage==8)
        {
            if(R->UIPage>=10){TypographyReviewPageV8210=INDEX_NONE;Clear();return Finish();}
            Clear();TypographyReviewPageV8210=INDEX_NONE;S.PreviousPageV35=INDEX_NONE;S.LastPhotoAtV37=Now;
            S.AchievementToastV81=S.MedalToastV8217=INDEX_NONE;
            const bool Chinese=(R->UIPage==2||R->UIPage==3||R->UIPage==5||R->UIPage==7);
            FrontlineLocalizationV35::SetLanguage(Chinese?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
            B->Phase=EFrontlinePhase::Fighting;
            if(R->UIPage<4)
            {S.AchievementsV76=true;HonoursListV82::Reveal(S,28+(R->UIPage%2),true);}
            else if(R->UIPage<6)
            {S.AchievementToastV81=R->UIPage==4?29:28;S.AchievementToastAtV81=Now-.68;}
            else if(R->UIPage<8)
            {
                FrontlineLoadingPageV82::Prewarm(B,S);FrontlineLoadingPageV82::ResetCaptionV844(B,S.PreparationStarted);
                S.PreparationSceneFrames=R->UIPage==7?1:0;TypographyReviewPageV8210=30;
            }
            else
            {
                B->Phase=EFrontlinePhase::Briefing;S.Dossier=1918;FactionSelectionV82::Preload();
                auto& F=S.FactionSelectionV82;F.Visible=true;F.Selected=0;F.LastTick=Now;
                for(int32 I=0;I<4;++I)F.Widths[I]=F.CommitWidths[I]=I==0?1920.f:0.f;
                F.CommitAt=Now-(R->UIPage==8?1.10:1.80);
            }
            R->Stage=9;R->Next=Now+.3;return true;
        }
        if(R->Stage==9)
        {
            static const TCHAR* Names[]={TEXT("ui-stage-0-hearts-special-en"),TEXT("ui-stage-1-quiet-special-en"),TEXT("ui-stage-2-hearts-special-zh"),TEXT("ui-stage-3-quiet-special-zh"),
                TEXT("ui-stage-4-special-banner-en"),TEXT("ui-stage-5-special-banner-zh"),TEXT("ui-stage-6-loading-en"),TEXT("ui-stage-7-loading-zh"),TEXT("ui-stage-8-selector-fade-middle"),TEXT("ui-stage-9-selector-fade-black")};
            if(R->UIPage>=8)
            {
                if(!FactionSelectionV82::PreloadSettled()&&Now-R->Started<100){R->Next=Now+.1;return true;}
                Check(TEXT("v844_selector_actual_art_preloaded_for_fade"),FactionSelectionV82::PreloadSettled());
                S.FactionSelectionV82.CommitAt=Now-(R->UIPage==8?1.10:1.80);
            }
            Shot(Names[R->UIPage]);++R->UIPage;R->Stage=8;R->Next=Now+.15;return true;
        }
        Check(TEXT("v844_known_stage"),false);return Finish();
    }),0.f);
}
}
