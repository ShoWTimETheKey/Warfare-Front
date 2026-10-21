// Isolated V30 acceptance: production display policy, campaign UI and real level.
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"

namespace
{
TSharedRef<FJsonObject> CheckDisplaysV30()
{
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;bool Passed=true;
    auto Check=[&](const TCHAR* Name,bool OK){auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),OK);Checks.Add(MakeShared<FJsonValueObject>(C));Passed&=OK;};
    bool Matrix=true;
    for(float Peak:{400.f,600.f,1000.f,1600.f})for(float White:{80.f,160.f,203.f,280.f,400.f,480.f,800.f,1000.f})
    {
        FFrontlineActiveDisplay D;D.PeakNits=Peak;D.SDRWhiteNits=White;D.bLuminanceKnown=D.bSDRWhiteKnown=true;
        const auto V=FrontlineDisplay::DefaultHDR(D);
        Matrix&=V.PeakNits==int32(Peak) && FMath::IsNearlyEqual(V.PaperWhiteNits,FMath::Min(White,Peak*.8f)) && V.PaperWhiteNits==V.UILuminanceNits && !V.bCalibrated;
    }
    Check(TEXT("32_panel_peak_and_independent_windows_white_combinations"),Matrix);
    FFrontlineActiveDisplay A;A.Id=TEXT("A");A.bValid=A.bHDRStateKnown=A.bLuminanceKnown=A.bSDRWhiteKnown=A.bRefreshKnown=true;
    A.bPersistentIdentity=true;A.PeakNits=600;A.SDRWhiteNits=280;A.RefreshHz=239.76f;
    FFrontlineActiveDisplay Missing;Missing.Id=TEXT("A");Missing.bValid=Missing.bPersistentIdentity=true;
    const auto Same=FrontlineDisplay::PreserveKnownState(Missing,A);Missing.Id=TEXT("B");
    const auto Other=FrontlineDisplay::PreserveKnownState(Missing,A);const auto Fallback=FrontlineDisplay::DefaultHDR(Other);
    Check(TEXT("temporary_same_monitor_failure_preserves_known_white_and_refresh"),Same.SDRWhiteNits==280 && Same.PeakNits==600 && Same.RefreshHz==A.RefreshHz);
    Check(TEXT("other_monitor_never_inherits_previous_brightness_or_refresh"),!Other.bSDRWhiteKnown && !Other.bLuminanceKnown && Other.RefreshHz==0 && Fallback.PaperWhiteNits==203 && Fallback.PeakNits==1000);
    Missing.Id=TEXT("session:B");Missing.bPersistentIdentity=false;
    const auto Session=FrontlineDisplay::PreserveKnownState(Missing,A);
    Check(TEXT("session_monitor_does_not_inherit_persistent_display_profile"),Session.PeakNits==0 && Session.RefreshHz==0 && !Session.bSDRWhiteKnown);
    Missing.bValid=false;Missing.Id=TEXT("B");Missing.Name=TEXT("OTHER");
    const auto Partial=FrontlineDisplay::PreserveKnownState(Missing,A);
    Check(TEXT("partially_identified_other_display_never_returns_previous_monitor"),Partial.Id==TEXT("B") && Partial.RefreshHz==0);
    FFrontlineActiveDisplay Invalid;Invalid.PeakNits=-20;Invalid.SDRWhiteNits=std::numeric_limits<float>::quiet_NaN();Invalid.bSDRWhiteKnown=true;
    const auto Safe=FrontlineDisplay::DefaultHDR(Invalid);
    Check(TEXT("missing_invalid_metadata_has_finite_uncalibrated_fallback"),Safe.PeakNits==1000 && Safe.PaperWhiteNits==203 && !Safe.bCalibrated);
    bool Refresh=true;for(float Hz:{59.94f,60.f,75.f,119.88f,144.f,165.f,239.76f,360.f})
    {
        const auto R=FrontlineDisplay::FrameRateRange(Hz);Refresh&=R.Y==FMath::RoundToInt(Hz);
        Refresh&=FrontlineDisplay::SliderFrameRate(1,Hz)==R.Y && FrontlineDisplay::SliderFrameRate(0,Hz)==R.X;
        int32 Previous=0;for(int32 I=0;I<=100;++I){const int32 F=FrontlineDisplay::SliderFrameRate(I/100.f,Hz);Refresh&=F>=Previous && F<=R.Y;Previous=F;}
    }
    Check(TEXT("eight_current_refresh_modes_monotonic_slider_and_endpoints"),Refresh);
    Check(TEXT("independent_profile_identity"),FrontlineDisplay::HDRSection(A.Id)!=FrontlineDisplay::HDRSection(Other.Id));
    J->SetBoolField(TEXT("success"),Passed);J->SetArrayField(TEXT("checks"),Checks);
    J->SetStringField(TEXT("scope"),TEXT("Synthetic monitor inputs through production policy; not physical tests on these monitors."));
    return J;
}
void StartFrontlineV30Review(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV30Review")))return;
    FString UserDir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty()||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    struct FRun
    {
        int32 Stage=0;bool Passed=true;double Next=FPlatformTime::Seconds()+2,Started=Next,Prepared=0;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V30/Review"));
    IFileManager::Get().MakeDirectory(*R->Directory,true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    V27ImageAuditActive=true;V28ReviewPointer=FVector2D(1810,185);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
    {
        const double Now=FPlatformTime::Seconds();
        auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        if(!B||!HUD)return false;
        if(R->Stage>=15 && R->Stage<=24)TickFrontlineV30MachinegunReview(B,B->GetWorld()->GetDeltaSeconds());
        if(Now<R->Next)return true;
        auto& S=ScreenState(PC);auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
        auto Check=[&](const FString& Name,bool Good){auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(C));R->Passed&=Good;};
        auto Shot=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(R->Directory/(Name+TEXT(".png")),true,false);};
        auto Camera=[&](float X,float Width)
        {
            PC->SetActorTickEnabled(false);const FVector Focus(X,0,150),From=Focus+FVector(0,14000,4000);
            PC->BattleCamera->SetActorLocation(From);PC->BattleCamera->SetActorRotation((Focus-From).Rotation());
            PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;PC->SetViewTarget(PC->BattleCamera);
            if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();
            for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(It->ActorHasTag(TEXT("WF_DistantMatte_V12")))
            {
                const double Centre=.6*X,MatteWidth=FMath::Max(12500.,Width+2*FMath::Abs(X-Centre)+400.);
                const double Height=4200.*MatteWidth/12500.;
                It->SetActorLocation(FVector(Centre,-5200,180.-.4*Height));It->SetActorScale3D(FVector(MatteWidth/100.,1,Height/100.));
            }
        };
        if(Now-R->Started>140)R->Stage=90;
        switch(R->Stage)
        {
        case 0:
        {
            Check(TEXT("volatile_campaign_guard"),B->GetProgressionSaveStatus()==TEXT("独立验证档案"));
            if(!R->Passed){R->Stage=90;break;}
            const auto Displays=CheckDisplaysV30();R->Report->SetObjectField(TEXT("display_matrix"),Displays);R->Passed&=Displays->GetBoolField(TEXT("success"));
            // Inspect the same objects used by the HUD: a successfully loaded older
            // fallback must not make a packaged V30 content omission pass review.
            TArray<TSharedPtr<FJsonValue>> UIAssets;bool Assets=true;
            auto Asset=[&](UObject* Object,const TCHAR* Name,bool bTexture)
            {
                const FString Expected=FString::Printf(TEXT("/Game/Warfare/UI/V30/%s.%s"),Name,Name);
                auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("expected"),Expected);
                Row->SetStringField(TEXT("actual"),Object?Object->GetPathName():TEXT(""));
                bool Ready=Object && Object->GetPathName()==Expected;
                if(bTexture)
                {
                    auto* T=Cast<UTexture2D>(Object);
                    Ready &= T && T->GetSizeX()>0 && T->GetSizeY()>0 && T->GetResource()!=nullptr;
                    Row->SetNumberField(TEXT("width"),T?T->GetSizeX():0);Row->SetNumberField(TEXT("height"),T?T->GetSizeY():0);
                    Row->SetBoolField(TEXT("render_resource_ready"),T && T->GetResource()!=nullptr);
                }
                Row->SetBoolField(TEXT("passed"),Ready);Assets&=Ready;UIAssets.Add(MakeShared<FJsonValueObject>(Row));
            };
            const TCHAR* Maps[]={TEXT("T_Map_Arras"),TEXT("T_Map_Messines"),TEXT("T_Map_Ypres"),TEXT("T_Map_Cambrai"),TEXT("T_Map_Michael"),TEXT("T_Map_Amiens")};
            for(int32 I=0;I<6;++I)Asset(Artwork().MapsV30[I],Maps[I],true);
            Asset(Artwork().CommandersV30[0],TEXT("T_Commanders_British"),true);Asset(Artwork().CommandersV30[1],TEXT("T_Commanders_German"),true);
            Asset(Artwork().EmblemsV30,TEXT("T_Emblems"),true);Asset(Artwork().GermanBackgroundV30,TEXT("T_Background_German"),true);
            Asset(Artwork().EtchedGlyphTexture,TEXT("T_EtchedGlyphRelief"),true);Asset(Artwork().EtchedTitleTexture,TEXT("T_EtchedTitleRelief"),true);
            Asset(Artwork().EtchedMaterial,TEXT("M_EtchedLabel_V30"),false);
            R->Report->SetArrayField(TEXT("required_v30_ui_assets"),UIAssets);Check(TEXT("thirteen_exact_v30_ui_assets_loaded"),Assets);
            if(Settings)
            {
                Settings->ResetHDRCalibration(false);const auto Automatic=Settings->GetHDRStatus();
                Settings->SetHDRPeakNits(400,false);const auto Limited=Settings->GetHDRStatus();
                Settings->ResetHDRCalibration(false);const auto Restored=Settings->GetHDRStatus();
                FFrontlineActiveDisplay ActualDisplay;ActualDisplay.PeakNits=Restored.DisplayPeakNits;ActualDisplay.SDRWhiteNits=Restored.DisplaySDRWhiteNits;
                ActualDisplay.bLuminanceKnown=Restored.bPeakMetadataKnown;ActualDisplay.bSDRWhiteKnown=Restored.bSystemWhiteKnown;
                const auto ExpectedWhite=FrontlineDisplay::DefaultHDR(ActualDisplay);
                Check(TEXT("actual_hdr_peak_change_keeps_automatic_white_within_peak"),Limited.PeakNits==400 && Limited.bPaperWhiteAutomatic && Limited.bUIWhiteAutomatic && FMath::IsFinite(Limited.PaperWhiteNits) && FMath::IsFinite(Limited.UILuminanceNits) && Limited.PaperWhiteNits>=80 && Limited.PaperWhiteNits<=320 && Limited.UILuminanceNits>=80 && Limited.UILuminanceNits<=320);
                Check(TEXT("actual_hdr_reset_restores_display_automatic_white"),Restored.bPaperWhiteAutomatic && Restored.bUIWhiteAutomatic && Restored.PeakNits==Automatic.PeakNits && FMath::IsNearlyEqual(Restored.PaperWhiteNits,Automatic.PaperWhiteNits) && FMath::IsNearlyEqual(Restored.UILuminanceNits,Automatic.UILuminanceNits));
                Check(TEXT("actual_hdr_automatic_white_matches_current_display_policy"),Restored.PeakNits==ExpectedWhite.PeakNits && FMath::IsNearlyEqual(Restored.PaperWhiteNits,ExpectedWhite.PaperWhiteNits) && FMath::IsNearlyEqual(Restored.UILuminanceNits,ExpectedWhite.UILuminanceNits));
                auto HDR=MakeShared<FJsonObject>();HDR->SetNumberField(TEXT("windows_white_nits"),Automatic.DisplaySDRWhiteNits);
                HDR->SetNumberField(TEXT("automatic_paper_white_nits"),Automatic.PaperWhiteNits);HDR->SetNumberField(TEXT("automatic_ui_nits"),Automatic.UILuminanceNits);
                HDR->SetNumberField(TEXT("at_400_peak_paper_white_nits"),Limited.PaperWhiteNits);HDR->SetNumberField(TEXT("at_400_peak_ui_nits"),Limited.UILuminanceNits);
                HDR->SetNumberField(TEXT("restored_paper_white_nits"),Restored.PaperWhiteNits);HDR->SetNumberField(TEXT("restored_ui_nits"),Restored.UILuminanceNits);
                R->Report->SetObjectField(TEXT("actual_hdr_peak_change"),HDR);
            }
            Check(TEXT("quality_2x_and_rr_defaults"),Settings && Settings->GetQuality()==EFrontlineRenderQuality::Quality && Settings->GetFrameGeneration()==EFrontlineFrameGeneration::X2 && Settings->GetRayReconstruction());
            S.Dossier=0;S.Menu=S.Settings=S.Technology=false;Shot(TEXT("01-main"));break;
        }
        case 1:S.Dossier=1918;HUD->NotifyHitBoxClick(TEXT("Difficulty2"));Shot(TEXT("02-difficulty-and-national-arms"));break;
        case 2:
            HUD->NotifyHitBoxClick(TEXT("FactionGerman"));Check(TEXT("faction_starts_survivor_campaign"),B->GetPlayerFaction()==EFrontlineFaction::German && B->GetCampaignDifficulty()==EFrontlineDifficulty::Survivor);
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))Run->CompletedMissions=5;
            B->SelectCampaignMission(0);S.Dossier=1917;Shot(TEXT("03-arras-commanders"));break;
        case 3:case 4:case 5:case 6:case 7:
            B->SelectCampaignMission(R->Stage-2);Shot(FString::Printf(TEXT("%02d-operation-map"),R->Stage+1));break;
        case 8:S.Dossier=0;Shot(TEXT("09-german-main"));break;
        case 9:
            B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran);S.Dossier=1917;
            Check(TEXT("new_campaign_resets_research_and_carries_selected_difficulty"),B->GetCampaignDifficulty()==EFrontlineDifficulty::Veteran && !B->HasTechnology(0));
            PC->BeginBattle();for(int32 I=0;I<5 && S.UnitIntroduction;++I)HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));
            R->Prepared=Now;break;
        case 10:
        {
            if(S.Preparing && Now-R->Prepared<40){R->Next=Now+.15;return true;}
            Check(TEXT("real_level_prepared_without_starting_squads"),B->Phase==EFrontlinePhase::Fighting && B->ActiveSquads(0)==0 && B->ActiveSquads(1)==0 && B->AssetFailure.IsEmpty());
            TArray<TSharedPtr<FJsonValue>> Grounds;bool GroundV30=true;
            for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
            {
                auto* Bound=It->Surface?It->Surface->GetMaterial(0):nullptr;auto* Base=Bound?Bound->GetBaseMaterial():nullptr;
                const bool Correct=Base && Base->GetPathName()==TEXT("/Game/Warfare/Field/V30/Materials/M_GroundMud_V30.M_GroundMud_V30");
                auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("actor"),It->GetPathName());
                Row->SetStringField(TEXT("bound_material"),Bound?Bound->GetPathName():TEXT(""));Row->SetStringField(TEXT("base_material"),Base?Base->GetPathName():TEXT(""));
                Row->SetBoolField(TEXT("passed"),Correct);GroundV30&=Correct;Grounds.Add(MakeShared<FJsonValueObject>(Row));
            }
            R->Report->SetArrayField(TEXT("actual_ground_materials"),Grounds);Check(TEXT("real_ground_surface_uses_v30_material"),!Grounds.IsEmpty() && GroundV30);
            B->SetActorTickEnabled(false);B->Elapsed=200;PC->ResetCamera();Shot(TEXT("10-locked-roster-and-field"));break;
        }
        case 11:
            B->Recruit(EFrontlineRole::Rifle);
            for(AWarfareFrontlineSoldier* U:B->Units)if(U && U->Team==0)
            {
                const float X=B->Trenches[B->GetHomeTrenchIndex()].X+350+U->MemberIndex*64;
                U->SetActorLocation(FVector(X,U->MemberIndex*45,AWarfareFrontlineGround::HeightAt(X,U->MemberIndex*45)+88));
            }
            break;
        case 12:
            if(!B->Units.IsEmpty())if(auto* U=B->Units[0].Get())
            {
                FVector2D Point;PC->ProjectWorldLocationToScreen(U->Feet()+FVector(0,0,88),Point);
                int32 W=0,H=0;PC->GetViewportSize(W,H);const float Scale=FMath::Min(W/1920.f,H/1080.f);
                V28ReviewPointer=(Point-FVector2D((W-1920*Scale)*.5f,(H-1080*Scale)*.5f))/Scale;
            }
            Shot(TEXT("11-squad-strength-hover"));break;
        case 13:
        {
            const auto Tactics=RunFrontlineV30TacticsChecks(B);R->Report->SetObjectField(TEXT("tactics"),Tactics);R->Passed&=Tactics->GetBoolField(TEXT("success"));
            V28ReviewPointer=FVector2D(700,530);PC->ToggleSettings();S.SettingsTab=3;Shot(TEXT("12-hdr-guidance"));
            if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->RunV30AudioChecks(Check);
            if(Settings)R->Report->SetObjectField(TEXT("render"),Settings->GetDiagnostics());break;
        }
        case 14:
        {
            PC->ToggleSettings();S.Settings=false;S.Menu=false;
            for(AWarfareFrontlineSoldier* U:B->Units)if(U)U->Destroy();B->Units.Reset();B->Squads.Reset();
            const float X=B->Trenches[B->GetHomeTrenchIndex()].X+1700;
            BeginFrontlineV30MachinegunReview(B,X);SetFrontlineV30MachinegunReviewStage(B,0);
            Camera(X,3600);V28ReviewPointer=FVector2D(1750,180);
            break;
        }
        case 15:Shot(TEXT("13-machinegun-carry"));break;
        case 16:SetFrontlineV30MachinegunReviewStage(B,1);R->Next=Now+1.0;++R->Stage;return true;
        case 17:Shot(TEXT("14-machinegun-deploy"));R->Next=Now+2.0;++R->Stage;return true;
        case 18:SetFrontlineV30MachinegunReviewStage(B,2);break;
        case 19:Shot(TEXT("15-prone-fire-and-assistants"));break;
        case 20:SetFrontlineV30MachinegunReviewStage(B,3);break;
        case 21:Shot(TEXT("16-machinegun-feed-reload"));R->Next=Now+5;++R->Stage;return true;
        case 22:SetFrontlineV30MachinegunReviewStage(B,4);R->Next=Now+2.5;++R->Stage;return true;
        case 23:
            SetFrontlineV30MachinegunReviewStage(B,5);Camera(B->Trenches[B->GetHomeTrenchIndex()].X,3300);
            R->Next=Now+2.3;++R->Stage;return true;
        case 24:
        {
            Shot(TEXT("17-parapet-braced-gun"));const auto Guns=GetFrontlineV30MachinegunReviewReport(B);
            R->Report->SetObjectField(TEXT("machinegun"),Guns);R->Passed&=Guns->GetBoolField(TEXT("success"));break;
        }
        default:
        {
            bool Aspect=true;TArray<TSharedPtr<FJsonValue>> Images;
            for(const auto& Pair:V27ImageAudit){Images.Add(MakeShared<FJsonValueObject>(Pair.Value));Aspect&=Pair.Value->GetNumberField(TEXT("aspect_relative_error"))<.0001;}
            Check(TEXT("all_recorded_art_draws_preserve_aspect"),Aspect);R->Report->SetArrayField(TEXT("art_draws"),Images);
            R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetArrayField(TEXT("checks"),R->Checks);
            R->Report->SetStringField(TEXT("scope"),TEXT("Actual 4K HUD/maps/level; isolated synthetic pointer and campaign unlock fixture. HDR status and policy, not panel luminance or complete difficulty balance."));
            FString Output;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Output));
            FFileHelper::SaveStringToFile(Output,*(R->Directory/TEXT("review.json")));
            V28ReviewPointer.Reset();V27ImageAuditActive=false;FPlatformMisc::RequestExit(false);return false;
        }
        }
        ++R->Stage;R->Next=Now+.95;return true;
    }));
}
}
