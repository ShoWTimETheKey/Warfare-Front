#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineV8217CombatReview.inl"
#include "WarfareFrontlineV8217UiFlowReview.inl"
#include "WarfareFrontlineMedalUIReviewV8217.inl"

namespace
{
void StartFrontlineV8217Review(AWarfareFrontlineController* Controller)
{
    if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8217Review")))return;
    FString User;if(!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
    struct FRun{int32 Stage=0,Shots=0;bool Passed=true,LoadingEN=false,LoadingZH=false;double Started=FPlatformTime::Seconds(),Next=Started+3,Prepared=0;FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;TArray<TWeakObjectPtr<AFrontlineTank>> ArtVehicles;};
    auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
    {
        auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        const double Now=FPlatformTime::Seconds();
        auto Check=[R](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Pass;};
        auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Passed);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Started);J->SetStringField(TEXT("scope"),TEXT("V82.17 changed-path checks in actual isolated Shipping production world; 4K SDR High screenshots. Concurrent older user instances retained. Not a full campaign, performance or HDR certification."));FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("checks.json")));FPlatformMisc::RequestExit(false);return false;};
        if(!B||!HUD||!FrontlineEffectsReviewSafetyV8216::Allowed(B)){Check(TEXT("v8217_isolated_actual_world_and_profile_boundary"),false);return Finish();}
        if(Now-R->Started>150){Check(TEXT("v8217_bounded_review"),false);return Finish();}
        if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
        auto& S=ScreenState(PC);
        auto Clear=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;TypographyReviewPageV8210=INDEX_NONE;MedalWallV8215::View(PC).Wall=false;MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::View(PC).Picker=false;PC->CancelTargeting();};
        auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(FString(Name)+TEXT(".png")),true,false);++R->Shots;};
        double Delay=.6;
        switch(R->Stage)
        {
        case 0:
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);}
            Check(TEXT("v8217_campaign_created_no_player_writes"),B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran));
            if(auto* C=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){C->CompletedMissions=14;C->WonOperationMaskV52=(1u<<14)-1;C->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;C->Research.ResearchPoints=30;C->ArmourResearchMaskV63=511;}
            Check(TEXT("v8217_test_mission_selected"),B->SelectCampaignMission(7));Clear();S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;Delay=.1;break;
        case 1:{
            {auto P=MakeShared<FJsonObject>();P->SetBoolField(TEXT("preparing"),S.Preparing);P->SetBoolField(TEXT("painted"),S.PreparationPainted);P->SetBoolField(TEXT("world_paused"),UGameplayStatics::IsGamePaused(B));P->SetNumberField(TEXT("phase"),int32(B->Phase));P->SetStringField(TEXT("asset_failure"),B->AssetFailure);P->SetStringField(TEXT("preparation_error"),S.PreparationError);P->SetNumberField(TEXT("frames"),S.PreparationSceneFrames);P->SetNumberField(TEXT("scene_seconds"),S.PreparationFirstSceneAt>0?Now-S.PreparationFirstSceneAt:0.);P->SetNumberField(TEXT("textures"),S.PreparationTextures.Num());P->SetNumberField(TEXT("resident"),ResidentOperationTextures(S));P->SetNumberField(TEXT("pending_psos"),PipelineStateCache::GetNumActivePipelinePrecompileTasks());FString J;FJsonSerializer::Serialize(P,TJsonWriterFactory<>::Create(&J));FFileHelper::SaveStringToFile(J,*(R->Dir/TEXT("preparation.json")));}
            if(S.Preparing&&Now-R->Prepared>.35&&!R->LoadingEN){Shot(TEXT("00-loading-en"));R->LoadingEN=true;R->Next=Now+.15;return true;}
            if(S.Preparing&&R->LoadingEN&&!R->LoadingZH){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);Shot(TEXT("00-loading-zh"));R->LoadingZH=true;R->Next=Now+.15;return true;}
            if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<90){R->Next=Now+.1;return true;}
            Check(TEXT("v8217_real_preparation_ready"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&B->AssetFailure.IsEmpty());
            if(!R->Passed)return Finish();B->SetActorTickEnabled(false);Clear();
            const auto Tanks=SetupFrontlineBeuteArtFixtureV8217(B);
            Check(TEXT("v8217_actual_imported_beute_pair"),Tanks.Num()==2);for(auto* Tank:Tanks)R->ArtVehicles.Add(Tank);
            if(Tanks.Num()==2){const FVector Centre=(Tanks[0]->GetActorLocation()+Tanks[1]->GetActorLocation())*.5f;FFrontlineReviewV32::Camera(PC,float(Centre.X),float(Centre.Y),FMath::Max(2800.f,float(FMath::Abs(Tanks[1]->GetActorLocation().X-Tanks[0]->GetActorLocation().X))+1200.f),180);}
            Delay=1.2;break;}
        case 2:Shot(TEXT("17-beute-iron-cross-both-hulls"));break;
        case 3:
            FFrontlineCombatV8217Review::ClearArtFixture(B,R->ArtVehicles);R->ArtVehicles.Empty();
            RunFrontlineCombatChecksV8217(B,Check);
            RunFrontlineUiFlowChecksV8217(B,PC,HUD,Check);
            B->Phase=EFrontlinePhase::Briefing;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=180;
            FrontlineAchievementsV76::RunMedalNotificationChecksV8217(B,Check);
            Check(TEXT("v8217_medal_display_fixture_disposable"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,true,3));
            for(int32 N=0;N<4;++N)MedalWallV8215::PreloadVariants(FrontlineMedalsV8215::NationIndices(EFRRUNationV81(N)),false);
            S.AchievementsV76=true;MedalWallV8215::View(PC).Wall=true;MedalWallV8215::View(PC).Nation=1;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);Delay=1.2;break;
        case 4:if(!FrontlineUIAssetsV8213::PreloadSettled()){R->Next=Now+.1;return true;}MedalUIReviewV8217::Checks(B,PC,HUD,Check);Shot(TEXT("01-medal-wall-german-en"));break;
        case 5:FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);break;
        case 6:Shot(TEXT("02-medal-wall-german-zh"));break;
        case 7:MedalWallV8215::View(PC).Nation=0;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);break;
        case 8:Shot(TEXT("03-medal-wall-british-en"));break;
        case 9:MedalWallV8215::View(PC).Nation=2;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);break;
        case 10:Shot(TEXT("04-medal-wall-france-unearned-zh"));break;
        case 11:Clear();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;MedalMountV8216::View(PC).Picker=true;MedalMountV8216::Chosen(MedalMountV8216::View(PC),B);MedalWallV8215::PreloadVariants(FrontlineMedalsV8215::NationIndices(EFRRUNationV81::German),true);break;
        case 12:Shot(TEXT("05-medal-picker-zh"));break;
        case 13:FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);break;
        case 14:Shot(TEXT("06-medal-picker-en"));break;
        case 15:Clear();B->Phase=EFrontlinePhase::Briefing;TypographyReviewPageV8210=30;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);break;
        case 16:Shot(TEXT("00-loading-final-en"));break;
        case 17:FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);break;
        case 18:Shot(TEXT("00-loading-final-zh"));break;
        case 19:Clear();B->Phase=EFrontlinePhase::Briefing;Check(TEXT("v8217_actual_report_fixture_without_early_unlock"),FrontlineUiFlowV8217::ReviewReportFixture(PC,B,EFrontlineFaction::German));FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);break;
        case 20:Check(TEXT("v8217_unlock_not_present_on_results"),!FrontlineNewUnlockV8215::Pending(B));Shot(TEXT("07-report-before-return-en"));break;
        case 21:HUD->NotifyHitBoxClick(TEXT("CampaignAfterBattle"));Check(TEXT("v8217_actual_report_return_exposes_next_roster"),B->Phase==EFrontlinePhase::Briefing&&FrontlineNewUnlockV8215::Pending(B));Delay=2.;break;
        case 22:if(S.PhotoTransitionV37){R->Next=Now+.1;return true;}Shot(TEXT("08-unlock-after-return-en"));break;
        case 23:Clear();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);Check(TEXT("v8217_banner_before_stamp_fixture"),FrontlineUiFlowV8217::ReviewBannerFixture(PC,B,false,.36));Shot(TEXT("09-achievement-before-stamp"));break;
        case 24:FrontlineUiFlowV8217::ReviewBannerFixture(PC,B,false,.45);Shot(TEXT("10-achievement-stamp-approach"));break;
        case 25:FrontlineUiFlowV8217::ReviewBannerFixture(PC,B,false,.51);Shot(TEXT("11-achievement-stamp-contact"));break;
        case 26:FrontlineUiFlowV8217::ReviewBannerFixture(PC,B,false,.725);Shot(TEXT("12-achievement-one-shine"));break;
        case 27:FrontlineUiFlowV8217::ReviewBannerFixture(PC,B,false,.95);Shot(TEXT("13-achievement-stamp-settled"));break;
        case 28:FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);Check(TEXT("v8217_medal_banner_from_saved_real_award"),FrontlineUiFlowV8217::ReviewBannerFixture(PC,B,true,.45));Shot(TEXT("14-medal-award-stamp"));break;
        case 29:FrontlineUiFlowV8217::ReviewBannerFixture(PC,B,true,.725);Shot(TEXT("15-medal-award-one-shine"));break;
        case 30:FrontlineUiFlowV8217::ReviewBannerFixture(PC,B,true,.95);Shot(TEXT("16-medal-award-settled"));break;
        default:Clear();Check(TEXT("v8217_all_textures_loaded_without_negative_cache"),FrontlineUIAssetsV8213::PreloadReady());return Finish();
        }
        ++R->Stage;R->Next=Now+Delay;return true;
    }),0.f);
}
}
