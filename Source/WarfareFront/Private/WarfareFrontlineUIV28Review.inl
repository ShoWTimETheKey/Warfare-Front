// Isolated UI fixture: real HUD, introduction buttons and preparation state.
// Synthetic pointer/aim/losses are confined to this explicitly flagged process.
bool V28UIContactSheet=false;
void DrawV28UIContactSheet(const FFrontlineCanvas& UI)
{
    UI.Rect(0,0,1920,1080,FLinearColor(.025f,.030f,.035f,1));
    UI.PlainText(TEXT("V28 / CAMPAIGN-WORN CLOTH"),72,39,26,Paper);
    UI.PlainText(TEXT("Synthetic cumulative casualties. Normal HUD size + enlarged cloth on checkerboard."),72,85,18,Muted);
    const float Losses[]={0,30,90,180};auto& A=Artwork();
    for(int32 F=0;F<2;++F)for(int32 I=0;I<4;++I)
    {
        const float X=72+I*457,Y=167+F*281;
        UI.PlainText(FString::Printf(TEXT("%s / %.0f LOSSES"),F?TEXT("GERMAN"):TEXT("BRITISH"),Losses[I]),X,Y-30,17,Muted);
        UI.Flag(A.Flags[F],X,Y,80,53,FVector4f(Losses[I],Losses[I],FMath::Max(0.f,Losses[I]-5),10),0);
        for(int32 R=0;R<8;++R)for(int32 C=0;C<12;++C)
            UI.Rect(X+105+C*20,Y+R*20,20,20,(C+R)%2?FLinearColor(.09f,.105f,.12f,1):FLinearColor(.34f,.37f,.39f,1));
        UI.Flag(A.Flags[F],X+105,Y,240,160,FVector4f(Losses[I],Losses[I],FMath::Max(0.f,Losses[I]-5),10),0);
    }
    UI.PlainText(TEXT("SIX ORIGINAL SUPPORT PAINTINGS / CONTAINED ASPECT / UNAVAILABLE GREYSCALE"),72,741,22,Paper);
    for(int32 I=0;I<6;++I)
    {
        const float X=72+I*303;UI.Rect(X,799,245,151,FLinearColor(.007f,.009f,.008f,1));
        UI.Image(A.SupportPaintings[I],X,799,245,151,FLinearColor::White,0,0,1,1,I%2==1,true);
    }
}
void StartFrontlineV28UIReview(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV28UIReview")))return;
    FString UserDirectory;
    if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDirectory)||UserDirectory.IsEmpty()||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {UE_LOG(LogTemp,Error,TEXT("WF_V28_UI_REVIEW requires UserDir and WFRenderNoSave"));return;}
    struct FRun
    {
        int32 Stage=0,Home=INDEX_NONE;double Next=FPlatformTime::Seconds()+1.5,PreparationAt=0,PreparationSeconds=0;
        float InitialElapsed=0;bool Passed=true,bFrozenSimulation=true;int32 DrawFrames=0;
        FVector2D Trench,Anchor;FString Directory;TArray<TSharedPtr<FJsonValue>> Checks;
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V28/UIReview")/FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*R->Directory,true);V28UIContactSheet=true;V27ImageAuditActive=true;V27ImageAudit.Reset();
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
    {
        auto* PC=Weak.Get();if(!PC)return false;const double Now=FPlatformTime::Seconds();if(Now<R->Next)return true;
        auto* B=FrontlineFor(PC);auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());if(!B||!HUD)return false;
        auto& S=ScreenState(PC);
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;UE_LOG(LogTemp,Display,TEXT("WF_V28_UI %s %s"),*Name,Good?TEXT("PASS"):TEXT("FAIL"));};
        auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(R->Directory/FString(Name),true,false);};
        switch(R->Stage)
        {
        case 0:
            Check(TEXT("volatile_campaign_guard"),B->GetProgressionSaveStatus()==TEXT("独立验证档案"));
            Check(TEXT("flag_mask_and_true_grayscale_material_loaded"),Artwork().Flags[0]&&Artwork().Flags[1]&&Artwork().FlagAbsorption&&Artwork().GrayscalePhotoMaterial);
            {bool All=true;for(auto* T:{Artwork().SupportPaintings[0].Get(),Artwork().SupportPaintings[1].Get(),Artwork().SupportPaintings[2].Get(),Artwork().SupportPaintings[3].Get(),Artwork().SupportPaintings[4].Get(),Artwork().SupportPaintings[5].Get()})All&=T!=nullptr;Check(TEXT("six_support_paintings_loaded"),All);}
            Shot(TEXT("01-slower-blood-original-support-art.png"));break;
        case 1:
            if(B->GetProgressionSaveStatus()!=TEXT("独立验证档案")){FPlatformMisc::RequestExit(false);return false;}
            V28UIContactSheet=false;S.Settings=S.Technology=S.Menu=false;S.Dossier=1917;
            Check(TEXT("isolated_new_campaign"),B->StartNewCampaign(EFrontlineFaction::British));PC->BeginBattle();
            Check(TEXT("real_begin_introduces_infantry_and_new_artillery"),S.UnitIntroduction&&UnitIntroductionCards(B).Num()==2&&B->Phase==EFrontlinePhase::Briefing);
            HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));break;
        case 2:
            Check(TEXT("first_confirm_keeps_support_introduction_before_preparation"),S.UnitIntroduction&&S.UnitIntroductionPage==1&&!S.Preparing);
            Shot(TEXT("02-new-support-introduction.png"));break;
        case 3:
            HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));R->PreparationAt=Now;R->InitialElapsed=B->Elapsed;
            Check(TEXT("last_intro_confirmation_starts_real_preparation"),S.Preparing&&!S.UnitIntroduction&&B->Phase==EFrontlinePhase::Briefing);break;
        case 4:
            Check(TEXT("loading_overlay_has_rendered_without_starting_combat"),S.Preparing&&S.PreparationPainted&&S.PreparationSceneFrames>0&&B->Phase==EFrontlinePhase::Briefing);
            Shot(TEXT("03-world-warming-under-preparation.png"));break;
        case 5:
            if(S.Preparing)
            {
                R->bFrozenSimulation&=B->Phase==EFrontlinePhase::Briefing&&FMath::IsNearlyEqual(B->Elapsed,R->InitialElapsed);
                if(Now-R->PreparationAt<32){R->Next=Now+.10;return true;}
            }
            R->PreparationSeconds=S.PreparationFirstSceneAt>0?Now-S.PreparationFirstSceneAt:0;R->DrawFrames=S.PreparationSceneFrames;
            Check(TEXT("minimum_3_5_seconds_and_60_actual_scene_draws"),!S.Preparing&&B->Phase==EFrontlinePhase::Fighting&&R->PreparationSeconds>=3.5&&R->DrawFrames>=60);
            Check(TEXT("preparation_never_advances_battle_simulation"),R->bFrozenSimulation);
            B->SetActorTickEnabled(false);B->Elapsed=200;B->Casualties[0]=30;B->Casualties[1]=90;PC->ResetCamera();
            R->Home=B->GetHomeTrenchIndex();V28ReviewPointer=FVector2D(1700,200);
            {auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());bool All=V&&V->GetRoster().Num()==3;if(V)for(const auto& E:V->GetRoster())All&=V->GetPortrait(E.Type)!=nullptr;Check(TEXT("separate_three_tank_roster_has_original_portraits"),All);}
            break;
        case 6:
            Check(TEXT("trench_controls_hidden_away_from_trench"),S.HoverTrench==INDEX_NONE);
            {const float X=B->Trenches[R->Home].X;FVector2D Pixel;PC->ProjectWorldLocationToScreen(FVector(X,0,AWarfareFrontlineGround::HeightAt(X,0)+190),Pixel);
                int32 W=0,H=0;PC->GetViewportSize(W,H);const float Scale=FMath::Max(.01f,FMath::Min(W/1920.f,H/1080.f));
                R->Trench=(Pixel-FVector2D((W-1920*Scale)*.5,(H-1080*Scale)*.5))/Scale;R->Anchor=R->Trench-FVector2D(0,95);V28ReviewPointer=R->Trench;}
            break;
        case 7:
            Check(TEXT("real_hud_shows_only_nearby_friendly_trench"),S.HoverTrench==R->Home);
            Shot(TEXT("04-five-infantry-two-tanks-six-supports-and-nearby-orders.png"));break;
        case 8:
            {const auto P=TrenchControlBounds(R->Anchor);V28ReviewPointer=FVector2D(P.X+P.W*.5,P.Y+P.H*.5);}
            break;
        case 9:
            Check(TEXT("moving_pointer_to_portraits_preserves_order_panel"),S.HoverTrench==R->Home);
            V28ReviewPointer=FVector2D(1700,200);HUD->NotifyHitBoxClick(TEXT("Artillery"));
            V28ReviewAim=FVector(B->Trenches[R->Home].X+1400,0,0);break;
        case 10:
            Check(TEXT("leaving_hides_orders_and_real_artillery_button_arms_preview"),S.HoverTrench==INDEX_NONE&&PC->SupportTargetMode==1&&PC->bPreviewValid&&FMath::IsNearlyZero(PC->ArtilleryPreview.Y));
            Shot(TEXT("05-classic-coarse-coverage-reticle.png"));break;
        case 11:
            PC->CancelTargeting();V28ReviewAim.Reset();PC->ToggleSettings();S.SettingsTab=3;V28ReviewPointer=FVector2D(700,480);break;
        case 12:
            {const auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());const auto H=Settings?Settings->GetHDRStatus():FFrontlineHDRStatus();bool All=Settings!=nullptr;
                for(int32 I=0;I<4;++I)All&=HDRCalibrationTip(I,H).Contains(TEXT("nit"))&&HDRCalibrationTip(I,H).Len()>100;
                Check(TEXT("all_four_hdr_controls_explain_nits_and_calibration"),All&&S.Settings&&S.SettingsTab==3);}
            Shot(TEXT("06-hdr-calibration-hover-guidance.png"));break;
        default:
            {
                bool Aspect=true;TArray<TSharedPtr<FJsonValue>> Images;for(const auto& Pair:V27ImageAudit){Images.Add(MakeShared<FJsonValueObject>(Pair.Value));Aspect&=Pair.Value->GetNumberField(TEXT("aspect_relative_error"))<.0001;}
                Check(TEXT("actual_ui_draws_keep_source_image_aspect"),Aspect&&Images.Num()>=18);
                auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Passed);J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("actual_image_draws"),Images);
                J->SetNumberField(TEXT("scene_draw_frames_during_preparation"),R->DrawFrames);J->SetNumberField(TEXT("preparation_scene_seconds"),R->PreparationSeconds);
                J->SetStringField(TEXT("fixture_scope"),TEXT("Real HUD and menu input; synthetic pointer, aim, casualties and post-preparation cooldown clock. Images require visual review. No HDR luminance, GPU convergence, performance or complete battle claim."));
                FString Output;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Output));FFileHelper::SaveStringToFile(Output,*(R->Directory/TEXT("ui-review.json")));
                V28ReviewPointer.Reset();V28ReviewAim.Reset();V27ImageAuditActive=false;
                UE_LOG(LogTemp,Display,TEXT("WF_V28_UI_REVIEW_COMPLETE passed=%d directory=%s"),R->Passed,*R->Directory);FPlatformMisc::RequestExit(false);return false;
            }
        }
        ++R->Stage;R->Next=FPlatformTime::Seconds()+.62;return true;
    }));
}
