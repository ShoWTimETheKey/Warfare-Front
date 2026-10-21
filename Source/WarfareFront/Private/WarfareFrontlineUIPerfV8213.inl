// REVIEW DRAFT ONLY. Not included by production code until the owning agent
// reviews it. Include at the end of WarfareFrontlinePresentation.cpp and call
// StartFrontlineUIPerfV8213Review(this) from controller BeginPlay. The same exact
// bridge must be used for baseline and candidate. No assets/quality are changed.
#include "ProfilingDebugging/MiscTrace.h"

namespace FrontlineUIPerfV8213
{
struct FAction
{
    const TCHAR* Command;FVector2D Pointer;int32 Page,Tab=-1;
    bool NavigationBack=false;
};
struct FFrame
{
    uint64 Frame;double Time,Wall,Game,Render,RHI,GPU;
    int32 Page,Loop,Action;FString Phase;int32 PSOPending;bool WindowFocus,Foreground,AppFocus;
};
struct FRun : TSharedFromThis<FRun>
{
    TWeakObjectPtr<AWarfareFrontlineController> Owner;
    FString Dir,Faction,Label,Phase=TEXT("startup");
    TArray<FFrame> Frames;TArray<TSharedPtr<FJsonValue>> Events,Checks;
    TArray<FAction> Actions;
    double Began=0,Last=0,At=0;uint64 AtFrame=0;
    int32 Loop=0,Loops=2,Action=0,Stage=0;bool Ready=false,Good=true,Interrupted=false,Done=false;
    int32 InitialLegacy=0,InitialNational=0;FGuid InitialId;float FixedCap=0;
    bool ContinueOnlyV8214=false,PersistenceV8214=false;
    TOptional<FVector2D> PreviousPointer;
    bool MouseSeen=false;float MouseX=0,MouseY=0;

