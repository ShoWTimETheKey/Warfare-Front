// Ten-second isolated presentation fixture, never an ordinary play session.
bool V26UIContactSheet=false;
void DrawV26UIContactSheet(const FFrontlineCanvas& UI)
{
    UI.Rect(0,0,1920,1080,FLinearColor(.018f,.022f,.027f,1));
    UI.PlainText(TEXT("V26  CLOTH ARRIVAL / PHOTO STATES"),88,42,27,Paper);
    UI.PlainText(TEXT("Upper flags: normal HUD size. Lower flags: 3x detail. Synthetic stages."),88,85,18,Muted);
    auto& A=Artwork();const float Losses[]={0,1.8f,5.7f,30},Targets[]={0,6,6,30},Previous[]={0,0,0,24},Ages[]={10,.12f,.60f,10};
    const TCHAR* Labels[]={TEXT("DRY"),TEXT("IMPACT 0.12s"),TEXT("WICKING 0.60s"),TEXT("SETTLED / 30 LOSSES")};
    for(int32 F=0;F<2;++F)for(int32 I=0;I<4;++I)
    {
        const int32 Id=F*4+I;const float X=88+I*440,Y=151+F*263;
        if(!A.FlagClothMaterial)continue;
        if(!A.ReviewFlags[Id])
        {
            auto* M=UMaterialInstanceDynamic::Create(A.FlagClothMaterial,GetTransientPackage());A.ReviewFlags[Id]=M;
            M->SetTextureParameterValue(TEXT("FlagCloth"),A.Flags[F]);
            M->SetScalarParameterValue(TEXT("Losses"),Losses[I]);M->SetScalarParameterValue(TEXT("TargetLosses"),Targets[I]);
            M->SetScalarParameterValue(TEXT("PreviousLosses"),Previous[I]);M->SetScalarParameterValue(TEXT("ImpactAge"),Ages[I]);
        }
        UI.PlainText(Labels[I],X,Y-24,17,Muted);
        UI.MaterialTile(A.ReviewFlags[Id],X,Y,80,53,GWhiteTexture);
        UI.MaterialTile(A.ReviewFlags[Id],X+113,Y,240,159,GWhiteTexture);
    }
    const TCHAR* Names[]={TEXT("INFANTRY / COOLDOWN"),TEXT("SUPPORT / COOLDOWN"),TEXT("TECH / UNRESEARCHED")};
    UTexture2D* Photos[]={A.BasicPortraits[0],A.SupportPaintings[0],A.TechnologyPaintings[0][0]};
    const float Widths[]={142,125,198},Heights[]={142,111,125};
    for(int32 I=0;I<3;++I)
    {
        const float X=88+I*590,Y=759;
        UI.PlainText(Names[I],X,Y-35,18,Paper);
        UI.Image(Photos[I],X,Y,Widths[I],Heights[I],FLinearColor::White,0,0,1,1,true);
        UI.Image(Photos[I],X+Widths[I]+26,Y,Widths[I],Heights[I]);
        UI.PlainText(TEXT("GRAY"),X,Y+151,16,Muted);UI.PlainText(TEXT("READY / RESEARCHED"),X+Widths[I]+26,Y+151,16,Muted);
    }
    UI.PlainText(TEXT("Original photographs. Linear luminance, exact sRGB output, unchanged V25 HDR composition."),88,1004,18,Muted);
}
void StartFrontlineV26UIReview(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV26UIReview")))return;
    FString UserDirectory;
    if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDirectory)||UserDirectory.IsEmpty()||
        !FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {UE_LOG(LogTemp,Error,TEXT("WF_V26_UI_REVIEW requires isolated UserDir and WFRenderNoSave"));return;}
    struct FRun {int32 Stage=0;double Start=FPlatformTime::Seconds(),Next=Start+2;uint64 SettleEligibleFrame=MAX_uint64;bool Passed=true;float Before=0;FString Directory;TArray<TSharedPtr<FJsonValue>> Checks,Snapshots;};
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V26/UIReview")/FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*R->Directory,true);V26UIContactSheet=true;
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
    {
        auto* PC=Weak.Get();if(!PC)return false;
        // Preserve the time between actual state changes if capturing a 4K
        // screenshot stalls a frame; absolute deadlines compress later stages.
        constexpr double After[]={1,1,.18,.22,.3,1.3,1,1.5,1.5};
        if(FPlatformTime::Seconds()<R->Next)return true;
        // The ticker may resume before DrawHUD after a screenshot hitch. Allow
        // the normal production HUD one frame to publish elapsed wall time.
        if(R->Stage==6 && R->SettleEligibleFrame==MAX_uint64){R->SettleEligibleFrame=GFrameCounter;return true;}
        if(R->Stage==6 && R->SettleEligibleFrame==GFrameCounter)return true;
        auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());if(!B)return false;
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(R->Directory/FString(Name),true,false);};
        auto& A=Artwork();
        switch(R->Stage)
        {
        case 0:
            Check(TEXT("two_v26_materials_and_source_flags_loaded"),A.FlagClothMaterial&&A.GrayscalePhotoMaterial&&A.Flags[0]&&A.Flags[1]);
            Check(TEXT("grayscale_material_instances_created_for_all_three_photo_classes"),A.GrayscalePhotos.Contains(A.BasicPortraits[0])&&A.GrayscalePhotos.Contains(A.SupportPaintings[0])&&A.GrayscalePhotos.Contains(A.TechnologyPaintings[0][0]));
            Shot(TEXT("01-contact-sheet-normal-scale-and-detail.png"));break;
        case 1:
            V26UIContactSheet=false;Check(TEXT("isolated_new_campaign_created"),B->StartNewCampaign(EFrontlineFaction::British));
            B->BeginBattle();B->SetActorTickEnabled(false);B->Elapsed=1;B->Casualties[0]=B->Casualties[1]=0;
            Check(TEXT("real_recruitment_starts_photo_cooldowns"),B->Recruit(EFrontlineRole::Rifle));
            ScreenState(PC).Dossier=0;PC->ResetCamera();break;
        case 2:B->Casualties[0]=6;break;
        case 3:Shot(TEXT("02-real-hud-arrival.png"));R->Before=A.FlagLosses[0];break;
        case 4:B->Casualties[0]=8;break;
        case 5:
            Check(TEXT("second_casualty_update_does_not_reset_existing_stain"),A.FlagLosses[0]>=R->Before&&A.FlagPreviousLosses[0]==6&&A.FlagTargetLosses[0]==8);
            Shot(TEXT("03-overlapping-arrival.png"));break;
        case 6:
            Check(TEXT("real_hud_settles_and_opposing_flag_stays_clean"),A.FlagLosses[0]>7.95f&&A.FlagLosses[1]==0);
            Check(TEXT("recruit_and_support_cooldowns_present"),B->RecruitCooldown(EFrontlineRole::Rifle)>0&&B->ArtilleryCooldown()>0);
            Shot(TEXT("04-real-hud-settled-gray-cooldowns.png"));break;
        case 7:
            B->RestartBattle();Check(TEXT("real_research_purchase_for_color_comparison"),B->PurchaseTechnology(0)&&B->HasTechnology(0)&&!B->HasTechnology(2));
            ScreenState(PC).Technology=true;break;
        case 8:Shot(TEXT("05-real-technology-owned-color-unowned-gray.png"));break;
        default:
            {
                auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Passed);J->SetBoolField(TEXT("synthetic_casualty_counts_no_combat_claim"),true);
                J->SetBoolField(TEXT("visual_acceptance_requires_image_review"),true);J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("flag_snapshots"),R->Snapshots);
                J->SetStringField(TEXT("directory"),R->Directory);J->SetStringField(TEXT("flag_material"),A.FlagClothMaterial?A.FlagClothMaterial->GetPathName():TEXT("missing"));
                FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Directory/TEXT("ui-review.json")));
                UE_LOG(LogTemp,Display,TEXT("WF_V26_UI_REVIEW_COMPLETE passed=%d directory=%s"),R->Passed,*R->Directory);
                FPlatformMisc::RequestExit(false);return false;
            }
        }
        auto Sample=MakeShared<FJsonObject>();const double Now=FPlatformTime::Seconds();
        Sample->SetNumberField(TEXT("stage"),R->Stage);Sample->SetNumberField(TEXT("wall_seconds"),Now-R->Start);
        Sample->SetNumberField(TEXT("displayed_losses"),A.FlagLosses[0]);Sample->SetNumberField(TEXT("target_losses"),A.FlagTargetLosses[0]);
        Sample->SetNumberField(TEXT("previous_losses"),A.FlagPreviousLosses[0]);Sample->SetNumberField(TEXT("opposing_displayed_losses"),A.FlagLosses[1]);
        Sample->SetNumberField(TEXT("opposing_target_losses"),A.FlagTargetLosses[1]);Sample->SetNumberField(TEXT("arrival_age_seconds"),Now-A.FlagImpactAt[0]);
        Sample->SetNumberField(TEXT("last_publication_age_seconds"),Now-A.FlagUpdatedAt[0]);Sample->SetNumberField(TEXT("actual_casualties"),B->Casualties[0]);
        R->Snapshots.Add(MakeShared<FJsonValueObject>(Sample));
        UE_LOG(LogTemp,Display,TEXT("WF_V26_UI_FLAG stage=%d wall=%.4f loss=%.6f target=%.1f opposing=%.6f age=%.4f publish_age=%.4f"),
            R->Stage,Now-R->Start,A.FlagLosses[0],A.FlagTargetLosses[0],A.FlagLosses[1],Now-A.FlagImpactAt[0],Now-A.FlagUpdatedAt[0]);
        R->Next=Now+After[R->Stage];++R->Stage;return true;
    }));
}
