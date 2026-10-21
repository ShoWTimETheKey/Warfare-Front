// Focused visual comparison only: deterministic synthetic losses, real HUD paths.
bool V27UIContactSheet=false;
void DrawV27UIContactSheet(const FFrontlineCanvas& UI)
{
    UI.Rect(0,0,1920,1080,FLinearColor(.025f,.030f,.035f,1));
    UI.PlainText(TEXT("V27  SOILED CLOTH / PROGRESSIVE TEARS / TRUE ASPECT"),78,42,26,Paper);
    UI.PlainText(TEXT("Normal HUD size at left; 3x detail on checker background exposes actual transparent holes."),78,88,18,Muted);
    const float Losses[]={0,8,30,90};auto& A=Artwork();
    for(int32 F=0;F<2;++F)for(int32 I=0;I<4;++I)
    {
        const float X=78+I*455,Y=165+F*280;
        UI.PlainText(FString::Printf(TEXT("%s  /  %.0f LOSSES"),F?TEXT("GERMAN"):TEXT("BRITISH"),Losses[I]),X,Y-31,17,Muted);
        UI.Flag(A.Flags[F],X,Y,80,53,FVector4f(Losses[I],Losses[I],FMath::Max(0.f,Losses[I]-6),10),0);
        for(int32 R=0;R<8;++R)for(int32 C=0;C<12;++C)
            UI.Rect(X+110+C*20,Y+R*20,20,20,(C+R)%2?FLinearColor(.09f,.105f,.12f,1):FLinearColor(.34f,.37f,.39f,1));
        UI.Flag(A.Flags[F],X+110,Y,240,160,FVector4f(Losses[I],Losses[I],FMath::Max(0.f,Losses[I]-6),10),0);
    }
    UI.PlainText(TEXT("ORIGINAL PHOTOGRAPHS / NO NONUNIFORM SCALING"),78,729,22,Paper);
    UTexture2D* Photos[]={A.BasicPortraits[0],A.SupportPaintings[0],A.TechnologyPaintings[0][0],A.OfficerPortraits[0],A.OfficerPortraits[1]};
    const float Widths[]={142,125,198,198,198},Heights[]={142,111,125,125,125};
    for(int32 I=0;I<5;++I)
    {
        const float X=78+I*355,Y=790;UI.Rect(X,Y,Widths[I],Heights[I],FLinearColor(.006f,.009f,.010f,1));
        UI.Image(Photos[I],X,Y,Widths[I],Heights[I],FLinearColor::White,0,0,1,1,I==2,I>0);
        UI.PlainText(FString::Printf(TEXT("SOURCE %d x %d"),Photos[I]?Photos[I]->GetSizeX():0,Photos[I]?Photos[I]->GetSizeY():0),X,Y+157,16,Muted);
    }
}
void StartFrontlineV27UIReview(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV27UIReview")))return;
    FString UserDirectory;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDirectory)||UserDirectory.IsEmpty()||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {UE_LOG(LogTemp,Error,TEXT("WF_V27_UI_REVIEW requires UserDir and WFRenderNoSave"));return;}
    struct FRun {int32 Stage=0;double Next=FPlatformTime::Seconds()+2;FString Directory;bool Passed=true;TArray<TSharedPtr<FJsonValue>> Checks;};
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V27/UIReview")/FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*R->Directory,true);V27UIContactSheet=true;V27ImageAuditActive=true;V27ImageAudit.Reset();
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
    {
        auto* PC=Weak.Get();if(!PC)return false;if(FPlatformTime::Seconds()<R->Next)return true;
        auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());if(!B)return false;
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(R->Directory/FString(Name),true,false);};
        auto& S=ScreenState(PC);
        switch(R->Stage)
        {
        case 0:
            Check(TEXT("original_flag_and_absorption_textures_loaded"),Artwork().Flags[0]&&Artwork().Flags[1]&&Artwork().FlagAbsorption);
            Shot(TEXT("01-cloth-holes-and-original-photo-aspects.png"));break;
        case 1:
            V27UIContactSheet=false;S.UnitIntroduction=false;S.Dossier=0;
            Check(TEXT("isolated_british_campaign"),B->StartNewCampaign(EFrontlineFaction::British));
            B->BeginBattle();B->SetActorTickEnabled(false);B->Elapsed=1;B->Recruit(EFrontlineRole::Rifle);B->Casualties[0]=30;B->Casualties[1]=90;PC->ResetCamera();break;
        case 2:
            Shot(TEXT("02-normal-console-flags-and-picture-frames.png"));break;
        case 3:
            B->RestartBattle();B->PurchaseTechnology(0);S.Technology=true;break;
        case 4:
            Shot(TEXT("03-british-all-technology-aspects.png"));break;
        case 5:
            Check(TEXT("isolated_german_campaign"),B->StartNewCampaign(EFrontlineFaction::German));B->PurchaseTechnology(0);break;
        case 6:Shot(TEXT("04-german-all-technology-aspects.png"));break;
        case 7:
            S.Technology=false;S.Dossier=1917;PC->BeginBattle();
            Check(TEXT("real_begin_opens_introduction_before_battle"),S.UnitIntroduction&&!S.Preparing&&B->Phase==EFrontlinePhase::Briefing&&
                Cast<AWarfareFrontlineHUD>(PC->GetHUD())&&Cast<AWarfareFrontlineHUD>(PC->GetHUD())->GetPresentationPage()==15);break;
        case 8:Shot(TEXT("05-real-prebattle-unit-introduction.png"));break;
        case 9:
            if(auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD()))HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));
            Check(TEXT("real_intro_confirm_enters_resource_preparation"),!S.UnitIntroduction&&S.Preparing&&B->Phase==EFrontlinePhase::Briefing);break;
        default:
            {
                TArray<TSharedPtr<FJsonValue>> Images;bool Undistorted=true;TSet<FString> TechnologyTextures;
                for(const auto& Pair:V27ImageAudit)
                {
                    Images.Add(MakeShared<FJsonValueObject>(Pair.Value));Undistorted&=Pair.Value->GetNumberField(TEXT("aspect_relative_error"))<.0001;
                    const FString Path=Pair.Value->GetStringField(TEXT("texture"));
                    if(Path.Contains(TEXT("T_Tech_"))||Path.Contains(TEXT("Officer_V24")))TechnologyTextures.Add(Path);
                }
                Check(TEXT("actual_draw_rectangles_preserve_sampled_source_aspect"),Undistorted&&Images.Num()>=35);
                Check(TEXT("both_complete_technology_boards_were_drawn"),TechnologyTextures.Num()>=32);
                auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Passed);J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("actual_image_draws"),Images);
                J->SetStringField(TEXT("flag_shader"),TEXT("/Project/Private/WarfareFlagV27.usf"));J->SetStringField(TEXT("alpha_route"),TEXT("single-pass straight sRGB color plus source coverage; no white repair"));
                J->SetBoolField(TEXT("synthetic_losses_visual_review_required"),true);J->SetBoolField(TEXT("gameplay_or_performance_claim"),false);
                FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Directory/TEXT("ui-review.json")));
                UE_LOG(LogTemp,Display,TEXT("WF_V27_UI_REVIEW_COMPLETE passed=%d draws=%d tech=%d directory=%s"),R->Passed,Images.Num(),TechnologyTextures.Num(),*R->Directory);
                FPlatformMisc::RequestExit(false);return false;
            }
        }
        ++R->Stage;R->Next=FPlatformTime::Seconds()+1.15;return true;
    }));
}