    void Check(const FString& Name,bool Pass)
    {
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);
        Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;
    }
    void Event(const TCHAR* Kind,const FString& Command=TEXT(""))
    {
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),Kind);J->SetStringField(TEXT("command"),Command);
        J->SetNumberField(TEXT("timestamp_s"),FPlatformTime::Seconds());J->SetNumberField(TEXT("frame"),double(GFrameCounter));
        J->SetNumberField(TEXT("loop"),Loop);J->SetNumberField(TEXT("action"),Action);
        if(auto* C=Owner.Get())if(auto* H=Cast<AWarfareFrontlineHUD>(C->GetHUD()))J->SetNumberField(TEXT("page"),H->GetPresentationPage());
        Events.Add(MakeShared<FJsonValueObject>(J));
        TRACE_BOOKMARK(TEXT("WFUIPerfV8213 %s loop=%d action=%d %s"),Kind,Loop,Action,*Command);
    }
    bool HumanInput(AWarfareFrontlineController* C)
    {
        // The fixture pointer never changes the hardware pointer/key state.
        // Only monitor the native window: hidden/offscreen runs do not own
        // unrelated activity on the desktop. A native run yields permanently.
        if(FParse::Param(FCommandLine::Get(),TEXT("RenderOffScreen")))return false;
        auto* Viewport=C&&C->GetLocalPlayer()&&C->GetLocalPlayer()->ViewportClient
            ?C->GetLocalPlayer()->ViewportClient->Viewport:nullptr;
        if(!Viewport||!Viewport->IsForegroundWindow())
        {
            // GetAsyncKeyState is desktop-global, and a client cursor can
            // change when another application owns the pointer. Neither is
            // input directed at this game. Resume sampling from a fresh
            // position if the user returns to its native window.
            MouseSeen=false;return false;
        }
        float X=0,Y=0;if(C->GetMousePosition(X,Y))
        {
            if(MouseSeen&&(FMath::Abs(X-MouseX)>.5f||FMath::Abs(Y-MouseY)>.5f))return true;
            MouseSeen=true;MouseX=X;MouseY=Y;
        }
#if PLATFORM_WINDOWS
        for(int32 KeyCode=1;KeyCode<255;++KeyCode)if((GetAsyncKeyState(KeyCode)&0x8000)!=0)return true;
#endif
        return false;
    }
    void Finish()
    {
        if(Done)return;Done=true;
        auto* C=Owner.Get();auto* B=C?FrontlineFor(C):nullptr;
        if(C&&B&&Ready&&Good&&!Interrupted&&Loop>=Loops&&FParse::Param(FCommandLine::Get(),TEXT("WFV834Review")))
        {
            const auto Verify=[this](const FString& Name,bool Pass){Check(Name,Pass);};
            if(auto* Audio=C->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->RunV834SpatialMixChecks(Verify);
            else Check(TEXT("v834_audio_subsystem_present"),false);
            FrontlineMapSceneV61::RunV834GeometryChecks(C,Verify);
            const auto NGX=FactionSelectionV82::CoveredWorldV826().NGXReviewV834();
            if(NGX->GetBoolField(TEXT("cvar_available"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFNGXMenuBaselineV834")))
                Check(TEXT("v834_ngx_menu_feature_pool_retained_then_restored"),NGX->GetNumberField(TEXT("retain_count"))>0
                    &&NGX->GetNumberField(TEXT("restore_count"))>0&&!NGX->GetBoolField(TEXT("owned"))
                    &&NGX->GetNumberField(TEXT("current_value"))==NGX->GetNumberField(TEXT("captured_previous_value")));
            const auto Layout=FrontlineTextLayoutV834::Review();
            Check(TEXT("v834_cached_layout_exact_same_frame_parity"),Layout->GetNumberField(TEXT("same_frame_exact_layout_checks"))>0
                &&Layout->GetNumberField(TEXT("same_frame_exact_layout_failures"))==0&&Layout->GetBoolField(TEXT("bounded")));
        }
        if(B&&Ready)
        {
            const auto* Run=B->GetCurrentCampaign();
            Check(TEXT("synthetic_campaign_identity_and_archive_counts_unchanged"),Run&&Run->Id==InitialId
                &&B->GetCampaignRuns().Num()==InitialLegacy&&WarfareFRRUCampaignV82::Archives(B).Num()==InitialNational);
            Check(TEXT("noncombat_uk_de_only"),B->Phase==EFrontlinePhase::Briefing&&!WarfareFRRUCampaignV82::Active(B));
            if(auto* R=UFrontlineRenderSettingsSubsystem::Get(C->GetWorld()))
                Check(TEXT("high_quality_fg_off_and_fixed_cap_retained"),R->GetGraphicsPreset()==EFrontlineGraphicsPreset::High
                    &&R->GetQuality()==EFrontlineRenderQuality::Quality&&R->GetFrameGeneration()==EFrontlineFrameGeneration::Off
                    &&FMath::IsNearlyEqual(R->GetFrameRateLimit(),FixedCap));
            if(PersistenceV8214)
            {
                FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);FPaths::NormalizeDirectoryName(User);
                const FString Path=User/TEXT("Saved/Gameplay/V8214/Continue/fixture-profile.json");
                FString Written;TSharedPtr<FJsonObject> Disk;
                bool Same=Run&&!B->IsProgressionVolatile()&&FFileHelper::LoadFileToString(Written,*Path)
                    &&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Written),Disk)&&Disk.IsValid();
                if(Same)
                {
                    Same=Disk->GetStringField(TEXT("active_campaign"))==Run->Id.ToString(EGuidFormats::Digits)
                        &&Disk->GetIntegerField(TEXT("player_faction"))==int32(Run->Faction);
                    const auto& Rows=Disk->GetArrayField(TEXT("campaigns"));Same&=Rows.Num()==InitialLegacy;
                    bool Found=false;
                    for(const auto& Value:Rows)if(const auto Row=Value->AsObject();Row&&Row->GetStringField(TEXT("id"))==Run->Id.ToString(EGuidFormats::Digits))
                    {
                        Found=true;Same&=Row->GetStringField(TEXT("updated_utc"))==Run->UpdatedUtc
                            &&Row->GetIntegerField(TEXT("selected_mission"))==Run->SelectedMission
                            &&Row->GetIntegerField(TEXT("completed_missions"))==Run->CompletedMissions
                            &&Row->GetObjectField(TEXT("research"))->GetIntegerField(TEXT("technology_mask"))==Run->Research.TechnologyMask;
                    }
                    Same&=Found;
                }
                Check(TEXT("real_continue_transaction_disk_active_timestamp_route_and_research_match"),Same);
                Check(TEXT("real_continue_transaction_backup_exists"),IFileManager::Get().FileExists(*(Path+TEXT(".backup"))));
            }
        }
        V28ReviewPointer=PreviousPointer;
        FString CSV=TEXT("frame,timestamp_s,wall_ms,game_ms,render_ms,rhi_ms,gpu_ms,faction,page,phase,action,loop,pso_pending,window_focus,foreground,app_focus\n");
        for(const auto& F:Frames)
            CSV+=FString::Printf(TEXT("%llu,%.9f,%.6f,%.6f,%.6f,%.6f,%.6f,%s,%d,%s,%d,%d,%d,%d,%d,%d\n"),
                F.Frame,F.Time,F.Wall,F.Game,F.Render,F.RHI,F.GPU,*Faction,F.Page,*F.Phase,F.Action,F.Loop,F.PSOPending,F.WindowFocus?1:0,F.Foreground?1:0,F.AppFocus?1:0);
        const bool Saved=FFileHelper::SaveStringToFile(CSV,*(Dir/TEXT("frames.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        Check(TEXT("raw_frame_csv_written"),Saved);Check(TEXT("bounded_route_completed"),Ready&&Loop>=Loops);
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("revision"),TEXT("v8213.ui.transitions.1"));
        J->SetBoolField(TEXT("success"),Good&&!Interrupted);J->SetBoolField(TEXT("interrupted"),Interrupted);
        J->SetBoolField(TEXT("window_left_open"),Interrupted);J->SetStringField(TEXT("label"),Label);J->SetStringField(TEXT("faction"),Faction);
        J->SetNumberField(TEXT("requested_loops"),Loops);J->SetNumberField(TEXT("completed_loops"),Loop);
        J->SetNumberField(TEXT("actions_per_loop"),Actions.Num());J->SetNumberField(TEXT("frame_samples"),Frames.Num());
        J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Began);J->SetStringField(TEXT("raw_csv"),Dir/TEXT("frames.csv"));
        J->SetBoolField(TEXT("render_offscreen"),FParse::Param(FCommandLine::Get(),TEXT("RenderOffScreen")));
        J->SetBoolField(TEXT("baseline_requested"),FParse::Param(FCommandLine::Get(),TEXT("WFUIPerfBaselineV8213")));
        J->SetBoolField(TEXT("continue_only_v8214"),ContinueOnlyV8214);
        J->SetBoolField(TEXT("continue_baseline_v8214"),FParse::Param(FCommandLine::Get(),TEXT("WFUIPerfContinueBaselineV8214")));
        J->SetBoolField(TEXT("real_fixture_persistence_v8214"),PersistenceV8214);
        J->SetStringField(TEXT("frame_cap_scope"),TEXT("From fixture setup: observed system refresh, clamped to the legal frame-rate range, via SetFrameRateLimit(value,false); player preferences untouched. Earlier startup frames retain initial defaults."));
        J->SetNumberField(TEXT("frame_cap_hz"),FixedCap);J->SetNumberField(TEXT("frame_budget_ms"),FixedCap>0?1000./FixedCap:0);
        int32 VW=0,VH=0;if(C)C->GetViewportSize(VW,VH);J->SetNumberField(TEXT("viewport_width"),VW);J->SetNumberField(TEXT("viewport_height"),VH);
        J->SetBoolField(TEXT("screenshots_during_samples"),false);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("events"),Events);
        J->SetStringField(TEXT("scope"),TEXT("UK/DE disposable unlocked noncombat fixture. Actual painted HUD hitboxes and production click/navigation handlers; synthetic pointer hover, no OS input. CoreTicker wall intervals from controller BeginPlay onward, including startup wait, fixture setup, hover and every transition; no long-interval filtering. This does not measure process pre-BeginPlay, presentations, input-to-photon, campaign/battle performance or authored appearance. Engine thread/GPU counters may be delayed and are not additive or frame-aligned. Output serialization occurs only after all samples."));
        if(C)if(auto* R=UFrontlineRenderSettingsSubsystem::Get(C->GetWorld()))J->SetObjectField(TEXT("render_diagnostics"),R->GetDiagnostics());
        J->SetObjectField(TEXT("ui_asset_resources"),FrontlineUIAssetsV8213::Summary());
        J->SetObjectField(TEXT("text_layout_v834"),FrontlineTextLayoutV834::Review());
        J->SetObjectField(TEXT("ngx_menu_pool_v834"),FactionSelectionV82::CoveredWorldV826().NGXReviewV834());
        FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));
        FFileHelper::SaveStringToFile(Text,*(Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        // User activity removes the driver and leaves the game/window alone.
        if(!Interrupted)FPlatformMisc::RequestExit(false);
    }
    void AdvancePhase(const TCHAR* Name)
    {Phase=Name;At=FPlatformTime::Seconds();AtFrame=GFrameCounter;}
    bool Delay(double Seconds)const
    {return FPlatformTime::Seconds()-At>=Seconds&&GFrameCounter>AtFrame+2;}
    FString CommandAt(AWarfareFrontlineController* C,AWarfareFrontlineHUD* H,const FVector2D& P,FName* Exact=nullptr)
    {
        int32 W=0,Height=0;C->GetViewportSize(W,Height);const float Scale=FMath::Min(W/1920.f,Height/1080.f);
        if(Scale<=0)return TEXT("");
        const auto* Hit=H->GetHitBoxAtCoordinates(FVector2D((W-1920*Scale)*.5f+P.X*Scale,(Height-1080*Scale)*.5f+P.Y*Scale));
        if(!Hit)return TEXT("");if(Exact)*Exact=Hit->GetName();
        FString Name=Hit->GetName().ToString(),Base,Suffix;return Name.Split(TEXT("__"),&Base,&Suffix)?Base:Name;
    }
    void BuildRoute()
    {
        if(ContinueOnlyV8214)
        {
            // Cold entry plus a repeated actual Continue, using the same
            // production hitboxes. No unrelated page or combat acceptance.
            Actions={{TEXT("ContinueCampaign"),{320,560},1},
                {TEXT("MenuHome"),{330,1038},0},{TEXT("ContinueCampaign"),{320,560},1}};
            return;
        }
        // Layout coordinates are checked against actual painted hitboxes before
        // dispatch. No research purchases, deployment confirmation, save delete,
        // national confirmation, quality switches, update/network actions.
        Actions={
            {TEXT("Settings"),{220,806},7,0},
            {TEXT("SettingsAudio"),{492,202},7,1},{TEXT("SettingsImage"),{776,202},7,2},
            {TEXT("SettingsHDR"),{1060,202},7,3},{TEXT("SettingsVideo"),{208,202},7,0},
            {TEXT("DisplayBack"),{212,987},0},
            {TEXT("Help"),{430,806},5},{TEXT("ManualTermsV829"),{638,202},5},
            {TEXT("Credits"),{1022,202},5},{TEXT("ManualPage"),{254,202},5},{TEXT("Back"),{212,987},0},
            {TEXT("AchievementsV76"),{220,926},19},{TEXT("AchievementV76_0"),{400,260},19},
            {TEXT("AchievementV76_1"),{400,380},19},{TEXT("AchievementsBackV76"),{1474,987},0},
            {TEXT("CampaignArchives"),{320,696},12},{TEXT("MenuHome"),{212,987},0},
            {TEXT("NewCampaign"),{320,628},11},{TEXT("ControllerBack"),{1790,110},0,-1,true},
            {TEXT("ContinueCampaign"),{320,560},1},
            {TEXT("Technology"),{1440,1038},8,0},{TEXT("V32TechNext"),{632,202},8,1},
            {TEXT("V63TechArmour"),{1010,202},8,2},{TEXT("V32TechPrevious"),{254,202},8,0},
            {TEXT("TechnologyBack"),{242,987},1},
            {TEXT("V52Deployment"),{1730,1038},17,0},{TEXT("V63DeploymentArmour"),{926,202},17,1},
            {TEXT("V63DeploymentMain"),{350,202},17,0},{TEXT("V32DeploymentBack"),{242,987},1},
            {TEXT("MenuHome"),{330,1038},0}
        };
    }
    bool Expected(AWarfareFrontlineController* C,AWarfareFrontlineHUD* H,const FAction& A)
    {
        const auto& S=ScreenState(C);if(H->GetPresentationPage()!=A.Page)return false;
        if(A.Tab<0)return true;
        return A.Page==7?S.SettingsTab==A.Tab:A.Page==8?S.TechnologyPage==A.Tab:A.Page==17?S.DeploymentTabV63==A.Tab:true;
    }
    bool Tick(float)
    {
        if(Done)return false;const double Now=FPlatformTime::Seconds();auto* C=Owner.Get();
        auto* B=C?FrontlineFor(C):nullptr;auto* H=C?Cast<AWarfareFrontlineHUD>(C->GetHUD()):nullptr;
        auto* Client=C&&C->GetWorld()?C->GetWorld()->GetGameViewport():nullptr;auto* Viewport=Client?Client->Viewport:nullptr;
        if(Last>0)Frames.Add({GFrameCounter,Now,(Now-Last)*1000.,FPlatformTime::ToMilliseconds(GGameThreadTime),
            FPlatformTime::ToMilliseconds(GRenderThreadTime),FPlatformTime::ToMilliseconds(GRHIThreadTime),
            FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()),H?H->GetPresentationPage():-1,Loop,Action,Phase,
            int32(PipelineStateCache::GetNumActivePipelinePrecompileTasks()),Viewport&&Viewport->HasFocus(),Viewport&&Viewport->IsForegroundWindow(),FApp::HasFocus()});
        Last=Now;
        if(Now-Began>240){Check(TEXT("bounded_240_second_run"),false);Finish();return false;}
        if(!C||!B||!H||!B->LoadedSoldier)return true;
        if(HumanInput(C)){Interrupted=true;Check(TEXT("interrupted_by_native_user_input"),false);Event(TEXT("interrupted"));Finish();return false;}
        if(B->Phase!=EFrontlinePhase::Briefing||WarfareFRRUCampaignV82::Active(B))
        {Check(TEXT("noncombat_uk_de_only"),false);Finish();return false;}
        if(!Ready)
        {
            if(Now-Began<4||!FrontlineUIAssetsV8213::PreloadSettled()||!NavigationV61::ClickSoundSettledV8213())return true;
            Check(TEXT("volatile_profile"),B->IsProgressionVolatile());if(!Good){Finish();return false;}
            if(auto* R=UFrontlineRenderSettingsSubsystem::Get(C->GetWorld()))
            {
                const auto Range=R->GetFrameRateRange();const float Refresh=R->GetActiveDisplay().RefreshHz;
                FixedCap=FMath::Clamp(FMath::RoundToFloat(Refresh>0?Refresh:60.f),float(Range.X),float(Range.Y));
                Check(TEXT("volatile_refresh_frame_cap_applied"),R->SetFrameRateLimit(FixedCap,false)&&FMath::IsNearlyEqual(R->GetFrameRateLimit(),FixedCap));
                if(!Good){Finish();return false;}
            }
            AdvancePhase(TEXT("fixture"));Event(TEXT("fixture_begin"));
            B->AbandonBattleToOperations();
            Check(TEXT("one_unlocked_uk_de_fixture"),FrontlineCampaignV41::PrepareArtReviewOperationV71(B,Faction==TEXT("German")?EFrontlineFaction::German:EFrontlineFaction::British,19,true));
            auto& S=ScreenState(C);S.Dossier=0;S.Menu=S.Help=S.Credits=S.Settings=S.Technology=S.Deployment=S.AchievementsV76=false;
            S.UnitIntroduction=S.Preparing=S.HistoricalDispatch=S.CampaignEndingV51=S.ExitConfirmV61=false;
            S.bPausedByOverlay=S.bPausedForAuxOverlay=S.FRRUArchivesV82=false;S.AcceptedLoadoutMission=INDEX_NONE;
            S.SettingsTab=S.TechnologyPage=S.DeploymentTabV63=0;S.ArchivePendingDelete.Empty();
            UGameplayStatics::SetGamePaused(C,false);C->CancelTargeting();C->ResetCamera();
            const auto* Run=B->GetCurrentCampaign();if(!Run){Check(TEXT("fixture_has_campaign"),false);Finish();return false;}
            InitialId=Run->Id;InitialLegacy=B->GetCampaignRuns().Num();InitialNational=WarfareFRRUCampaignV82::Archives(B).Num();
            if(PersistenceV8214)
            {
                Check(TEXT("fresh_absolute_userdir_fixture_real_save_and_readback"),B->PrepareContinuePersistenceReviewV8214());
                if(!Good){Finish();return false;}
            }
            Ready=true;Stage=-1;BuildRoute();Event(TEXT("fixture_end"));if(!Good){Finish();return false;}return true;
        }
        if(Stage==-1){if(!Delay(2))return true;Stage=0;AdvancePhase(TEXT("steady"));return true;}
        if(Action>=Actions.Num())
        {Action=0;++Loop;if(Loop>=Loops){Event(TEXT("complete"));Finish();return false;}Stage=0;AdvancePhase(TEXT("steady"));}
        const auto& A=Actions[Action];
        if(Stage==0)
        {
            if(!Delay(.7))return true;
            V28ReviewPointer=A.Pointer;Event(TEXT("hover_begin"),A.Command);AdvancePhase(TEXT("hover"));Stage=1;return true;
        }
        if(Stage==1)
        {
            if(!Delay(.25))return true;
            FName Hit;
            if(!A.NavigationBack)
            {
                const FString Actual=CommandAt(C,H,A.Pointer,&Hit);
                if(Actual!=A.Command)
                {
                    // Wait for production animation/hitbox readiness without
                    // dropping these frames or fabricating a clickable region.
                    if(Now-At<6)return true;
                    Check(FString::Printf(TEXT("painted_hitbox_%d_%s_actual_%s"),Action,A.Command,*Actual),false);Finish();return false;
                }
            }
            AdvancePhase(TEXT("transition"));Event(TEXT("dispatch_begin"),A.Command);
            if(ContinueOnlyV8214&&FString(A.Command)==TEXT("ContinueCampaign")
                &&!FParse::Param(FCommandLine::Get(),TEXT("WFUIPerfContinueBaselineV8214")))
                Check(FString::Printf(TEXT("continue_%d_actual_map_view_rendering_before_dispatch"),Action),
                    FrontlineMapSceneV61::ContinueViewWarmedV8214(C));
            if(A.NavigationBack)C->MenuBackV61();else H->NotifyHitBoxClick(Hit);
            Event(TEXT("dispatch_return"),A.Command);Stage=2;return true;
        }
        if(Stage==2)
        {
            if(!Delay(.85))return true;
            const bool Pass=Expected(C,H,A);
            if(!Pass&&Now-At<6)return true;
            Check(FString::Printf(TEXT("route_%d_loop_%d_%s"),Action,Loop,A.Command),Pass);
            Event(TEXT("destination_checked"),A.Command);if(!Pass){Finish();return false;}
            AdvancePhase(TEXT("steady"));Stage=3;return true;
        }
        if(!Delay(.7))return true;++Action;Stage=0;AdvancePhase(TEXT("steady"));return true;
    }
};
}
void StartFrontlineUIPerfV8213Review(AWarfareFrontlineController* C)
{
    FString User,Faction,Label;const TCHAR* Cmd=FCommandLine::Get();
    if(!C||!FParse::Param(Cmd,TEXT("WFUIPerfV8213"))||!FParse::Param(Cmd,TEXT("WFRenderNoSave"))
        ||!FParse::Param(Cmd,TEXT("nosound"))||!FParse::Value(Cmd,TEXT("UserDir="),User)
        ||!FParse::Value(Cmd,TEXT("WFUIPerfFaction="),Faction)||(Faction!=TEXT("British")&&Faction!=TEXT("German")))return;
    if(FPaths::IsRelative(User)||User.Contains(TEXT("..")))return;
    FPaths::NormalizeDirectoryName(User);
    FString Root;FParse::Value(Cmd,TEXT("WFUIPerfRoot="),Root);FPaths::NormalizeDirectoryName(Root);
    // Packaged ProjectDir differs from the source checkout: require the host's
    // exact fresh review-root argument rather than broadening to all UserDirs.
    const FString Allowed=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8213/Transitions"));
    if(Root!=Allowed||!User.StartsWith(Allowed+TEXT("/"),ESearchCase::IgnoreCase))return;
    auto* B=FrontlineFor(C);if(!B||!B->IsProgressionVolatile())return;
    auto* Render=UFrontlineRenderSettingsSubsystem::Get(C->GetWorld());
    if(!Render||Render->GetQuality()!=EFrontlineRenderQuality::Quality||Render->GetFrameGeneration()!=EFrontlineFrameGeneration::Off
        ||Render->GetGraphicsPreset()!=EFrontlineGraphicsPreset::High)return;
    // Other review routes would alter fixture state or collect screenshots.
    for(const TCHAR* Other:{TEXT("WFV75UIReview"),TEXT("WFTypographyV8210"),TEXT("WFUIMotionStableV83"),TEXT("WFAuthenticInputReviewV82")})
        if(FParse::Param(Cmd,Other))return;
    FParse::Value(Cmd,TEXT("WFUIPerfLabel="),Label);
    auto R=MakeShared<FrontlineUIPerfV8213::FRun>();R->Owner=C;R->Faction=Faction;R->Label=Label;
    R->ContinueOnlyV8214=FParse::Param(Cmd,TEXT("WFUIPerfContinueOnlyV8214"));
    R->PersistenceV8214=R->ContinueOnlyV8214&&FParse::Param(Cmd,TEXT("WFUIPerfPersistenceV8214"));
    R->Began=FPlatformTime::Seconds();R->Last=R->Began;R->At=R->Began;R->AtFrame=GFrameCounter;R->PreviousPointer=V28ReviewPointer;
    FParse::Value(Cmd,TEXT("WFUIPerfLoops="),R->Loops);R->Loops=FMath::Clamp(R->Loops,1,2);
    R->Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V8213/UITransitions");IFileManager::Get().MakeDirectory(*R->Dir,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}
