// Explicit isolated presentation review: synthetic casualty counts, no player saves.
int32 V24FlagReviewStage=-1;
void DrawV24FlagContactSheet(const FFrontlineCanvas& UI)
{
    UI.Rect(0,0,1920,1080,FLinearColor(.018f,.022f,.027f,1));
    UI.PlainText(TEXT("V24  CLOTH / CASUALTY PROGRESSION"),90,45,30,Paper);
    auto& A=Artwork();const float Losses[]={0,15,60,180};
    for(int32 F=0;F<2;++F)for(int32 I=0;I<4;++I)
    {
        const int32 Id=F*4+I;const float X=90+I*460,Y=125+F*292;
        if(!A.FlagClothMaterial)continue;
        if(!A.ReviewFlags[Id])
        {
            A.ReviewFlags[Id]=UMaterialInstanceDynamic::Create(A.FlagClothMaterial,GetTransientPackage());
            A.ReviewFlags[Id]->SetTextureParameterValue(TEXT("FlagCloth"),A.Flags[F]);
            A.ReviewFlags[Id]->SetScalarParameterValue(TEXT("Losses"),Losses[I]);
        }
        UI.MaterialTile(A.ReviewFlags[Id],X,Y,400,265,GWhiteTexture);
        UI.PlainText(FString::Printf(TEXT("%.0f"),Losses[I]),X,Y-29,20,Paper);
    }
    for(int32 F=0;F<2;++F)UI.Image(A.OfficerPortraits[F],735+F*244,725,224,224);
    UI.PlainText(TEXT("Synthetic presentation stages; production flags use each army's actual fatalities."),90,1010,21,Muted);
}
void StartFrontlineV24UIReview(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV24UIReview")))return;
    FString Isolated;
    if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Isolated)||Isolated.IsEmpty())return;
    V24FlagReviewStage=0;
    struct FRun {int32 Stage=0;double Next=FPlatformTime::Seconds()+4;bool Passed=true;FString Directory;TArray<TSharedPtr<FJsonValue>> Checks;};
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V24/UIReview"));
    IFileManager::Get().MakeDirectory(*R->Directory,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
    {
        auto* PC=Weak.Get();if(!PC)return false;
        if(FPlatformTime::Seconds()<R->Next)return true;R->Next=FPlatformTime::Seconds()+3;
        auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());if(!B)return false;
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(R->Directory/FString(Name),true,false);};
        if(R->Stage==0)
        {
            Check(TEXT("new_portraits_and_cloth_material_loaded"),Artwork().OfficerPortraits[0]&&Artwork().OfficerPortraits[1]&&Artwork().FlagClothMaterial);
            Shot(TEXT("cloth-and-officers.png"));
        }
        else if(R->Stage==1)
        {
            B->StartNewCampaign(EFrontlineFaction::British);B->BeginBattle();B->Casualties[0]=60;B->Casualties[1]=0;
            ScreenState(PC).Dossier=0;PC->ResetCamera();V24FlagReviewStage=1;
        }
        else if(R->Stage==2)
        {
            Shot(TEXT("british-own-losses.png"));
            Check(TEXT("own_army_losses_stain_only_own_flag"),Artwork().FlagLosses[0]>55&&Artwork().FlagLosses[1]==0);
        }
        else if(R->Stage==3)
        {
            B->Phase=EFrontlinePhase::Briefing;B->StartNewCampaign(EFrontlineFaction::German);B->BeginBattle();
            B->Casualties[0]=0;B->Casualties[1]=120;PC->ResetCamera();
        }
        else if(R->Stage==4)
        {
            Shot(TEXT("german-opposing-losses.png"));
            Check(TEXT("faction_swap_and_new_battle_reset"),Artwork().FlagLosses[0]==0&&Artwork().FlagLosses[1]>110);
        }
        else if(R->Stage==5){B->Casualties[0]=75;B->Phase=EFrontlinePhase::Victory;}
        else if(R->Stage==6)
        {
            Shot(TEXT("field-report-stained-flags.png"));
            Check(TEXT("last_losses_continue_wicking_on_final_report"),Artwork().FlagLosses[0]>70);
        }
        else
        {
            auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Passed);J->SetBoolField(TEXT("synthetic_casualty_presentation"),true);J->SetArrayField(TEXT("checks"),R->Checks);
            FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Directory/TEXT("ui-review.json")));
            UKismetSystemLibrary::QuitGame(PC,PC,EQuitPreference::Quit,false);return false;
        }
        ++R->Stage;return true;
    }));
}
