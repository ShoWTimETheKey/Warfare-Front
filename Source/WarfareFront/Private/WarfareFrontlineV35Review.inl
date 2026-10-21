#include "WarfareFrontlineCombatV35.h"
namespace
{
void StartFrontlineV35Review(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV35Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    FString UserDir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
    struct FRun
    {
        int32 Stage=0,Shots=0,GpuSamples=0;bool Passed=true;
        double Next=FPlatformTime::Seconds()+3,Start=Next,Prepared=0,GpuStart=0,GpuTotal=0,FrameTotal=0,LastFrame=0;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V35/Review"));
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
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){Run->CompletedMissions=Mission;Run->Research.TechnologyMask=0;Run->Research.ResearchPoints=25;}
            Check(TEXT("operation_selected_")+FString::FromInt(Mission)+FString::FromInt(int32(F)),B->SelectCampaignMission(Mission));
            S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;PC->ResetCamera();
        };
        auto Begin=[&]()
        {
            PC->SetActorTickEnabled(true);B->SetActorTickEnabled(true);
            for(int32 I=0;I<32&&!S.Preparing;++I){PC->BeginBattle();if(S.HistoricalDispatch)HUD->NotifyHitBoxClick(TEXT("V32Acknowledge"));if(S.Deployment)HUD->NotifyHitBoxClick(TEXT("V32DeploymentConfirm"));if(S.UnitIntroduction)HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));}
            R->Prepared=Now;
        };
        if(Now-R->Start>150){Check(TEXT("bounded_review_timeout"),false);R->Stage=99;}
        double Delay=.9;
        switch(R->Stage)
        {
        case 0:
            Check(TEXT("isolated_campaign"),B->GetProgressionSaveStatus()==TEXT("独立验证档案"));
            Check(TEXT("english_default_or_explicit_review_locale"),FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English);
            if(!R->Passed){R->Stage=99;break;}
            B->RunV35MovementChecks(Check);RunV35CombatChecks(B,Check);
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            Check(TEXT("whole_message_dynamic_format_english"),FrontlineLocalizationV35::Text(TEXT("需要：刘易斯开路"))==TEXT("Requires: The Lewis opens the way"));
            Check(TEXT("numeric_first_counter_localized"),FrontlineLocalizationV35::Text(TEXT("25  研究点"))==TEXT("25  research points"));
            Check(TEXT("name_first_support_order_localized"),FrontlineLocalizationV35::Text(TEXT("18 磅野炮齐射 · 选择覆盖地段"))==TEXT("18-pounder salvo · Choose the area to cover"));
            Check(TEXT("structured_status_join_localized"),FrontlineLocalizationV35::Text(TEXT("刘易斯开路 · 机枪支援"))==TEXT("The Lewis opens the way · Machine-gun support"));
            Check(TEXT("painted_lock_cutout_loaded"),Artwork().LockCutoutV35!=nullptr);
            for(int32 I=0;I<4;++I)Check(TEXT("historical_transition_photo_")+FString::FromInt(I),Artwork().ArchivePhotosV35[I]&&Artwork().ArchivePhotosV35[I]->GetSizeX()>=1500);
            B->AbandonBattleToOperations();Clear();S.Dossier=0;break;
        case 1:Shot(TEXT("01-main-menu-en"));break;
        case 2:PC->ToggleSettings();S.SettingsTab=0;break;
        case 3:Shot(TEXT("02-settings-en"));break;
        case 4:HUD->NotifyHitBoxClick(TEXT("LanguageChinese"));Check(TEXT("language_toggle_immediate_chinese"),FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese);break;
        case 5:Shot(TEXT("03-settings-zh"));break;
        case 6:FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);Select(EFrontlineFaction::British,7);break;
        case 7:Shot(TEXT("04-operations-en"));break;
        case 8:S.Technology=true;S.TechnologyPage=0;V28ReviewPointer=FVector2D(482,377);break;
        case 9:Shot(TEXT("05-technology-en"));break;
        case 10:Select(EFrontlineFaction::British,0);Begin();Delay=.15;break;
        case 11:
            if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&Now-R->Prepared<35){R->Next=Now+.15;return true;}
            Check(TEXT("actual_operation_prepared"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty());
            B->SetActorTickEnabled(false);B->Elapsed=65;FFrontlineReviewV32::Camera(PC,0,0,6200);V28ReviewPointer=FVector2D(960,34);break;
        case 12:Shot(TEXT("06-clock-briefing-locked-roster"));break;
        case 13:PC->SupportTargetMode=SupportTargetMode(EFrontlineSupport::Artillery);PC->ArtilleryPreview=FVector::ZeroVector;PC->bPreviewValid=true;V28ReviewPointer=FVector2D(1200,530);break;
        case 14:Shot(TEXT("07-artillery-sight"));break;
        case 15:Clear();Check(TEXT("movement_visual_fixture"),B->SetupV35MovementVisualFixture());FFrontlineReviewV32::Camera(PC,0,0,5400,220);V28ReviewPointer=FVector2D(1760,180);Delay=3;break;
        case 16:
        {
            int32 Alive=0,Onscreen=0;int32 VW=0,VH=0;PC->GetViewportSize(VW,VH);
            if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const auto& T:Fleet->GetVehicles())if(T&&T->IsOperational())
            {++Alive;FVector2D P;if(PC->ProjectWorldLocationToScreen(T->AimPoint(),P)&&P.X>=0&&P.X<=VW&&P.Y>=0&&P.Y<=VH)++Onscreen;}
            Check(TEXT("four_operational_tanks_survive_visual_warmup"),Alive==4);
            Check(TEXT("four_tank_aimpoints_project_inside_viewport"),Onscreen==4);
            Shot(TEXT("08-terrain-vehicles-and-sections"));break;
        }
        case 17:Check(TEXT("combat_visual_fixture"),SetupV35CombatVisualFixture(B));Delay=1.6;break;
        case 18:Shot(TEXT("09-smoke-gas-normal-view"));R->GpuStart=Now;R->LastFrame=Now;Delay=0;break;
        case 19:
        {
            const double Ms=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
            if(Ms>0 && Ms<1000){R->GpuTotal+=Ms;R->FrameTotal+=(Now-R->LastFrame)*1000.;++R->GpuSamples;}R->LastFrame=Now;
            if(Now-R->GpuStart<2.5){R->Next=Now;return true;}
            R->Report->SetNumberField(TEXT("gpu_samples"),R->GpuSamples);R->Report->SetNumberField(TEXT("mean_gpu_ms"),R->GpuSamples?R->GpuTotal/R->GpuSamples:0);R->Report->SetNumberField(TEXT("mean_observed_frame_ms"),R->GpuSamples?R->FrameTotal/R->GpuSamples:0);
            R->Report->SetStringField(TEXT("gpu_scope"),TEXT("Single normal-camera cosmetic fixture with 12 established smoke/gas volumes; FG disabled; no frame-rate guarantee inferred."));
            Check(TEXT("gpu_sample_has_data"),R->GpuSamples>10);break;
        }
        case 20:Select(EFrontlineFaction::German,0);Delay=.32;break;
        case 21:Shot(TEXT("10-archive-photo-transition"));break;
        case 22:Shot(TEXT("11-german-operations-en"));break;
        default:
        {
            Check(TEXT("english_active_at_end"),FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English);
            Check(TEXT("affected_screens_requested"),R->Shots==11);Check(TEXT("visible_localization_coverage_complete"),FrontlineLocalizationV35::WriteCoverageReport());
            R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetNumberField(TEXT("screenshots_requested"),R->Shots);R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetStringField(TEXT("scope"),TEXT("V35 bounded movement/combat checks, actual EN/Chinese UI, photographic transitions, locked roster and support sight. Smoke/gas image uses explicitly aged cosmetic volumes, not natural-arrival proof. Images require independent visual review."));
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))R->Report->SetObjectField(TEXT("render"),Render->GetDiagnostics());
            FString Out;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Out));FFileHelper::SaveStringToFile(Out,*(R->Directory/TEXT("review.json")));
            V28ReviewPointer.Reset();V27ImageAuditActive=false;FPlatformMisc::RequestExit(false);return false;
        }
        }
        ++R->Stage;R->Next=Now+Delay;return true;
    }));
}
}
