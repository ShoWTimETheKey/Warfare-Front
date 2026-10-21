// Bounded production-page acceptance. One disposable process renders the exact
// final page code and records text, image and actual AddHitBox geometry.
struct FFrontlineUIReviewV75 : public TSharedFromThis<FFrontlineUIReviewV75>
{
    TWeakObjectPtr<AWarfareFrontlineController> PC;
    int32 Page=-1,Language=0,Phase=0,Stage=0;double Started=0,Next=0;bool Good=true;
    FString Dir;TArray<TSharedPtr<FJsonValue>> Checks,Pages;
    double SampleStart=0,LastSample=0;TArray<double> WallMS,GPUMS,GameMS,RenderMS,RHIMS;
    TSharedRef<FJsonObject> Timing(TArray<double> Values)
    {
        auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),Values.Num());
        if(!Values.IsEmpty()){double Sum=0;for(double V:Values)Sum+=V;Values.Sort();J->SetNumberField(TEXT("mean_ms"),Sum/Values.Num());J->SetNumberField(TEXT("p95_ms"),Values[FMath::Clamp(FMath::CeilToInt(Values.Num()*.95)-1,0,Values.Num()-1)]);}
        return J;
    }
    void Check(const FString& Name,bool Pass)
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V75_UI_CHECK %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));}
    void Save(const FString& Name,const TSharedRef<FJsonObject>& J)
    {FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Dir/Name));}
    void Finish()
    {
        if(FParse::Param(FCommandLine::Get(),TEXT("WFTypographyV841")))
        {
            // The manager loads these existing assets asynchronously. Check
            // the unchanged binding after the page run, not before its first load.
            for(const auto& Pair:{TPair<EFrontlineRole,FString>(EFrontlineRole::RifleGrenadier,TEXT("T_Portrait_British_RifleGrenadier")),TPair<EFrontlineRole,FString>(EFrontlineRole::AntiTankRifle,TEXT("T_Portrait_German_AntiTankRifle"))})
            {auto* T=FrontlineCampaignV41::RolePortrait(Pair.Key);Check(TEXT("v841_existing_portrait_ready_")+Pair.Value,FrontlineTextureReady(T)&&T->GetPathName()==TEXT("/Game/Warfare/UI/V41/")+Pair.Value+TEXT(".")+Pair.Value);}
        }
        Check(TEXT("bilingual_runtime_message_coverage"),FrontlineLocalizationV35::WriteCoverageReport());
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("pages"),Pages);
        J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        J->SetStringField(TEXT("scope"),TEXT("V75 re-photographed archival counterparts and cold graphite production pages: paired British/German archival and colour research, all four full-screen settings tabs in EN/ZH, two long HDR explanations, layout/hitbox/navigation/pause checks and actual paused battle/settings navigation. Disposable profile. Manual visual review required; no HDR luminance, generated-frame temporal, performance, full-campaign or cross-hardware claim."));
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV78CopyOnly")))J->SetStringField(TEXT("scope"),TEXT("Focused Chinese production copy pages: research, deployment, manual, settings and calibration explanations at 4K. Actual typography and hitbox checks, disposable profile. No battlefield, map or performance review; manual visual inspection required."));
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV80ConsoleOnly")))J->SetStringField(TEXT("scope"),TEXT("British/German V80 faction console review: early and late actual battle HUD, research, infantry/armour orders, settings, pause and home. Real typography/hitbox and repeat-order dispatch, 14 faction-policy checks. Disposable profile; focused offscreen SDR samples, FG off; manual visual inspection required."));
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV81UIReview")))J->SetStringField(TEXT("scope"),TEXT("V81 exact production UI: both faction consoles, full-page settings/orders, every honours detail in EN/ZH, campaign selection, earned banners, ground artillery sight at two zooms live/paused. Disposable profile, offscreen SDR Quality SR, FG off. Manual inspection required; no hardware cursor/HDR/full campaign claim."));
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV81WorkflowOnly")))J->SetStringField(TEXT("scope"),TEXT("Focused V81 workflow regression: both factions, EN/ZH deployment, settings, honours, pause and home; actual commands, return context, side-key ownership, invisible-command rejection and layout/hitbox checks. Disposable profile, 4K SDR Quality SR, FG off. Manual inspection is separate; no HDR/performance/full campaign claim."));
        J->SetBoolField(TEXT("render_offscreen"),FParse::Param(FCommandLine::Get(),TEXT("RenderOffScreen")));
        J->SetBoolField(TEXT("native_font_layout_audit"),FParse::Param(FCommandLine::Get(),TEXT("WFV825FontLayout")));
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV825FontLayout")))J->SetStringField(TEXT("scope"),TEXT("V82.5 production UI font/layout and exact artwork binding checks. Disposable profile, Quality SR, FG off. Per-page receipts record actual Canvas DPI and measured-versus-drawn glyph bounds. HDR output route, appearance and cached tooltip replay require separate runtime-log and screenshot review; no HDR luminance, cross-hardware or full-campaign claim."));
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV826PortraitOnly")))J->SetStringField(TEXT("scope"),TEXT("Focused restored V82 infantry artwork: original UK/DE resources and UV regions, actual field HUD and deployment pages at 4K, resident objects and real text/image/hitbox bounds. Manual artwork comparison required; no full campaign, HDR or battlefield-performance claim."));
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV827ReleaseGateOnly")))J->SetStringField(TEXT("scope"),TEXT("V82.7 local release gate: EN/ZH four-army selector, blocked FR/RU start/resume/deployment, and retained British/German home and field HUD. Disposable profile, actual packaged rendering and command paths; no FR/RU campaign acceptance."));
        Save(TEXT("review.json"),J);FPlatformMisc::RequestExit(false);
    }
    void WorkflowNavigationChecks(AWarfareFrontlineController* C,AWarfareFrontlineGameMode* B,AWarfareFrontlineHUD* H)
    {
        auto& S=ScreenState(C);auto& History=NavigationV61::History(C);
        const auto Previous=S;const auto PreviousHistory=History;const auto PreviousPhase=B->Phase;
        const bool WasPaused=UGameplayStatics::IsGamePaused(C);
        auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());
        if(!Run){Check(TEXT("workflow_has_disposable_campaign"),false);return;}const auto PreviousRun=*Run;
        ON_SCOPE_EXIT{*Run=PreviousRun;S=Previous;History=PreviousHistory;B->Phase=PreviousPhase;B->SetPaused(WasPaused);};
        auto Reset=[&]()
        {
            S.Menu=S.Settings=S.Help=S.Technology=S.AchievementsV76=S.CampaignEndingV51=false;
            S.Deployment=S.HistoricalDispatch=S.UnitIntroduction=S.Preparing=S.ExitConfirmV61=false;
            S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.ArchivePendingDelete.Empty();
            S.Dossier=1917;B->Phase=EFrontlinePhase::Briefing;B->SetPaused(false);
            History.Back.Empty();History.Forward.Empty();
        };
        for(int32 Workflow=0;Workflow<4;++Workflow)
        {
            Reset();const FGuid Pending=FGuid::NewGuid();
            S.Deployment=Workflow<2;S.DeploymentTabV63=Workflow==1?1:0;
            S.HistoricalDispatch=Workflow==2;
            if(Workflow==3){S.Dossier=1919;S.ArchivePendingDelete.Add(Pending);}
            auto Intact=[&]()
            {return Workflow<2?S.Deployment&&S.DeploymentTabV63==Workflow:Workflow==2?S.HistoricalDispatch:S.ArchivePendingDelete.Num()==1&&S.ArchivePendingDelete.Contains(Pending);};
            const FString Key=FString::Printf(TEXT("workflow%d_language%d_"),Workflow,Language);
            C->ToggleSettings();Check(Key+TEXT("settings_is_foreground"),S.Settings&&H->GetPresentationPage()==7&&Intact());
            H->NotifyHitBoxClick(TEXT("SettingsAudio"));Check(Key+TEXT("actual_audio_click"),S.Settings&&S.SettingsTab==1&&Intact());
            H->NotifyHitBoxClick(TEXT("SettingsHDR"));Check(Key+TEXT("actual_hdr_click"),S.Settings&&S.SettingsTab==3&&Intact());
            C->MenuBackV61();Check(Key+TEXT("side_back_preserves_underlying_workflow"),S.Settings&&S.SettingsTab==1&&Intact());
            const TCHAR* Behind=Workflow<2?TEXT("V32DeploymentBack"):Workflow==2?TEXT("V52DispatchBack"):TEXT("ArchiveDeleteCancel");
            H->NotifyHitBoxClick(FName(Behind));Check(Key+TEXT("hidden_workflow_click_is_rejected"),S.Settings&&Intact());
            H->NotifyHitBoxClick(TEXT("DisplayBack"));Check(Key+TEXT("return_button_restores_workflow"),!S.Settings&&Intact());
            C->ToggleSettings();C->ToggleMenu();Check(Key+TEXT("escape_restores_workflow"),!S.Settings&&Intact());
            if(Workflow<3)
            {C->ToggleHelp();C->ToggleTechnology();Check(Key+TEXT("hidden_help_and_research_cannot_open"),!S.Help&&!S.Technology&&Intact());}
            C->ToggleAchievementsV76();Check(Key+TEXT("honours_is_foreground"),S.AchievementsV76&&H->GetPresentationPage()==19&&Intact());
            // The unified list scrolls; an offscreen command is correctly rejected.
            S.AchievementScrollV82=S.AchievementScrollTargetV82=0;
            H->NotifyHitBoxClick(TEXT("AchievementV76_0"));
            const auto PointerBefore=V28ReviewPointer;V28ReviewPointer=FVector2D(300,500);
            HonoursListV82::Wheel(C,1);HonoursListV82::Advance(S);HonoursListV82::Wheel(C,1);HonoursListV82::Advance(S);
            Check(Key+TEXT("actual_wheel_reveals_honour_without_preselecting"),HonoursListV82::Visible(S,8)&&S.AchievementFocusV76==0);
            V28ReviewPointer=PointerBefore;
            H->NotifyHitBoxClick(TEXT("AchievementV76_8"));Check(Key+TEXT("actual_honour_selection_click"),S.AchievementsV76&&S.AchievementFocusV76==8&&Intact());
            H->NotifyHitBoxClick(FName(Behind));Check(Key+TEXT("honours_rejects_underlying_command"),S.AchievementsV76&&Intact());
            H->NotifyHitBoxClick(TEXT("AchievementsBackV76"));Check(Key+TEXT("honours_back_restores_workflow"),!S.AchievementsV76&&Intact());
            if(Workflow>=2)
            {
                C->ToggleAchievementsV76();C->MenuForwardV61();
                Check(Key+TEXT("honours_side_forward_retains_modal_workflow"),S.AchievementsV76&&Intact());
                C->MenuBackV61();Check(Key+TEXT("honours_side_back_returns_to_modal_workflow"),!S.AchievementsV76&&Intact());
            }
        }
        Reset();S.Dossier=1919;S.ArchivePendingDelete.Add(FGuid::NewGuid());C->ToggleSettings();
        auto OtherContext=NavigationV61::Capture(C);OtherContext.Dossier=1917;
        History.Back.Add(OtherContext);C->MenuBackV61();
        Check(TEXT("settings_history_leaving_archives_discards_pending_delete_" )+FString::FromInt(Language),S.Settings&&S.Dossier==1917&&S.ArchivePendingDelete.IsEmpty());
        Reset();Run->CompletedMissions=B->GetCampaignMissions().Num();
        Check(TEXT("completed_campaign_record_fixture_" )+FString::FromInt(Language),FrontlineCampaignV51::CanOpenEnding(B));
        S.Settings=true;H->NotifyHitBoxClick(TEXT("V51CampaignRecord"));
        Check(TEXT("settings_rejects_hidden_campaign_record_" )+FString::FromInt(Language),S.Settings&&!S.CampaignEndingV51);
        S.Settings=false;S.AchievementsV76=true;const float MapZoom=WarRoom52::State(C).Zoom;bool BoundWheel=false;
        if(C->InputComponent)for(const auto& Binding:C->InputComponent->KeyBindings)
            if(Binding.Chord.Key==EKeys::MouseScrollUp&&Binding.KeyEvent==IE_Pressed)
            {Binding.KeyDelegate.Execute(EKeys::MouseScrollUp);BoundWheel=true;break;}
        Check(TEXT("honours_blocks_background_map_zoom_" )+FString::FromInt(Language),BoundWheel&&!WarRoom52::Active(C)&&FMath::IsNearlyEqual(MapZoom,WarRoom52::State(C).Zoom));
        H->NotifyHitBoxClick(TEXT("V51CampaignRecord"));
        Check(TEXT("honours_rejects_hidden_campaign_record_" )+FString::FromInt(Language),S.AchievementsV76&&!S.CampaignEndingV51);
        S.AchievementsV76=false;S.Help=true;H->NotifyHitBoxClick(TEXT("V51CampaignRecord"));
        Check(TEXT("manual_rejects_hidden_campaign_record_" )+FString::FromInt(Language),S.Help&&!S.CampaignEndingV51);
        S.Help=false;H->NotifyHitBoxClick(TEXT("V51CampaignRecord"));
        Check(TEXT("visible_campaign_record_still_opens_ending_" )+FString::FromInt(Language),S.CampaignEndingV51&&!S.AchievementsV76&&!S.Settings);
        Reset();B->Phase=EFrontlinePhase::Victory;S.AchievementsV76=true;C->BeginBattle();
        Check(TEXT("victory_honours_enter_does_not_advance_" )+FString::FromInt(Language),B->Phase==EFrontlinePhase::Victory&&S.AchievementsV76&&!S.CampaignEndingV51);
        Reset();B->Phase=EFrontlinePhase::Victory;S.Help=true;C->BeginBattle();
        Check(TEXT("victory_manual_enter_does_not_advance_" )+FString::FromInt(Language),B->Phase==EFrontlinePhase::Victory&&S.Help&&!S.CampaignEndingV51);
        Reset();S.CampaignEndingV51=true;C->ToggleAchievementsV76();
        Check(TEXT("campaign_ending_rejects_hidden_honours_" )+FString::FromInt(Language),S.CampaignEndingV51&&!S.AchievementsV76);
    }
    void NavigationChecks(AWarfareFrontlineController* C,AWarfareFrontlineGameMode* B,AWarfareFrontlineHUD* H)
    {
        auto& S=ScreenState(C);auto& History=NavigationV61::History(C);
        History.Back.Empty();History.Forward.Empty();
        S.Menu=true;S.Settings=false;S.Help=S.Technology=false;S.bPausedByOverlay=true;S.bPausedForAuxOverlay=false;B->SetPaused(true);
        H->NotifyHitBoxClick(TEXT("Settings"));
        Check(TEXT("settings_from_pause_keeps_return_context_" )+FString::FromInt(Language),S.Settings&&S.Menu&&UGameplayStatics::IsGamePaused(C)&&!S.bPausedForAuxOverlay);
        H->NotifyHitBoxClick(TEXT("SettingsAudio"));Check(TEXT("audio_tab_dispatch_" )+FString::FromInt(Language),S.SettingsTab==1);
        H->NotifyHitBoxClick(TEXT("SettingsHDR"));Check(TEXT("hdr_tab_dispatch_" )+FString::FromInt(Language),S.SettingsTab==3);
        C->MenuBackV61();Check(TEXT("mouse_side_back_restores_settings_tab_" )+FString::FromInt(Language),S.Settings&&S.SettingsTab==1&&S.Menu&&UGameplayStatics::IsGamePaused(C));
        C->MenuForwardV61();Check(TEXT("mouse_side_forward_restores_settings_tab_" )+FString::FromInt(Language),S.Settings&&S.SettingsTab==3&&S.Menu&&UGameplayStatics::IsGamePaused(C));
        C->ToggleMenu();Check(TEXT("escape_returns_to_paused_menu_" )+FString::FromInt(Language),!S.Settings&&S.Menu&&UGameplayStatics::IsGamePaused(C));
        H->NotifyHitBoxClick(TEXT("Settings"));H->NotifyHitBoxClick(TEXT("DisplayBack"));
        Check(TEXT("back_button_returns_to_paused_menu_" )+FString::FromInt(Language),!S.Settings&&S.Menu&&UGameplayStatics::IsGamePaused(C));
        S.Menu=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;B->SetPaused(false);
        H->NotifyHitBoxClick(TEXT("Settings"));Check(TEXT("live_battle_settings_owns_pause_" )+FString::FromInt(Language),S.Settings&&S.bPausedForAuxOverlay&&UGameplayStatics::IsGamePaused(C));
        H->NotifyHitBoxClick(TEXT("DisplayBack"));Check(TEXT("live_battle_back_restores_running_state_" )+FString::FromInt(Language),!S.Settings&&!S.Menu&&!UGameplayStatics::IsGamePaused(C));
        S.Menu=true;S.bPausedByOverlay=true;B->SetPaused(true);H->NotifyHitBoxClick(TEXT("Settings"));H->NotifyHitBoxClick(TEXT("SettingsVideo"));
        WorkflowNavigationChecks(C,B,H);
    }
    bool Tick(float)
    {
        const double Now=FPlatformTime::Seconds();
        if(!FParse::Param(FCommandLine::Get(),TEXT("WFTypographyV8210"))&&Phase==1&&Now>SampleStart&&Now<Next)
        {
            if(LastSample>0){const double MS=(Now-LastSample)*1000.;if(MS>0&&MS<250)WallMS.Add(MS);}
            LastSample=Now;
            auto Add=[](TArray<double>& A,double V){if(V>0&&V<250)A.Add(V);};
            Add(GPUMS,FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Add(GameMS,FPlatformTime::ToMilliseconds(GGameThreadTime));
            Add(RenderMS,FPlatformTime::ToMilliseconds(GRenderThreadTime));Add(RHIMS,FPlatformTime::ToMilliseconds(GRHIThreadTime));
        }
        if(Now<Next)return true;
        auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;auto* H=C?Cast<AWarfareFrontlineHUD>(C->GetHUD()):nullptr;
        if(Now-Started>400){Check(TEXT("bounded_ui_review"),false);Finish();return false;}
        if(!B||!B->LoadedSoldier||!H){Next=Now+.25;return true;}
#if WITH_EDITOR
        if(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling())){Next=Now+.25;return true;}
#endif
        if(PipelineStateCache::GetNumActivePipelinePrecompileTasks()>0){Next=Now+.25;return true;}
        auto& S=ScreenState(C);auto& Map=WarRoom52::State(C);Next=Now+.15;
        if(Page<0)
        {
            Check(TEXT("disposable_profile"),B->IsProgressionVolatile());
            const auto& A=Artwork();
            // These contracts validate resident objects and exact bindings only.
            // Authored appearance still requires a separate visual review.
            const TCHAR* NationsV825[]={TEXT("British"),TEXT("German"),TEXT("French"),TEXT("Russian")};
            const TCHAR* PartsV825[]={TEXT("Case"),TEXT("Well"),TEXT("Key"),TEXT("Tab"),TEXT("Field"),TEXT("Gauge"),TEXT("Fill"),TEXT("Lock"),TEXT("Settings"),TEXT("Manual"),TEXT("Research"),TEXT("Crest"),TEXT("Terminal")};
            auto ExactTextureV825=[&](const FString& Key,UTexture2D* Texture,const FString& Name,const TCHAR* Directory=TEXT("/Game/Warfare/UI/V825"))
            {
                const FString Expected=FString::Printf(TEXT("%s/%s.%s"),Directory,*Name,*Name);
                Check(Key,FrontlineTextureReady(Texture)&&Texture->GetPathName()==Expected);
            };
            for(int32 F=0;F<4;++F)
            {
                bool Ready=A.NationalConsoleReadyV82[F];
                for(int32 P=0;P<13;++P)Ready&=FrontlineTextureReady(A.ConsoleV79[F][P]);
                Check(FString::Printf(TEXT("v825_%s_all_13_console_parts_resident"),NationsV825[F]),Ready);
            }
            for(int32 F=2;F<4;++F)for(int32 P=5;P<13;++P)
                ExactTextureV825(FString::Printf(TEXT("v825_%s_%s_console_exact_binding"),NationsV825[F],PartsV825[P]),A.ConsoleV79[F][P],
                    FString::Printf(TEXT("T_Console_%s_%s_V825"),NationsV825[F],PartsV825[P]));
            for(int32 P=5;P<13;++P)
            {
                const auto* French=A.ConsoleV79[2][P].Get();const auto* Russian=A.ConsoleV79[3][P].Get();
                bool Independent=French&&Russian&&French!=Russian;
                // The two new watches are intentionally shared with UK/DE;
                // all other FR/RU instruments retain national bindings.
                if(P!=12)for(int32 F=0;F<2;++F)
                    Independent&=French!=A.ConsoleV79[F][P].Get()&&Russian!=A.ConsoleV79[F][P].Get();
                Check(FString::Printf(TEXT("v825_%s_national_instrument_objects_distinct"),PartsV825[P]),Independent);
            }
            ExactTextureV825(TEXT("v825_german_settings_exact_binding"),A.ConsoleV79[1][8],TEXT("T_Console_German_Settings_V825"));
            ExactTextureV825(TEXT("v826_british_unused_watch_asset_retained"),A.ConsoleV79[0][12],TEXT("T_Console_French_Terminal_V825"));
            ExactTextureV825(TEXT("v826_german_unused_watch_asset_retained"),A.ConsoleV79[1][12],TEXT("T_Console_Russian_Terminal_V825"));
            Check(TEXT("v826_unused_watch_objects_distinct"),A.ConsoleV79[0][12]&&A.ConsoleV79[1][12]
                &&A.ConsoleV79[0][12]!=A.ConsoleV79[1][12]);
            const TCHAR* LegacyBasicV16[]={TEXT("T_Portrait_British_Rifle"),TEXT("T_Portrait_British_Lewis"),TEXT("T_Portrait_British_Mills"),TEXT("T_Portrait_German_Rifle"),TEXT("T_Portrait_German_MG"),TEXT("T_Portrait_German_Assault")};
            const TCHAR* LegacySpecialistV16[]={TEXT("T_Portrait_British_Sniper"),TEXT("T_Portrait_British_Officer"),TEXT("T_Portrait_German_Sniper"),TEXT("T_Portrait_German_Officer")};
            for(int32 I=0;I<UE_ARRAY_COUNT(LegacyBasicV16);++I)
                ExactTextureV825(FString::Printf(TEXT("v825_legacy_v16_basic_%d_binding_preserved"),I),A.BasicPortraits[I],LegacyBasicV16[I],TEXT("/Game/Warfare/UI/V16"));
            for(int32 I=0;I<UE_ARRAY_COUNT(LegacySpecialistV16);++I)
                ExactTextureV825(FString::Printf(TEXT("v825_legacy_v16_specialist_%d_binding_preserved"),I),A.SpecialistPortraits[I],LegacySpecialistV16[I],TEXT("/Game/Warfare/UI/V16"));
            for(int32 I=0;I<4;++I)
            {
                const auto& Art=A.RolePaintingsV72[1][I];
                ExactTextureV825(FString::Printf(TEXT("v826_restored_german_role%d_atlas"),I),Art.Texture,TEXT("T_Portraits_German_V75"),TEXT("/Game/Warfare/UI/V75"));
                Check(FString::Printf(TEXT("v826_restored_german_role%d_original_region"),I),Art.Slot==I&&Art.Columns==2&&Art.Rows==2);
            }
            if(!FParse::Param(FCommandLine::Get(),TEXT("WFTypographyV841")))
            {
                ExactTextureV825(TEXT("v826_restored_british_rifle_grenadier"),FrontlineCampaignV41::RolePortrait(EFrontlineRole::RifleGrenadier),TEXT("T_Portrait_British_RifleGrenadier"),TEXT("/Game/Warfare/UI/V41"));
                ExactTextureV825(TEXT("v826_restored_german_antitank_rifle"),FrontlineCampaignV41::RolePortrait(EFrontlineRole::AntiTankRifle),TEXT("T_Portrait_German_AntiTankRifle"),TEXT("/Game/Warfare/UI/V41"));
            }
            ExactTextureV825(TEXT("v825_frru_armour_atlas_exact_binding"),FRRUArmourUIV82::Portraits(),TEXT("T_Armour_FRRU_V825"));
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV80ConsoleOnly"))||FParse::Param(FCommandLine::Get(),TEXT("WFV81UIReview")))
            {
                FrontlineConsoleV79::RunPolicyChecks([&](const FString& Name,bool Pass){Check(Name,Pass);});
                bool Ready=true,Distinct=true,NewParts=true;for(int32 Part=0;Part<13;++Part)
                {Ready&=FrontlineTextureReady(A.ConsoleV79[0][Part])&&FrontlineTextureReady(A.ConsoleV79[1][Part]);Distinct&=A.ConsoleV79[0][Part]!=A.ConsoleV79[1][Part];}
                const bool V81=FParse::Param(FCommandLine::Get(),TEXT("WFV81UIReview"));
                for(int32 F=0;F<2;++F)for(int32 Part:(V81?TArray<int32>{0,5,6}:TArray<int32>{0,1,2,3,4,11,12}))
                {
                    if(!V81&&Part==12)
                    {
                        const FString Name=FString::Printf(TEXT("T_Console_%s_Terminal_V825"),F?TEXT("Russian"):TEXT("French"));
                        NewParts&=A.ConsoleV79[F][Part]&&A.ConsoleV79[F][Part]->GetPathName()==FString::Printf(TEXT("/Game/Warfare/UI/V825/%s.%s"),*Name,*Name);
                    }
                    else NewParts&=A.ConsoleV79[F][Part]&&A.ConsoleV79[F][Part]->GetPathName().Contains(V81?TEXT("/UI/V81/"):TEXT("/UI/V80/"));
                }
                Check(TEXT("all_26_console_parts_resident"),Ready);Check(TEXT("new_authored_parts_bound"),NewParts);Check(TEXT("independently_authored_faction_kits"),Distinct);
                using namespace FrontlineConsoleLayoutV79;
                Check(TEXT("v79_card_hit_contract_excludes_neighbour_gap"),ContainsCard(FVector2D(ArmourX+1,DockTop+CardTop+1),ArmourX,0,DockTop)
                    &&!ContainsCard(FVector2D(InfantryX+CardWidth+1,DockTop+CardTop+1),InfantryX,0,DockTop)
                    &&ContainsCard(FVector2D(InfantryX+CardPitch+1,DockTop+CardTop+1),InfantryX,1,DockTop));
            }
            Check(TEXT("v74_painting_and_archive_materials_loaded"),A.PaintingMaterialV73&&A.ArchivePhotoMaterialV72&&A.ArchivePhotoMaterialV72->GetName()==TEXT("M_ArchivePhoto_V833"));
            Check(TEXT("v8213_cooked_gauge_material_available"),A.ConsolePigmentV8213&&A.ConsolePigmentV8213->GetName()==TEXT("M_ConsolePigment_V8213"));
            bool GaugeInstances=true;for(const auto& Nation:A.ConsolePigmentsV8213)for(const auto& Paint:Nation)GaugeInstances&=Paint!=nullptr;
            Check(TEXT("v8213_national_gauge_instances_available"),GaugeInstances);
            Check(TEXT("v73_native_identity_loaded"),A.BrandWordmarkV72&&A.BrandWordmarkV72->GetName()==TEXT("T_Brand_Wordmark_V73"));
            int32 Revised=0;for(const auto& Art:A.RolePaintingsV72[1])Revised+=Art.Texture&&Art.Texture->GetPathName().Contains(TEXT("/UI/V75/"));
            Check(TEXT("v74_ten_helmet_portraits_bound"),Revised==4);
            // The 72 V75 cards remain exact authored counterparts. V82.15 added
            // one forward-service card per nation; it must not inflate this count.
            int32 ArchiveCount=0;
            for(int32 N=0;N<2;++N)for(int32 I=0;I<36;++I)
            {
                const auto& Card=A.ArchivePaintingsV74[N][I];
                FString Name=FString::Printf(TEXT("T_Archive_%s_%s_%02d_V75"),N?TEXT("german"):TEXT("british"),I<28?TEXT("infantry"):TEXT("armour"),(I<28?I:I-28)/4);
                int32 Slot=I%4;FString Folder=TEXT("V75");
                if(N==1&&I==1){Name=TEXT("T_GermanTechAArchive_V8215");Slot=1;Folder=TEXT("V8215");}
                if(I==27){Name=TEXT("T_MobileGunTech_V8215");Slot=N+2;Folder=TEXT("V8215");}
                const FString Path=FString::Printf(TEXT("/Game/Warfare/UI/%s/%s.%s"),*Folder,*Name,*Name);
                ArchiveCount+=FrontlineTextureReady(Card.Texture)&&Card.Texture->GetPathName()==Path&&Card.Columns==2&&Card.Rows==2&&Card.Slot==Slot;
            }
            Check(TEXT("all_72_authored_archive_regions_loaded"),ArchiveCount==72);
            bool ForwardArchives=true;
            for(int32 N=0;N<2;++N){const auto& Card=A.ArchivePaintingsV74[N][36];ForwardArchives&=FrontlineTextureReady(Card.Texture)&&Card.Texture->GetPathName()==TEXT("/Game/Warfare/UI/V8215/T_ForwardServiceTeams_V8215.T_ForwardServiceTeams_V8215")&&Card.Columns==2&&Card.Rows==2&&Card.Slot==N+2;}
            Check(TEXT("v8215_two_forward_service_archive_regions_loaded"),ForwardArchives);
            Check(TEXT("five_physical_chrome_assets_loaded"),FrontlineTextureReady(A.DeskV74)&&FrontlineTextureReady(A.FieldBoardV74)&&FrontlineTextureReady(A.OrderCardV74)&&FrontlineTextureReady(A.CommandKeyV74)&&FrontlineTextureReady(A.IndexTabV74));
            Check(TEXT("required_full_page_geometry"),PageCanvasV73::Margin==64&&PageCanvasV73::FooterY==936&&FrameRateSliderX==662&&FrameRateSliderWidth==450);
            Page=0;if(FParse::Param(FCommandLine::Get(),TEXT("WFV75MapOnly")))Stage=31;return true;
        }
        static const TCHAR* Names[]={TEXT("main-menu"),TEXT("campaign-selection"),TEXT("campaign-records"),TEXT("company-research"),TEXT("attachment-research"),TEXT("armour-research"),TEXT("infantry-deployment"),TEXT("armour-deployment"),TEXT("field-manual"),TEXT("credits"),TEXT("display-settings"),TEXT("audio-settings"),TEXT("graphics-settings"),TEXT("hdr-settings"),TEXT("hdr-peak-help"),TEXT("hdr-paper-help"),TEXT("hdr-ui-help"),TEXT("hdr-black-help"),TEXT("narrative-introduction"),TEXT("sector-map"),TEXT("paused-command-post"),TEXT("settings-from-pause"),TEXT("field-grain-on"),TEXT("field-grain-off"),TEXT("operation-map"),TEXT("theatre-map"),TEXT("honours"),TEXT("earned-banner"),TEXT("support-sight"),TEXT("field-report"),TEXT("loading"),TEXT("campaign-ending")};
        struct FStage {int32 Page,Language,Faction;bool Research;int32 Focus=0;};
        static const FStage Stages[]={{0,0,0,false},{0,1,1,false},{2,1,1,false},
            {3,0,0,false},{3,0,0,true},{3,1,1,false},{3,1,1,true},
            {4,0,0,false},{4,0,0,true},{4,1,1,false},{4,1,1,true},
            {5,0,0,false},{5,0,0,true},{5,1,1,false},{5,1,1,true},
            {6,0,0,true},{7,1,1,true},{8,1,1,false},{9,0,0,false},
            {10,0,0,false},{11,0,0,false},{12,0,0,false},{13,0,0,false},
            {10,1,1,false},{11,1,1,false},{12,1,1,false},{13,1,1,false},
            {15,0,0,false},{16,1,1,false},{20,1,1,true},{21,0,0,true},{25,0,0,true},{19,0,0,true},{24,0,0,true}};
        const bool V81=FParse::Param(FCommandLine::Get(),TEXT("WFV81UIReview"));
        const bool ConsoleOnly=V81||FParse::Param(FCommandLine::Get(),TEXT("WFV80ConsoleOnly"));
        static const FStage ConsoleStages[]={{23,0,0,true},{23,1,1,true},{22,0,0,true},{22,1,1,true},
            {3,0,0,true},{3,1,1,false},{5,1,1,true},{6,0,0,true},{7,1,1,true},
            {10,0,0,false},{13,1,1,false},{20,1,0,true},{21,0,1,true},{0,0,1,false},
            {4,0,0,true},{4,1,1,false},{11,0,0,false},{11,1,1,false},
            {12,0,0,false},{12,1,1,false},{8,1,0,false},{9,0,1,false}};
        static const TArray<FStage> V81Stages=[]()
        {
            TArray<FStage> R;for(const auto& E:ConsoleStages)R.Add(E);
            R.Add({1,0,0,false});R.Add({1,1,1,false});
            for(int32 L=0;L<2;++L)for(int32 I=0;I<FrontlineAchievementsV76::Count;++I)R.Add({26,L,L,false,I});
            R.Add({27,0,0,true,3});R.Add({27,1,1,true,10});
            for(int32 I=0;I<4;++I)R.Add({28,I%2,I%2,true,I});
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV826PortraitOnly")))R.Add({6,1,1,true});
            return R;
        }();
        const bool Typography=FParse::Param(FCommandLine::Get(),TEXT("WFTypographyV8210"));
        static const TArray<FStage> TypographyStages=[]()
        {
            TArray<FStage> R;for(int32 L=0;L<2;++L)R.Add({29,L,L,true});
            if(FParse::Param(FCommandLine::Get(),TEXT("WFTypographyReportOnly")))return R;
            if(FParse::Param(FCommandLine::Get(),TEXT("WFTypographyV841")))
            {
                const bool Focused=FParse::Param(FCommandLine::Get(),TEXT("WFTypographyDPIFocused"));
                if(Focused){for(int32 P:{30,26,1,12,20,24,0,31,18})for(int32 L=0;L<2;++L)R.Add({P,L,L,true});}
                else {for(int32 P:{30,3,5,6,7,18,26,0,1,10,12,13,15,8,24,20,31})for(int32 L=0;L<2;++L)R.Add({P,L,L,true});}
                for(int32 L=0;L<2;++L)for(int32 Topic:Focused?TArray<int32>{4}:TArray<int32>{1,2,3,4,5})R.Add({8,L,L,true,Topic});
                return R;
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("WFTypographyDPIFocused")))
            {for(int32 P:{30,5,26,0,10,23})for(int32 L=0;L<2;++L)R.Add({P,L,L,true});return R;}
            for(int32 P:{30,3,5,6,7,18,26,0,1,10,13,8,24,23,31})for(int32 L=0;L<2;++L)R.Add({P,L,L,true});return R;
        }();
        const bool CopyOnly=FParse::Param(FCommandLine::Get(),TEXT("WFV78CopyOnly"));
        if(CopyOnly)while(Stage<UE_ARRAY_COUNT(Stages)&&!(Stages[Stage].Language==1||Stages[Stage].Page==6||Stages[Stage].Page==15)&&Stages[Stage].Page<19)++Stage;
        if(V81&&FParse::Param(FCommandLine::Get(),TEXT("WFV81WorkflowOnly")))
            while(Stage<V81Stages.Num())
            {
                const auto& Entry=V81Stages[Stage];
                if(Entry.Page==23||Entry.Page==6||Entry.Page==7||Entry.Page==11||Entry.Page==21||Entry.Page==0||(Entry.Page==26&&Entry.Focus==0))break;
                ++Stage;
            }
        if(V81&&FParse::Param(FCommandLine::Get(),TEXT("WFV826PortraitOnly")))
            while(Stage<V81Stages.Num())
            {
                const auto& Entry=V81Stages[Stage];
                if(Entry.Page==23||Entry.Page==6)break;
                ++Stage;
            }
        if(V81&&FParse::Param(FCommandLine::Get(),TEXT("WFV827ReleaseGateOnly")))
            while(Stage<V81Stages.Num()&&V81Stages[Stage].Page!=0&&V81Stages[Stage].Page!=1&&V81Stages[Stage].Page!=23)++Stage;
        if(Stage>=(Typography?TypographyStages.Num():V81?V81Stages.Num():ConsoleOnly?UE_ARRAY_COUNT(ConsoleStages):UE_ARRAY_COUNT(Stages))||(CopyOnly&&Stages[Stage].Page>=19)){Finish();return false;}
        const auto& Current=Typography?TypographyStages[Stage]:V81?V81Stages[Stage]:ConsoleOnly?ConsoleStages[Stage]:Stages[Stage];Page=Current.Page;Language=CopyOnly?1:Current.Language;
        const FString Key=FString::Printf(TEXT("%02d-%s-%s-%s-%s-%02d"),Stage,Names[Page],Current.Faction?TEXT("german"):TEXT("british"),Language?TEXT("zh"):TEXT("en"),Current.Research?TEXT("colour"):TEXT("archive"),Current.Focus);
        if(Phase==0)
        {
            B->AbandonBattleToOperations();FrontlineReviewUIV41::Clear(C);S.Credits=false;S.ExitConfirmV61=false;S.ManualBattleV841=false;S.ManualTermsV829=false;S.Dossier=1917;S.SettingsTab=0;
            FrontlineLocalizationV35::SetLanguage(Language?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
            const bool Fixture=FrontlineCampaignV41::PrepareArtReviewOperationV71(B,EFrontlineFaction(Current.Faction),ConsoleOnly&&Page==22?19:Page>=24?8:Page>=22?5:Page==18?4:19,Current.Research);
            Check(Key+TEXT("_fixture"),Fixture);if(!Fixture){Finish();return false;}
            S.AchievementToastV81=INDEX_NONE;V28ReviewAim.Reset();C->CancelTargeting();
            B->SetActorTickEnabled(false);V28ReviewPointer=FVector2D(1880,888);
            FrontlineMapSceneV61::Update(C,false,Map.Centre,Map.Zoom,1,Map.AtlasSector,Map.Preview,Map.Detail);
            TypographyReviewPageV8210=Typography&&Page>=29?Page:INDEX_NONE;
            if(Typography&&Page==29){B->BeginBattle();B->SetActorTickEnabled(false);B->Phase=Current.Faction?EFrontlinePhase::Defeat:EFrontlinePhase::Victory;B->EndReason=Current.Faction?TEXT("我军士气瓦解，部队无法继续坚守。"):TEXT("敌军士气瓦解，我军守住了战线。");B->Casualties[0]=42;B->Casualties[1]=78;B->Elapsed=267.f;}
            else if(Typography&&Page==30){FrontlineLoadingPageV82::Prewarm(B,S);}
            else if(Typography&&Page==31){if(auto* R=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))R->CompletedMissions=B->GetCampaignMissions().Num();}
            else if(Page==0){S.Dossier=0;C->ResetCamera();}
            else if(Page==1)S.Dossier=1918;
            else if(Page==2)S.Dossier=1919;
            else if(Page>=3&&Page<=5)
            {
                S.Technology=true;S.TechnologyPage=Page-3;V28ReviewPointer=Page==5?FVector2D(220,400):FVector2D(240,360);
                if(Page==5&&Current.Research)for(int32 Id:{0,1,2,3,4})Check(FString::Printf(TEXT("mixed_armour_research_%d"),Id),FrontlineArmourV63::BuyResearch(B,Id));
            }
            else if(Page==6||Page==7)
            {
                S.Deployment=true;S.DeploymentTabV63=Page==7?1:0;V28ReviewPointer=FVector2D(1880,888);
                C->ToggleSettings();Check(Key+TEXT("_settings_covers_deployment"),S.Settings&&S.Deployment&&H->GetPresentationPage()==7);
                C->ToggleMenu();Check(Key+TEXT("_escape_preserves_deployment_return"),!S.Settings&&S.Deployment&&S.DeploymentTabV63==(Page==7?1:0));
            }
            else if(Page==8||Page==9){S.Help=true;S.Credits=Page==9;if(FParse::Param(FCommandLine::Get(),TEXT("WFTypographyV841"))&&Current.Focus>0){S.ManualTermsV829=Current.Focus==5;S.ManualBattleV841=Current.Focus<5;S.ManualBattleTopicV841=FMath::Clamp(Current.Focus-1,0,3);}}
            else if(Page>=10&&Page<=17)
            {
                H->NotifyHitBoxClick(TEXT("Settings"));LoadDisplayChoices(S);
                const TCHAR* Tabs[]={TEXT("SettingsVideo"),TEXT("SettingsAudio"),TEXT("SettingsImage"),TEXT("SettingsHDR")};
                H->NotifyHitBoxClick(FName(Tabs[FMath::Min(Page-10,3)]));
                if(Page>=14)V28ReviewPointer=FVector2D(300,426+(Page-14)*76);
                Check(Key+TEXT("_actual_tab_dispatch"),S.Settings&&S.SettingsTab==FMath::Min(Page-10,3));
            }
            else if(Page==18)
            {
                S.UnitIntroduction=true;const auto Cards=UnitIntroductionCards(B);
                S.UnitIntroductionPage=Cards.IndexOfByPredicate([](const FUnitIntroductionCard& Card){return Card.Role==EFrontlineRole::Officer;});
                Check(Key+TEXT("_narrative_card"),Cards.IsValidIndex(S.UnitIntroductionPage));
            }
            else if(Page==19){WarRoom52::Synchronize(Map,B);WarRoom52::BeginRoute(Map,B);Map.WasActive=true;WarRoom52::AtlasChooseSector(Map,B,1);WarRoom52::AtlasTick(Map,Map.FlightStart+Map.FlightDuration+.01);}
            else if(Page==24||Page==25){WarRoom52::Synchronize(Map,B);WarRoom52::BeginRoute(Map,B);Map.WasActive=true;if(Page==24){Map.AtlasMode=2;Map.Preview=8;Map.AtlasSector=3;Map.Centre=WarRoom52::UV(FrontlineCampaignV52::GetNode(B,8).LonLat);Map.Zoom=20.f;Map.FlightDuration=0.;Map.Detail=1.;Map.OperationSheet=true;Map.SheetStarted=Now;}else{Map.AtlasMode=0;Map.Zoom=1.f;Map.FlightDuration=0.;}}
            else if(Page==26)
            {S.AchievementsV76=true;S.AchievementFocusV76=Current.Focus;}
            else if(Page==27)
            {S.Dossier=0;S.AchievementToastV81=Current.Focus;S.AchievementToastAtV81=Now-.3;}
            else if(Page>=22)
            {
                B->BeginBattle();B->SetActorTickEnabled(false);FrontlineReviewUIV41::Clear(C);
                if(Page==23){NavigationChecks(C,B,H);FrontlineReviewUIV41::Clear(C);B->SetPaused(false);}
                Check(Key+TEXT("_production_soldier_fixture"),FrontlineCampaignV41::PopulateArtReviewFieldV73(B));
                FrontlineFilmV32::SetEnabled(Page==22,false);
                FrontlineReviewUIV41::Camera(C,0,0,6500,400,true);
                S.Performance=false;V28ReviewPointer=FVector2D(1880,400);
                Check(Key+TEXT("_field_has_no_menu_overlay"),!S.Menu&&!S.Settings&&!S.Technology&&!S.Deployment);
                if(ConsoleOnly&&Page==22)
                {
                    const auto Roles=FieldRolesV32(B);bool RoleDispatch=true,ArmourDispatch=true;
                    for(int32 I=0;I<Roles.Num()&&I<5;++I)if(B->IsRoleUnlocked(Roles[I]))
                    {S.AutoRoleV63=INDEX_NONE;V28ReviewPointer=FVector2D(FrontlineConsoleLayoutV79::InfantryX+I*FrontlineConsoleLayoutV79::CardPitch+1,DockTop+37);C->RightClickOrdersV63();RoleDispatch&=S.AutoRoleV63==int32(Roles[I]);}
                    if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
                    {const auto Roster=V->GetEquippedRoster();for(int32 I=0;I<Roster.Num()&&I<2;++I)
                    {S.AutoVehicleV63=INDEX_NONE;V28ReviewPointer=FVector2D(FrontlineConsoleLayoutV79::ArmourX+I*FrontlineConsoleLayoutV79::CardPitch+1,DockTop+37);C->RightClickOrdersV63();ArmourDispatch&=S.AutoVehicleV63==int32(Roster[I].Type);}}
                    Check(Key+TEXT("_repeat_order_matches_all_drawn_infantry_cards"),RoleDispatch);Check(Key+TEXT("_repeat_order_matches_drawn_armour_left_edges"),ArmourDispatch);
                    S.AutoRoleV63=S.AutoVehicleV63=INDEX_NONE;V28ReviewPointer=FVector2D(1880,400);
                }
                if((ConsoleOnly||Typography)&&Page==23){V28ReviewPointer=FVector2D(90,DockTop+90);S.AutoRoleV63=int32(EFrontlineRole::Rifle);}
                if(Page==28)
                {
                    FrontlineReviewUIV41::Camera(C,0,0,Current.Focus%2?9000:6500,650,true);
                    C->SetActorTickEnabled(true);
                    C->SupportTargetMode=SupportTargetMode(EFrontlineSupport::Artillery);C->bTargetingArtillery=true;
                    V28ReviewPointer=FVector2D(1090,560);V28ReviewAim=FVector(800,0,0);
                    B->SetPaused(Current.Focus>=2);
                }

            }
            else
            {
                B->BeginBattle();B->SetActorTickEnabled(false);S.Menu=true;S.bPausedByOverlay=true;B->SetPaused(true);
                FrontlineReviewUIV41::Camera(C,0,0,6500,400,true);
                Check(Key+TEXT("_battle_fixture"),B->Phase==EFrontlinePhase::Fighting);
                if(Page==21)NavigationChecks(C,B,H);
            }
            if(Page==19||Page==24||Page==25){C->SetActorTickEnabled(true);Check(Key+TEXT("_native_map_camera_entered"),FrontlineMapSceneV61::Update(C,true,Map.Centre,Map.Zoom,1.f,Map.AtlasSector,Map.Preview,Map.Detail));}
            FrontlineReviewUIV41::BeginAudit();Phase=1;
            const double ReadyAt=FPlatformTime::Seconds();
            SampleStart=ReadyAt+.6;LastSample=0;WallMS.Empty();GPUMS.Empty();GameMS.Empty();RenderMS.Empty();RHIMS.Empty();
            Next=ReadyAt+(Typography?(Page==24?3.f:2.f):((Page==24||Page==25)?5.f:Page==0||Page==19||Page>=20?2.8:1.25));return true;
        }
        if(Phase==1)
        {
            if(Page==19||Page==24||Page==25)Check(Key+TEXT("_native_map_camera_still_active"),FrontlineMapSceneV61::IsActive(C));
            if(Page==28)
            {
                Check(Key+TEXT("_full_ground_preview_survives_pause"),C->SupportTargetMode>0&&C->bPreviewValid&&UGameplayStatics::IsGamePaused(C)==(Current.Focus>=2));
                FVector2D At=FVector2D::ZeroVector,X=FVector2D::ZeroVector,Y=FVector2D::ZeroVector;const FVector Aim=C->ArtilleryPreview+FVector(0,0,60);const FVector2D Ext=B->FireSupportScatterExtent(EFrontlineSupport::Artillery);
                const bool Projected=C->ProjectWorldLocationToScreen(Aim,At)&&C->ProjectWorldLocationToScreen(Aim+FVector(Ext.X,0,0),X)&&C->ProjectWorldLocationToScreen(Aim+FVector(0,Ext.Y,0),Y);
                int32 VW=0,VH=0;C->GetViewportSize(VW,VH);const float UIScale=FMath::Max(.001f,FMath::Min(VW/1920.f,VH/1080.f));
                const float RX=FMath::Max(90.f,float(FMath::Abs(X.X-At.X)/UIScale)),RY=FMath::Max(37.f,float(FMath::Abs(Y.Y-At.Y)/UIScale));
                Check(Key+TEXT("_coverage_ellipse_exceeds_hardware_crosshair"),Projected&&RX*2.54f>=228.6f&&RY*2.54f>=93.98f);
            }
            if(S.PhotoTransitionV37){Next=Now+.2;return true;}
            FScreenshotRequest::RequestScreenshot(Dir/TEXT("captures")/(Key+TEXT(".png")),true,false);Phase=2;Next=Now+.4;return true;
        }
        const auto Layout=FrontlineReviewUIV41::EndAudit();Save(Key+TEXT(".json"),Layout);
        Check(Key+TEXT("_actual_layout"),Layout->GetBoolField(TEXT("success")));
        if(Page==22||Page==23)
        {
            // V82.16 reserves the centre for the hanging decoration. The accepted
            // rail has one right crest, not the retired pair of mirrored crests.
            const FString Crest=Artwork().ConsoleV79[Current.Faction][11]->GetPathName();
            bool WatchAbsent=true,RightCrest=false,MountFits=false;int32 CrestCount=0;
            for(const auto& Entry:Layout->GetArrayField(TEXT("images")))
            {
                const auto Image=Entry->AsObject();const auto Texture=Image->GetStringField(TEXT("texture"));
                const float X=Image->GetNumberField(TEXT("draw_x")),Y=Image->GetNumberField(TEXT("draw_y"));
                const float DrawW=Image->GetNumberField(TEXT("draw_width")),DrawH=Image->GetNumberField(TEXT("draw_height"));
                if(Texture.Contains(TEXT("_Terminal_V825"))&&Y<78.f)WatchAbsent=false;
                if(Texture==Crest&&Y<78.f)
                {
                    ++CrestCount;
                    RightCrest=X>=1023.99f&&X+DrawW<=1071.01f&&Y>=11.99f&&Y+DrawH<=59.01f
                        &&FMath::Abs(X+DrawW*.5f-1047.5f)<.01f&&FMath::Abs(Y+DrawH*.5f-35.5f)<.01f;
                }
            }
            // Nine-sliced chrome is deliberately absent from the photograph
            // audit. Its production interaction aperture is recorded instead.
            for(const auto& Entry:Layout->GetArrayField(TEXT("hitboxes")))
            {
                const auto Hit=Entry->AsObject();
                if(Hit->GetStringField(TEXT("command"))==TEXT("V8216MedalMount"))
                    MountFits=Hit->GetBoolField(TEXT("fits"))&&FMath::Abs(Hit->GetNumberField(TEXT("x"))-915.)<.01
                        &&FMath::Abs(Hit->GetNumberField(TEXT("y")))<.01&&FMath::Abs(Hit->GetNumberField(TEXT("width"))-90.)<.01
                        &&FMath::Abs(Hit->GetNumberField(TEXT("height"))-74.)<.01;
            }
            Check(Key+TEXT("_v8216_right_crest_and_central_medal_mount"),CrestCount==1&&RightCrest&&MountFits&&WatchAbsent);
        }
        const auto& Hits=Layout->GetArrayField(TEXT("hitboxes"));
        auto Has=[&](const TCHAR* Name)
        {for(const auto& Entry:Hits)if(Entry->AsObject()->GetStringField(TEXT("command"))==Name)return true;return false;};
        bool HitBounds=true;int32 Interactive=0;
        for(const auto& Entry:Hits){const auto J=Entry->AsObject();HitBounds&=J->GetBoolField(TEXT("fits"));if(J->GetStringField(TEXT("command"))!=TEXT("Blocked"))++Interactive;}
        Check(Key+TEXT("_real_hitboxes_fit"),HitBounds&&(Page==30?Interactive==0:Interactive>0));
        if(V81&&Page==26)
        {
            bool Inside=true;for(const auto& E:Hits){const auto J=E->AsObject();if(J->GetStringField(TEXT("command"))==TEXT("Blocked"))continue;
            Inside&=J->GetNumberField(TEXT("x"))>=64&&J->GetNumberField(TEXT("y"))>=56&&J->GetNumberField(TEXT("x"))+J->GetNumberField(TEXT("width"))<=1856&&J->GetNumberField(TEXT("y"))+J->GetNumberField(TEXT("height"))<=1024;}
            Check(Key+TEXT("_interactive_content_clears_case_inner_edges"),Inside);
        }
        if(V81&&Page==1)
        {
            // V82 uses four full-bleed panels, rather than the old inset case.
            // Check exact per-element tiling instead of weakening inset bounds.
            const auto L=FactionSelectionV82::Layout(S.FactionSelectionV82);
            auto Exact=[&](FName Name,const FactionSelectionV82::FRect& R)
            {for(const auto& E:Hits){const auto J=E->AsObject();if(FName(*J->GetStringField(TEXT("command")))==Name)
                return FMath::Abs(J->GetNumberField(TEXT("x"))-R.X)<.01&&FMath::Abs(J->GetNumberField(TEXT("y"))-R.Y)<.01&&FMath::Abs(J->GetNumberField(TEXT("width"))-R.W)<.01&&FMath::Abs(J->GetNumberField(TEXT("height"))-R.H)<.01;}return false;};
            const FName Armies[]={TEXT("FactionBritish"),TEXT("FactionFrenchV82"),TEXT("FactionGerman"),TEXT("FactionRussianV82")};
            bool ExactPanels=true;float X=0;
            for(int32 I=0;I<4;++I){ExactPanels&=Exact(Armies[I],L.Panels[I])&&L.Panels[I].X==X&&L.Panels[I].Y==112&&L.Panels[I].H==802;X+=L.Panels[I].W;}
            Check(Key+TEXT("_four_full_bleed_panels_match_exact_contiguous_art"),ExactPanels&&X==1920);
            bool Footer=Exact(TEXT("FactionBackV82"),L.Back);for(int32 I=0;I<3;++I)Footer&=Exact(FName(*FString::Printf(TEXT("Difficulty%d"),I)),L.Difficulty[I]);
            Check(Key+TEXT("_exact_footer_geometry_and_unselected_confirmation_absent"),Footer&&!Has(TEXT("FactionConfirmV82"))&&S.FactionSelectionV82.Selected==INDEX_NONE);
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV827ReleaseGateOnly")))
            {
                const int32 ArchivesBefore=WarfareFRRUCampaignV82::Archives(B).Num();
                Check(Key+TEXT("_britain_and_germany_remain_available"),FactionSelectionV82::Available(0)&&FactionSelectionV82::Available(2));
                Check(Key+TEXT("_france_and_russia_are_unavailable"),!FactionSelectionV82::Available(1)&&!FactionSelectionV82::Available(3));
                for(int32 I:{1,3})
                {
                    const auto Nation=I==1?EFRRUNationV81::French:EFRRUNationV81::Russian;
                    Check(Key+FString::Printf(TEXT("_army%d_public_start_refused"),I),!WarfareFRRUCampaignV82::Start(B,Nation,EFrontlineDifficulty::Rookie));
                    FactionSelectionV82::Select(C,I,true);
                    Check(Key+FString::Printf(TEXT("_army%d_pointer_selection_refused"),I),S.FactionSelectionV82.Selected==INDEX_NONE);
                }
                Check(Key+TEXT("_national_resume_refused"),!WarfareFRRUCampaignV82::Continue(B));
                FString Error;
                Check(Key+TEXT("_national_deployment_refused"),!WarfareFRRUCampaignV82::WorldReady(B)&&!WarfareFRRUWorldV82::CanPrepare(B)&&!WarfareFRRUWorldV82::Begin(B,Error)&&!Error.IsEmpty());
                Check(Key+TEXT("_national_archives_unchanged"),ArchivesBefore==WarfareFRRUCampaignV82::Archives(B).Num()&&!WarfareFRRUCampaignV82::Active(B));
                Check(Key+TEXT("_continue_ignores_unavailable_armies"),MostRecentNationalCampaignV827(B)==nullptr);
                S.FactionSelectionV82.Notice.Empty();
            }
        }
        if(ConsoleOnly&&(Page==3||Page==4))
        {
            bool OnPaintedBoard=true;int32 Cards=0;
            for(const auto& Entry:Hits)
            {
                const auto J=Entry->AsObject();const FString Command=J->GetStringField(TEXT("command"));
                if(!Command.StartsWith(TEXT("TechBuy"))&&!Command.StartsWith(TEXT("TechFocus")))continue;
                ++Cards;OnPaintedBoard&=J->GetNumberField(TEXT("x"))-5>=104&&J->GetNumberField(TEXT("y"))-5>=276
                    &&J->GetNumberField(TEXT("x"))+J->GetNumberField(TEXT("width"))+5<=1208
                    &&J->GetNumberField(TEXT("y"))+J->GetNumberField(TEXT("height"))+5<=904;
            }
            Check(Key+TEXT("_photo_frames_clear_physical_board_rails"),Cards>0&&OnPaintedBoard);
        }
        if((Page>=10&&Page<=17)||Page==21)
        {
            bool BackAtFooter=false;
            for(const auto& Entry:Hits){const auto J=Entry->AsObject();if(J->GetStringField(TEXT("command"))==TEXT("DisplayBack"))BackAtFooter=J->GetNumberField(TEXT("x"))==72&&J->GetNumberField(TEXT("y"))==958;}
            Check(Key+TEXT("_four_persistent_tabs_and_full_page_return"),Has(TEXT("SettingsVideo"))&&Has(TEXT("SettingsAudio"))&&Has(TEXT("SettingsImage"))&&Has(TEXT("SettingsHDR"))&&BackAtFooter);
            Check(Key+TEXT("_language_and_updates_remain_available"),Has(TEXT("LanguageEnglish"))&&Has(TEXT("LanguageChinese"))&&Has(TEXT("UpdateCheck"))&&Has(TEXT("UpdateOpen")));
            if(Page==10)Check(Key+TEXT("_display_controls_preserved"),Has(TEXT("WindowMode0"))&&Has(TEXT("WindowMode1"))&&Has(TEXT("WindowMode2"))&&Has(TEXT("FrameRateSlider"))&&Has(TEXT("Performance"))&&Has(TEXT("FilmGrain"))&&Has(TEXT("VideoApply")));
            if(Page==11){bool Audio=Has(TEXT("AudioMute"));for(int32 Bus=0;Bus<5;++Bus)Audio&=Has(*FString::Printf(TEXT("AudioDown%d"),Bus))&&Has(*FString::Printf(TEXT("AudioUp%d"),Bus))&&Has(*FString::Printf(TEXT("AudioLevel%d_0"),Bus))&&Has(*FString::Printf(TEXT("AudioLevel%d_10"),Bus));Check(Key+TEXT("_five_audio_buses_preserved"),Audio);}
        }
        if(Page>=14&&Page<=17)
        {
            bool Explanation=false;for(const auto& Entry:Layout->GetArrayField(TEXT("typography")))
            {const auto J=Entry->AsObject();Explanation|=J->GetStringField(TEXT("text")).Contains(TEXT("nit"))&&J->GetNumberField(TEXT("x"))==1296&&J->GetNumberField(TEXT("height"))==430&&!J->GetBoolField(TEXT("truncated"));}
            Check(Key+TEXT("_full_calibration_explanation_in_fixed_rail"),Explanation);
        }
        auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("page"),Key);P->SetStringField(TEXT("capture"),TEXT("captures/")+Key+TEXT(".png"));P->SetNumberField(TEXT("interactive_hitboxes"),Interactive);P->SetNumberField(TEXT("visible_text_boxes"),Layout->GetArrayField(TEXT("typography")).Num());Pages.Add(MakeShared<FJsonValueObject>(P));
        P->SetObjectField(TEXT("warmed_wall"),Timing(WallMS));P->SetObjectField(TEXT("warmed_gpu"),Timing(GPUMS));P->SetObjectField(TEXT("warmed_game_thread"),Timing(GameMS));P->SetObjectField(TEXT("warmed_render_thread"),Timing(RenderMS));P->SetObjectField(TEXT("warmed_rhi_thread"),Timing(RHIMS));
        P->SetStringField(TEXT("timing_scope"),TEXT("Short held production UI sample after 0.6s settling, before screenshot. RHI-delayed GPU cycles and per-thread timers; offscreen SDR Quality SR, FG off. Not combat FPS, pass timing, HDR or cross-device certification."));
        ++Stage;Phase=0;return true;
    }
};
void StartFrontlineUIV75Review(AWarfareFrontlineController* C)
{
    FString U;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV75UIReview"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),U)||U.IsEmpty())return;
    if(FParse::Param(FCommandLine::Get(),TEXT("WFUIMotionReviewV83"))){StartUIMotionReviewV83(C);return;}
    auto R=MakeShared<FFrontlineUIReviewV75>();R->PC=C;R->Started=FPlatformTime::Seconds();R->Next=R->Started+4;
    R->Dir=FPaths::ProjectSavedDir()/(FParse::Param(FCommandLine::Get(),TEXT("WFTypographyV8210"))?TEXT("Gameplay/V8210/UIReview"):FParse::Param(FCommandLine::Get(),TEXT("WFV81UIReview"))?TEXT("Gameplay/V81/UIReview"):FParse::Param(FCommandLine::Get(),TEXT("WFV80ConsoleOnly"))?TEXT("Gameplay/V80/UIReview"):TEXT("Gameplay/V75/UIReview"));IFileManager::Get().MakeDirectory(*(R->Dir/TEXT("captures")),true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}

