// Narrow, disposable rendered timeline fixtures. Forced ages are never enabled
// by a player preference, console command, or a normal launch.
struct FFrontlineMotionReviewV83 : TSharedFromThis<FFrontlineMotionReviewV83>
{
    TWeakObjectPtr<AWarfareFrontlineController> PC;
    FString Dir;int32 Stage=0,Phase=0;double Began=0,Next=0;bool Good=true,HDRConfigured=false;
    TArray<TSharedPtr<FJsonValue>> Checks,Captures;
    void Check(const FString& Name,bool Pass)
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;}
    void Finish()
    {
        const bool HDRRequested=FParse::Param(FCommandLine::Get(),TEXT("WFUIMotionHDRV83"));
        if(HDRRequested)Check(TEXT("actual_hdr_canvas_composite_was_rendered"),UIMotionReviewV83::HDRCompositeFrames>0);
        for(const auto& C:Captures){auto J=C->AsObject();const bool Exists=IFileManager::Get().FileSize(*J->GetStringField(TEXT("path")))>0;J->SetBoolField(TEXT("exists"),Exists);Good&=Exists;}
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("captures"),Captures);
        J->SetStringField(TEXT("scope"),TEXT("Actual UK/DE production Canvas at 4K, EN/ZH paired fixtures, scalar numerical timelines plus forced start/middle/end frames. Disposable profile; no natural achievement unlock or HDR luminance certification."));
        J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Began);
        J->SetBoolField(TEXT("hdr_requested"),HDRRequested);J->SetNumberField(TEXT("hdr_composite_frames"),UIMotionReviewV83::HDRCompositeFrames);
        FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(Dir/TEXT("review.json")));
        UIMotionReviewV83::TooltipAge=UIMotionReviewV83::BannerAge=UIMotionReviewV83::NumberAge=-1;UIMotionReviewV83::HoldMorale=false;UIMotionReviewV83::Active=false;
        FPlatformMisc::RequestExit(false);
    }
    bool Tick(float)
    {
        const double Now=FPlatformTime::Seconds();if(Now-Began>180){Check(TEXT("bounded_timeline_review"),false);Finish();return false;}if(Now<Next)return true;
        auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;if(!B||!B->LoadedSoldier){Next=Now+.25;return true;}
        if(!HDRConfigured)
        {
            HDRConfigured=true;
            if(FParse::Param(FCommandLine::Get(),TEXT("WFUIMotionHDRV83")))
            {auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());Check(TEXT("temporary_nosave_hdr_request_supported"),R&&R->SetHDREnabled(true,false));Next=Now+2;return true;}
        }
#if WITH_EDITOR
        if(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling())){Next=Now+.25;return true;}
#endif
        if(Stage>=24){Finish();return false;}
        const int32 TeamStyle=Stage/12,Effect=(Stage%12)/3,Moment=Stage%3;
        const TCHAR* Effects[]={TEXT("tooltip"),TEXT("honour"),TEXT("report"),TEXT("morale")};
        auto& S=ScreenState(C);
        if(Phase==0)
        {
            B->AbandonBattleToOperations();FrontlineReviewUIV41::Clear(C);
            FrontlineLocalizationV35::SetLanguage(TeamStyle?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
            Check(TEXT("isolated_profile"),B->IsProgressionVolatile());
            const bool Fixture=FrontlineCampaignV41::PrepareArtReviewOperationV71(B,EFrontlineFaction(TeamStyle),19,true);
            Check(TEXT("production_fixture_" )+FString::FromInt(Stage),Fixture);if(!Fixture){Finish();return false;}
            B->BeginBattle();B->SetActorTickEnabled(false);C->SetActorTickEnabled(false);FrontlineReviewUIV41::Clear(C);
            Check(TEXT("production_units_" )+FString::FromInt(Stage),FrontlineCampaignV41::PopulateArtReviewFieldV73(B));
            FrontlineReviewUIV41::Camera(C,0,0,6500,400,true);S.Menu=false;S.Dossier=1917;S.AchievementToastV81=INDEX_NONE;
            UIMotionReviewV83::TooltipAge=UIMotionReviewV83::BannerAge=UIMotionReviewV83::NumberAge=-1;UIMotionReviewV83::HoldMorale=false;
            V28ReviewPointer=FVector2D(1880,400);
            if(Effect==0){UIMotionReviewV83::TooltipAge=Moment==0?.24:Moment==1?.31:.37;V28ReviewPointer=FVector2D(90,DockTop+90);S.TooltipV83.Reset();}
            if(Effect==1){UIMotionReviewV83::BannerAge=Moment==0?0:Moment==1?.43:.68;S.AchievementToastV81=3;S.AchievementToastAtV81=Now;auto& Badge=Artwork().AchievementBadgesV76[3];if(!Badge)Badge=LoadObject<UTexture2D>(nullptr,*FrontlineAchievementsV76::BadgeObjectPathV82(3));}
            if(Effect==2){UIMotionReviewV83::NumberAge=Moment==0?0:Moment==1?.3:.6;B->Phase=EFrontlinePhase::Victory;B->Casualties[0]=74;B->Casualties[1]=133;B->Morale[0]=72;B->Morale[1]=31;}
            if(Effect==3)
            {
                UIMotionReviewV83::HoldMorale=true;FrontlineFrontsV76::Reset(B);
                for(int32 T=0;T<2;++T)B->Morale[T]=FrontlineMoraleV36::StartingMaximum(B,T);
                FrontlineFrontsV76::Read(B,0);for(int32 T=0;T<2;++T)B->Morale[T]*=T?.42f:.61f;FrontlineFrontsV76::Read(B,0);
                const int32 Steps=Moment==0?0:Moment==1?42:88;for(int32 I=0;I<Steps;++I)FrontlineFrontsV76::Read(B,.01f);
            }
            S.PhotoTransitionV37=S.SmallTransitionV35=false;S.PreviousPageV35=Effect==2?6:3;
            Phase=1;Next=Now+1.1;return true;
        }
        if(Phase==1)
        {
            if(FScreenshotRequest::IsScreenshotRequested()){Next=Now+.15;return true;}
            const FString Path=Dir/TEXT("captures")/FString::Printf(TEXT("%s-%s-%s.png"),TeamStyle?TEXT("german-zh"):TEXT("british-en"),Effects[Effect],Moment==0?TEXT("start"):Moment==1?TEXT("middle"):TEXT("end"));
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("path"),Path);J->SetStringField(TEXT("effect"),Effects[Effect]);J->SetNumberField(TEXT("moment"),Moment);Captures.Add(MakeShared<FJsonValueObject>(J));
            J->SetNumberField(TEXT("hdr_composite_frames_so_far"),UIMotionReviewV83::HDRCompositeFrames);
            FScreenshotRequest::RequestScreenshot(Path,true,false);Phase=2;Next=Now+.4;return true;
        }
        ++Stage;Phase=0;Next=Now+.05;return true;
    }
};
void StartUIMotionReviewV83(AWarfareFrontlineController* C)
{
    auto Run=MakeShared<FFrontlineMotionReviewV83>();Run->PC=C;Run->Began=FPlatformTime::Seconds();Run->Next=Run->Began+4;
    UIMotionReviewV83::Active=true;UIMotionReviewV83::HDRCompositeFrames=0;
    Run->Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/UI-Motion-V83");IFileManager::Get().MakeDirectory(*(Run->Dir/TEXT("captures")),true);
    FrontlineUIMotionV83::RunChecks([&](const FString& N,bool P){Run->Check(N,P);});
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float DT){return Run->Tick(DT);}),0);
}
