// Focused Editor-only shared-feature review. All qualification, career and
// report inputs are labelled fixtures in an exact disposable profile.
#if WITH_EDITOR
#include "FRRUParityReviewSafetyV87.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "TextureCompiler.h"
#include "FRRUOutcomeReviewV87.inl"
namespace FRRUParityReviewV87
{
struct FRun
{
    FString User;TWeakObjectPtr<AWarfareFrontlineController> Controller;
    TSharedPtr<FrontlineAchievementsV76::FNationalParityFixtureV87> Account;
    TArray<TSharedPtr<FJsonValue>> Checks,Captures,ArmourSchoolWarmups;
    double Started=FPlatformTime::Seconds(),Next=0;uint64 Frame=0;
    double ArmourSchoolWarmStarted=0;uint64 ArmourSchoolReadyFrame=MAX_uint64;
    int32 Stage=0,Page=0;FGuid SourceCampaign;bool Good=true;
    const bool ArmourSchoolOnly=FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87ArmourSchoolOnly"));
    const bool AllocationOnly=FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87AllocationOnly"));
    void Check(const FString& Name,bool Passed)
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Passed;
        if(!Passed)UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_PARITY_CHECK_FAILED %s"),*Name);}
    bool Finish()
    {
        auto* PC=Controller.Get();auto* B=PC?FrontlineFor(PC):nullptr;
        if(B&&FrontlineUltimateArchiveV8215::Active(B))FrontlineUltimateArchiveV8215::Leave(B);
        Account.Reset();V28ReviewPointer.Reset();V27ImageAuditActive=false;PageCanvasV73::Audit=false;UIMotionReviewV83::NumberAge=-1;
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("captures"),Captures);
        J->SetBoolField(TEXT("synthetic_qualification_and_report_fixture"),true);J->SetBoolField(TEXT("natural_campaign_completion_accepted"),false);
        J->SetBoolField(TEXT("armour_school_only"),ArmourSchoolOnly);
        if(ArmourSchoolOnly)J->SetArrayField(TEXT("armour_school_warmups"),ArmourSchoolWarmups);
        J->SetBoolField(TEXT("allocation_only"),AllocationOnly);
        J->SetStringField(TEXT("scope"),AllocationOnly?TEXT("Isolated production medal/archive contracts and four actual French/Russian EN/ZH special-allocation deployment frames using labelled archive qualification fixtures. Public selector mission 9, Begin, armour tab and free pioneer attachment; English frames hover the native armoured-reserve description. Actual archive return preserves selection and original campaign GUID; private-byte restoration remains required. Archive UI fixture only: no natural campaign completion, battle balance, HDR/FG, Windows DPI or Shipping acceptance."):ArmourSchoolOnly?TEXT("Isolated production medal/archive contracts and four actual French/Russian EN/ZH armour-school frames using labelled qualification fixtures. Actual public research page and native option toggles; synthetic late progression with actual paid attachment disk transactions/cold reload and exact private-byte restoration. No natural campaign completion, battle balance, HDR/FG or Shipping acceptance."):TEXT("Isolated production medal codec/atomic settlement and receipt deduplication; French/Russian public archive Begin/Leave, native option contracts, both-side availability and original runtime restoration; synthetic late progression with actual paid attachment disk transactions/cold reload and exact private-byte restoration. Twenty actual 4K EN/ZH UI frames use labelled qualification/career/report fixtures. No natural campaign, combat balance, HDR/FG, Windows DPI or sustained performance acceptance."));
        J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Started);
        FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));
        if(!FFileHelper::SaveStringToFile(Text,*(User/TEXT("checks.json"))))Good=false;
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_PARITY_FINISHED success=%d captures=%d"),Good,Captures.Num());
        FPlatformMisc::RequestExitWithStatus(false,Good?0:1);return false;
    }
    void Clear(AWarfareFrontlineController* PC)
    {
        auto& S=ScreenState(PC);S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
        S.Preparing=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.Dossier=1917;
        S.bPausedByOverlay=S.bPausedForAuxOverlay=false;MedalMountV8216::CloseV836(PC);MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;
        FrontlineReportV839::View(PC)={};UltimateArchiveUIV8215::Close(PC);PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);UGameplayStatics::SetGamePaused(PC,false);
        UIMotionReviewV83::NumberAge=-1;
    }
    bool Tick()
    {
        const double Now=FPlatformTime::Seconds();
        if(Now-Started>240){Check(TEXT("v87_parity_bounded_240_seconds"),false);return Finish();}
        auto* PC=Controller.Get();auto* B=PC?FrontlineFor(PC):nullptr;
        if(!PC||!B)
        {
            if(GEngine)for(const auto& Context:GEngine->GetWorldContexts())if(auto* W=Context.World();W&&W->WorldType==EWorldType::Game)
                if(auto* C=Cast<AWarfareFrontlineController>(W->GetFirstPlayerController());C&&C->HasActorBegunPlay())
                    if(auto* Battle=FrontlineFor(C);Battle&&Battle->HasActorBegunPlay()){Controller=C;break;}
            return true;
        }
        FString ActualUser;if(!FRRUParityReviewSafetyV87::UserDir(ActualUser)||ActualUser!=User||!B->IsProgressionVolatile())
        {Check(TEXT("v87_parity_isolation_remains_valid"),false);return Finish();}
        if(Now<Next||FScreenshotRequest::IsScreenshotRequested())return true;
        auto CheckFn=[&](const FString& Name,bool Passed){Check(Name,Passed);};
        auto& S=ScreenState(PC);auto* HUD=PC->GetHUD();if(!HUD){Check(TEXT("v87_parity_actual_hud_exists"),false);return Finish();}
        if(Stage==0)
        {
            Clear(PC);Check(TEXT("v87_parity_begins_only_in_briefing"),B->Phase==EFrontlinePhase::Briefing);if(!Good)return Finish();
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
            {Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Native,false);}
            Account=FrontlineAchievementsV76::BeginNationalParityFixtureV87(B,CheckFn);Check(TEXT("v87_parity_fixture_lifetime_acquired"),Account.IsValid());if(!Good)return Finish();
            Check(TEXT("v87_parity_disposable_french_source_started"),WarfareFRRUCampaignV82::Start(B,EFRRUNationV81::French,EFrontlineDifficulty::Veteran));
            if(!Good)return Finish();WarfareFRRUCampaignV82::RunArchiveReplayChecksV87(B,CheckFn);if(!Good)return Finish();
            B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);if(ArmourSchoolOnly||AllocationOnly)Page=3;Stage=1;Next=Now+.2;return true;
        }
        const auto Nation=Page<10?EFRRUNationV81::French:EFRRUNationV81::Russian;
        const int32 Kind=Page%5;const bool Chinese=(Page%10)>=5;
        const FString Prefix=Nation==EFRRUNationV81::French?TEXT("fr"):TEXT("ru"),Language=Chinese?TEXT("zh"):TEXT("en");
        if(Stage==1)
        {
            FrontlineLocalizationV35::SetLanguage(Chinese?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
            Clear(PC);V27ImageAuditActive=true;V27ImageAudit.Empty();PageCanvasV73::Audit=true;PageCanvasV73::Hits.Empty();
            if(Kind==0||ArmourSchoolOnly||AllocationOnly)
            {
                if(FrontlineUltimateArchiveV8215::Active(B))FrontlineUltimateArchiveV8215::Leave(B);
                Check(Prefix+TEXT("_source_campaign_for_ui_")+Language,WarfareFRRUCampaignV82::Start(B,Nation,EFrontlineDifficulty::Veteran));
                SourceCampaign=WarfareFRRUCampaignV82::CampaignId(B);FRRUWarRoomV82::Synchronise(PC);
                MedalWallV8215::PreloadVariants(FrontlineMedalsV8215::NationIndices(Nation),true);
                Check(Prefix+TEXT("_warroom_has_national_ribbons_")+Language,!MedalWallV8215::OrderedRibbonItems(Nation).IsEmpty());
            }
            if(Kind==1)
            {
                const int32 Medal=Nation==EFRRUNationV81::French?6:9;
                HUD->NotifyHitBoxClick(MedalWallV8215::WarRoomRibbonNameV8444(Nation,Medal));const auto& M=MedalWallV8215::View(PC);
                Check(Prefix+TEXT("_actual_ribbon_click_opens_same_nation_wall_")+Language,S.AchievementsV76&&M.Wall&&M.Nation==int32(Nation)&&M.Focus==Medal
                    &&WarfareFRRUCampaignV82::CampaignId(B)==SourceCampaign);
            }
            if(Kind==2)
            {
                HUD->NotifyHitBoxClick(TEXT("V8215ArchiveOpen"));HUD->NotifyHitBoxClick(FName(*FString::Printf(TEXT("V8215ArchiveFaction%d"),int32(Nation))));
                const auto& A=UltimateArchiveUIV8215::View(PC);
                Check(Prefix+TEXT("_actual_archive_tab_preserves_source_guid_")+Language,UltimateArchiveUIV8215::Active(PC)
                    &&UltimateArchiveUIV8215::SelectedNationV87(A)==Nation&&WarfareFRRUCampaignV82::CampaignId(B)==SourceCampaign);
            }
            if(Kind==3)
            {
                // Re-open the same real selector after the preceding capture;
                // no direct session constructor or fake technology mask.
                HUD->NotifyHitBoxClick(TEXT("V8215ArchiveOpen"));HUD->NotifyHitBoxClick(FName(*FString::Printf(TEXT("V8215ArchiveFaction%d"),int32(Nation))));
                if(AllocationOnly)HUD->NotifyHitBoxClick(TEXT("V8215ArchiveMission9"));
                HUD->NotifyHitBoxClick(TEXT("V8215ArchiveBegin"));
                Check(Prefix+TEXT("_actual_selector_begin_enters_native_archive_")+Language,FrontlineUltimateArchiveV8215::Active(B)
                    &&FrontlineUltimateArchiveV8215::ReplayEligible(B)&&WarfareFRRUCampaignV82::ArchiveReplayNationV87(B)==Nation
                    &&WarfareFRRUCampaignV82::CampaignId(B)!=SourceCampaign&&FrontlineUltimateArchiveV8215::FullArmory(B,0)&&FrontlineUltimateArchiveV8215::FullArmory(B,1));
                if(AllocationOnly)
                {
                    Check(Prefix+TEXT("_public_mission_9_begin_opens_native_deployment_")+Language,S.Deployment&&!S.Technology
                        &&B->Phase==EFrontlinePhase::Briefing&&WarfareFRRUCampaignV82::ArchiveReplayMissionV87(B)==9);
                    const int32 Points=WarfareFRRUCampaignV82::ResearchPoints(B);
                    HUD->NotifyHitBoxClick(TEXT("V63DeploymentArmour"));HUD->NotifyHitBoxClick(TEXT("V63Requisition0"));
                    Check(Prefix+TEXT("_public_armour_tab_and_free_pioneer_attachment_")+Language,S.Deployment&&S.DeploymentTabV63==1
                        &&FrontlineArmourV63::Requisition(B)==0&&WarfareFRRUCampaignV82::ResearchPoints(B)==Points);
                    // FFrontlineCanvas consumes V28ReviewPointer directly in its
                    // 1920x1080 authored space, including at the 4K viewport.
                    if(!Chinese)V28ReviewPointer=FVector2D(112+2*572+552*.5f,806+58*.5f);
                }
                else
                {
                    S.Deployment=false;S.Technology=true;const FString Basic=Prefix+TEXT(".tech.0");
                    HUD->NotifyHitBoxClick(FName(*(TEXT("FRRUOption:")+Basic)));
                    Check(Prefix+TEXT("_actual_technology_button_disables_native_option_")+Language,!WarfareFRRUCampaignV82::OwnsOption(B,Basic));
                    HUD->NotifyHitBoxClick(FName(*(TEXT("FRRUOption:")+Basic)));
                    for(int32 Pass=0;Pass<12;++Pass)for(const auto& O:WarfareFRRUCampaignV82::Options(B))if(!WarfareFRRUCampaignV82::OwnsOption(B,O.Id))HUD->NotifyHitBoxClick(FName(*(TEXT("FRRUOption:")+O.Id)));
                    bool All=true;for(const auto& O:WarfareFRRUCampaignV82::Options(B))All&=O.Owned;
                    Check(Prefix+TEXT("_actual_technology_button_restores_native_options_")+Language,All&&WarfareFRRUCampaignV82::OwnsOption(B,Basic));
                    if(ArmourSchoolOnly)
                    {
                        const FString Crew=Prefix+TEXT(".armour.crew");
                        HUD->NotifyHitBoxClick(FName(*(TEXT("FRRUOption:")+Crew)));
                        bool Disabled=true;for(const TCHAR* KindId:{TEXT("crew"),TEXT("repair"),TEXT("prepare")})Disabled&=!WarfareFRRUCampaignV82::OwnsOption(B,Prefix+TEXT(".armour.")+KindId);
                        Check(Prefix+TEXT("_armour_crew_toggle_cascades_real_dependants_")+Language,Disabled);
                        for(const TCHAR* KindId:{TEXT("crew"),TEXT("repair"),TEXT("prepare")})HUD->NotifyHitBoxClick(FName(*(TEXT("FRRUOption:")+Prefix+TEXT(".armour.")+KindId)));
                        bool Owned=true,Copy=true;int32 Count=0;
                        for(const auto& O:WarfareFRRUCampaignV82::Options(B))if(NationalArmourResearchV87(O))
                        {++Count;Owned&=O.Owned;Copy&=!O.Title.IsEmpty()&&!O.Description.IsEmpty()&&!WarfareFRRUTextV82::English(O.Id,TEXT("title")).IsEmpty();}
                        Check(Prefix+TEXT("_three_real_armour_options_have_paired_copy_and_are_enabled_")+Language,Owned&&Copy&&Count==3);
                        SynchroniseNationalResearchV82(B,S);HUD->NotifyHitBoxClick(TEXT("FRRUResearchArmourV87"));
                        Check(Prefix+TEXT("_actual_armour_school_tab_selected_")+Language,S.FRRUResearchPageV87==1);
                        // Reuse the production asynchronous UI cache and its
                        // resource-init/RHI readiness, rather than treating a
                        // loaded Texture2D pointer as a rendered atlas.
                        FrontlineUIAssetsV8213::PreloadCore();NationalArmourResearchAtlasV87();
                        ArmourSchoolWarmStarted=Now;ArmourSchoolReadyFrame=MAX_uint64;
                    }
                }
            }
            if(Kind==4)
            {
                Check(Prefix+TEXT("_synthetic_report_prepared_")+Language,FrontlineReportV839::PrepareNationalCampaignVisualV87(B,Nation,CheckFn));
                S.CampaignEndingV51=true;HUD->NotifyHitBoxClick(TEXT("CampaignDetailedReportV86"));
                UIMotionReviewV83::NumberAge=10; // Existing layout-only number clock override, not a production animation change.
                Check(Prefix+TEXT("_actual_national_detailed_report_route_")+Language,FrontlineReportV839::Active(PC,B)&&FrontlineReportV839::View(PC).CampaignV86
                    &&FrontlineReportV839::View(PC).Campaign==WarfareFRRUCampaignV82::CampaignId(B));
            }
            if(!Good)return Finish();Frame=GFrameCounter;Stage=2;Next=Now+.7;return true;
        }
        if(Stage==2)
        {
            if(GFrameCounter<=Frame+2)return true;
            // Capture the requested page only after its actual presentation
            // has completed. A transition photograph is not a war-room frame.
            if(S.PhotoTransitionV37||(S.SmallTransitionV35&&Now-S.PageChangedAtV35<.15))
            {ArmourSchoolReadyFrame=MAX_uint64;Next=Now+.1;return true;}
            if(ArmourSchoolOnly)
            {
                auto* Art=NationalArmourResearchAtlasV87();
                const bool Exact=FrontlineTextureReady(Art)&&Art->GetSizeX()==1086&&Art->GetSizeY()==1448;
                const int32 Textures=FTextureCompilingManager::Get().GetNumRemainingTextures(),Assets=FAssetCompilingManager::Get().GetNumRemainingAssets();
                const bool Shaders=GShaderCompilingManager&&GShaderCompilingManager->IsCompiling();
                const int32 PSOs=PipelineStateCache::GetNumActivePipelinePrecompileTasks();
                const bool CacheReady=FrontlineUIAssetsV8213::PreloadReady();
                const bool Resident=Art&&Art->GetNumResidentMips()>0&&Art->IsFullyStreamedIn()&&!Art->HasPendingInitOrStreaming();
                const bool Ready=Exact&&CacheReady&&Resident&&Textures==0&&Assets==0&&!Shaders&&PSOs==0;
                const auto RecordWarmup=[&](bool Passed)
                {
                    auto W=MakeShared<FJsonObject>();W->SetStringField(TEXT("nation"),Prefix);W->SetStringField(TEXT("language"),Language);W->SetBoolField(TEXT("passed"),Passed);
                    W->SetStringField(TEXT("asset"),GetPathNameSafe(Art));W->SetNumberField(TEXT("wait_seconds"),Now-ArmourSchoolWarmStarted);
                    W->SetNumberField(TEXT("remaining_textures"),Textures);W->SetNumberField(TEXT("remaining_assets"),Assets);W->SetBoolField(TEXT("shaders_compiling"),Shaders);W->SetNumberField(TEXT("remaining_psos"),PSOs);
                    W->SetBoolField(TEXT("ui_cache_rhi_ready"),CacheReady);W->SetBoolField(TEXT("atlas_resident"),Resident);W->SetNumberField(TEXT("resident_mips"),Art?Art->GetNumResidentMips():0);
                    W->SetNumberField(TEXT("ready_frame"),ArmourSchoolReadyFrame==MAX_uint64?-1.:double(ArmourSchoolReadyFrame));W->SetNumberField(TEXT("capture_frame"),double(GFrameCounter));
                    if(!Passed)W->SetObjectField(TEXT("ui_cache"),FrontlineUIAssetsV8213::Summary());ArmourSchoolWarmups.Add(MakeShared<FJsonValueObject>(W));
                };
                if(!Ready)
                {
                    ArmourSchoolReadyFrame=MAX_uint64;
                    if(Now-ArmourSchoolWarmStarted<120.){Next=Now+.05;return true;}
                    RecordWarmup(false);Check(Prefix+TEXT("_armour_school_actual_texture_compile_rhi_warmup_within_120_seconds_")+Language,false);return Finish();
                }
                // Once real compilation/resource/streaming readiness holds,
                // require fresh ordinary draw frames. Reset this condition if
                // work reappears; no fixed sleep can qualify an empty first frame.
                if(ArmourSchoolReadyFrame==MAX_uint64){ArmourSchoolReadyFrame=GFrameCounter;Next=Now+.05;return true;}
                if(GFrameCounter<=ArmourSchoolReadyFrame+1){Next=Now+.05;return true;}
                RecordWarmup(true);
                UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_ARMOUR_ATLAS_READY ready=%d exact=%d asset=%s platform_pixels=%dx%d expected=1086x1448 textures=%d assets=%d shaders=%d psos=%d cache_rhi=%d resident=%d ready_frame=%llu capture_frame=%llu"),Ready,Exact,*GetPathNameSafe(Art),Art?Art->GetSizeX():0,Art?Art->GetSizeY():0,Textures,Assets,Shaders,PSOs,CacheReady,Resident,ArmourSchoolReadyFrame,GFrameCounter);
                Check(Prefix+TEXT("_armour_school_actual_texture_compile_rhi_warmup_")+Language,Ready&&GFrameCounter>ArmourSchoolReadyFrame+1);
                Check(Prefix+TEXT("_exact_national_colour_and_old_photo_atlas_loaded_")+Language,Exact);if(!Good)return Finish();
            }
            int32 W=0,H=0;PC->GetViewportSize(W,H);Check(Prefix+TEXT("_actual_4k_")+FString::FromInt(Kind)+TEXT("_")+Language,W==3840&&H==2160);if(!Good)return Finish();
            if(AllocationOnly)
            {
                uint8 AllocationHitMask=0;bool Fits=true,ArmourHover=false;
                for(const auto& Hit:PageCanvasV73::Hits)
                {
                    const auto& HitData=Hit.Value;const FString Command=HitData->GetStringField(TEXT("command"));
                    const double X=HitData->GetNumberField(TEXT("x")),Y=HitData->GetNumberField(TEXT("y"));
                    for(int32 I=0;I<3;++I)
                    {
                        // Disabled native buttons correctly register Blocked;
                        // their description still uses the same hover bounds.
                        if((Command!=FString::Printf(TEXT("V63Requisition%d"),I)&&Command!=TEXT("Blocked"))
                            ||!FMath::IsNearlyEqual(X,double(112+I*572),.1)||!FMath::IsNearlyEqual(Y,806.,.1)
                            ||!FMath::IsNearlyEqual(HitData->GetNumberField(TEXT("width")),552.,.1)
                            ||!FMath::IsNearlyEqual(HitData->GetNumberField(TEXT("height")),58.,.1))continue;
                        AllocationHitMask|=1u<<I;Fits&=HitData->GetBoolField(TEXT("fits"));
                        if(I==2&&V28ReviewPointer.IsSet())
                        {const auto P=V28ReviewPointer.GetValue();ArmourHover=P.X>=X&&P.X<=X+552&&P.Y>=Y&&P.Y<=Y+58;}
                    }
                }
                Check(Prefix+TEXT("_actual_special_allocation_buttons_drawn_in_bounds_")+Language,AllocationHitMask==7&&Fits
                    &&S.Deployment&&S.DeploymentTabV63==1&&!S.Technology&&FrontlineArmourV63::Requisition(B)==0);
                if(!Chinese)Check(Prefix+TEXT("_english_longest_armour_description_hover_uses_actual_button_bounds_")+Language,ArmourHover
                    &&FrontlineArmourV63::RequisitionDescription(2).Len()>FrontlineArmourV63::RequisitionDescription(0).Len()
                    &&FrontlineArmourV63::RequisitionDescription(2).Len()>FrontlineArmourV63::RequisitionDescription(1).Len());
                if(!Good)return Finish();
            }
            static const TCHAR* Names[]={TEXT("warroom"),TEXT("medal-wall-ribbon"),TEXT("archive-four-armies"),TEXT("archive-native-technology"),TEXT("national-campaign-report")};
            const TCHAR* CapturePage=AllocationOnly?TEXT("archive-special-allocation"):ArmourSchoolOnly?TEXT("archive-armour-school"):Names[Kind];
            const FString File=FString::Printf(TEXT("%02d-%s-%s-%s.png"),Page+1,*Prefix,CapturePage,*Language);
            FScreenshotRequest::RequestScreenshot(User/TEXT("Captures")/File,true,false);auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("file"),File);
            J->SetStringField(TEXT("nation"),Prefix);J->SetStringField(TEXT("language"),Language);J->SetStringField(TEXT("page"),AllocationOnly?CapturePage:Names[Kind]);J->SetBoolField(TEXT("synthetic_fixture"),true);
            if(AllocationOnly){J->SetNumberField(TEXT("archive_mission"),9);J->SetBoolField(TEXT("armoured_reserve_hover"),!Chinese);}
            if(ArmourSchoolOnly){J->SetBoolField(TEXT("actual_texture_compile_rhi_ready"),true);J->SetNumberField(TEXT("ready_draw_frame"),double(ArmourSchoolReadyFrame));}
            Captures.Add(MakeShared<FJsonValueObject>(J));
            Stage=3;Next=Now+.15;return true;
        }
        if(Stage==3)
        {
            if(Kind==3)
            {
                Check(Prefix+TEXT("_real_archive_return_to_selection_")+Language,EndArchiveReplayV8215(PC,B,true));
                const auto& A=UltimateArchiveUIV8215::View(PC);
                Check(Prefix+TEXT("_return_keeps_nation_mission_and_original_guid_")+Language,!FrontlineUltimateArchiveV8215::Active(B)
                    &&UltimateArchiveUIV8215::Active(PC)&&UltimateArchiveUIV8215::SelectedNationV87(A)==Nation&&A.Mission==(AllocationOnly?9:0)
                    &&WarfareFRRUCampaignV82::CampaignId(B)==SourceCampaign);
            }
            if(!Good)return Finish();Page+=(ArmourSchoolOnly||AllocationOnly)?5:1;Stage=Page<20?1:4;Next=Now+.1;return true;
        }
        if(Stage==4)
        {
            int32 Written=0;for(const auto& V:Captures)if(IFileManager::Get().FileSize(*(User/TEXT("Captures")/V->AsObject()->GetStringField(TEXT("file"))))>1024)++Written;
            if(Written<Captures.Num()&&Now-Next<3)return true;
            const int32 Expected=(ArmourSchoolOnly||AllocationOnly)?4:20;
            Check(AllocationOnly?TEXT("v87_all_four_actual_special_allocation_bilingual_4k_captures_written"):ArmourSchoolOnly?TEXT("v87_all_four_actual_armour_school_bilingual_4k_captures_written"):TEXT("v87_all_twenty_actual_bilingual_4k_captures_written"),Captures.Num()==Expected&&Written==Expected);return Finish();
        }
        Check(TEXT("v87_known_parity_review_stage"),false);return Finish();
    }
};
FAutoConsoleCommand Command(TEXT("WF.FRRU.ReviewParityV87"),TEXT("Strict disposable national shared-feature and bilingual UI review."),FConsoleCommandDelegate::CreateLambda([]()
{
    const TCHAR* Cmd=FCommandLine::Get();
    const int32 FocusedModes=int32(FParse::Param(Cmd,TEXT("WFFRRUV87OutcomeOnly")))+int32(FParse::Param(Cmd,TEXT("WFFRRUV87ArmourSchoolOnly")))+int32(FParse::Param(Cmd,TEXT("WFFRRUV87AllocationOnly")));
    if(FocusedModes>1){UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_PARITY_REFUSED: focused review modes are mutually exclusive"));return;}
    FString User;if(!FRRUParityReviewSafetyV87::UserDir(User)){UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_PARITY_REFUSED: exact review flags and fixed fresh UserDir required"));return;}
    const FString Marker=User/TEXT("parity-started.json");const FString Account=User/TEXT("AccountReview/achievements-v2.json");const FString Repository=User/TEXT("FRRUReview/campaigns-tactical-v2.json");
    if(IFileManager::Get().FileExists(*Marker)||IFileManager::Get().FileExists(*Account)||IFileManager::Get().FileExists(*(Account+TEXT(".bak")))
        ||IFileManager::Get().FileExists(*Repository)||IFileManager::Get().FileExists(*(Repository+TEXT(".bak"))))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_PARITY_REFUSED: review profile is not fresh"));return;}
    static bool Requested=false;if(Requested)return;Requested=true;
    IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
    if(!FFileHelper::SaveStringToFile(TEXT("{\"synthetic_fixture\":true,\"natural_completion\":false}"),*Marker))return;
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87OutcomeOnly"))){FRRUOutcomeReviewV87::Start(User);return;}
    auto Run=MakeShared<FRun>();Run->User=User;
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float){return Run->Tick();}));
}));
}
#endif
