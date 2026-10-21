// Bounded production-page acceptance. One disposable process renders the exact
// final page code and records text, image and actual AddHitBox geometry.
struct FFrontlineUIReviewV74 : public TSharedFromThis<FFrontlineUIReviewV74>
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
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V74_UI_CHECK %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));}
    void Save(const FString& Name,const TSharedRef<FJsonObject>& J)
    {FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Dir/Name));}
    void Finish()
    {
        Check(TEXT("bilingual_runtime_message_coverage"),FrontlineLocalizationV35::WriteCoverageReport());
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("pages"),Pages);
        J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        J->SetStringField(TEXT("scope"),TEXT("V74 authored archival counterparts and tactile production pages: paired British/German archival and colour research, all four full-screen settings tabs in EN/ZH, two long HDR explanations, layout/hitbox/navigation/pause checks and actual paused battle/settings navigation. Disposable profile. Manual visual review required; no HDR luminance, generated-frame temporal, performance, full-campaign or cross-hardware claim."));
        Save(TEXT("review.json"),J);FPlatformMisc::RequestExit(false);
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
    }
    bool Tick(float)
    {
        const double Now=FPlatformTime::Seconds();
        if(Phase==1&&Now>SampleStart&&Now<Next)
        {
            if(LastSample>0){const double MS=(Now-LastSample)*1000.;if(MS>0&&MS<250)WallMS.Add(MS);}
            LastSample=Now;
            auto Add=[](TArray<double>& A,double V){if(V>0&&V<250)A.Add(V);};
            Add(GPUMS,FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Add(GameMS,FPlatformTime::ToMilliseconds(GGameThreadTime));
            Add(RenderMS,FPlatformTime::ToMilliseconds(GRenderThreadTime));Add(RHIMS,FPlatformTime::ToMilliseconds(GRHIThreadTime));
        }
        if(Now<Next)return true;
        auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;auto* H=C?Cast<AWarfareFrontlineHUD>(C->GetHUD()):nullptr;
        if(Now-Started>240){Check(TEXT("bounded_ui_review"),false);Finish();return false;}
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
            Check(TEXT("v74_painting_and_archive_materials_loaded"),A.PaintingMaterialV73&&A.ArchivePhotoMaterialV72&&A.ArchivePhotoMaterialV72->GetName()==TEXT("M_ArchivePhoto_V74"));
            Check(TEXT("v73_native_identity_loaded"),A.BrandWordmarkV72&&A.BrandWordmarkV72->GetName()==TEXT("T_Brand_Wordmark_V73"));
            int32 Revised=0;for(const auto& Art:A.RolePaintingsV72[1])Revised+=Art.Texture&&Art.Texture->GetPathName().Contains(TEXT("/UI/V73/"));
            Check(TEXT("v74_ten_helmet_portraits_bound"),Revised==9&&A.RoleVariantsV72[1][1].Texture&&A.RoleVariantsV72[1][1].Texture->GetName()==TEXT("T_Portrait_German_Stormtrooper_V73"));
            int32 ArchiveCount=0;for(const auto& Nation:A.ArchivePaintingsV74)for(const auto& Card:Nation)ArchiveCount+=FrontlineTextureReady(Card.Texture)&&Card.Columns==2&&Card.Rows==2&&Card.Slot>=0&&Card.Slot<4;
            Check(TEXT("all_72_authored_archive_regions_loaded"),ArchiveCount==72);
            Check(TEXT("five_physical_chrome_assets_loaded"),FrontlineTextureReady(A.DeskV74)&&FrontlineTextureReady(A.FieldBoardV74)&&FrontlineTextureReady(A.OrderCardV74)&&FrontlineTextureReady(A.CommandKeyV74)&&FrontlineTextureReady(A.IndexTabV74));
            Check(TEXT("required_full_page_geometry"),PageCanvasV73::Margin==64&&PageCanvasV73::FooterY==936&&FrameRateSliderX==662&&FrameRateSliderWidth==450);
            Page=0;return true;
        }
        static const TCHAR* Names[]={TEXT("main-menu"),TEXT("campaign-selection"),TEXT("campaign-records"),TEXT("company-research"),TEXT("attachment-research"),TEXT("armour-research"),TEXT("infantry-deployment"),TEXT("armour-deployment"),TEXT("field-manual"),TEXT("credits"),TEXT("display-settings"),TEXT("audio-settings"),TEXT("graphics-settings"),TEXT("hdr-settings"),TEXT("hdr-peak-help"),TEXT("hdr-paper-help"),TEXT("hdr-ui-help"),TEXT("hdr-black-help"),TEXT("narrative-introduction"),TEXT("sector-map"),TEXT("paused-command-post"),TEXT("settings-from-pause"),TEXT("field-grain-on"),TEXT("field-grain-off")};
        struct FStage {int32 Page,Language,Faction;bool Research;};
        static const FStage Stages[]={{0,0,0,false},{0,1,1,false},{2,1,1,false},
            {3,0,0,false},{3,0,0,true},{3,1,1,false},{3,1,1,true},
            {4,0,0,false},{4,0,0,true},{4,1,1,false},{4,1,1,true},
            {5,0,0,false},{5,0,0,true},{5,1,1,false},{5,1,1,true},
            {6,0,0,true},{7,1,1,true},{8,1,1,false},{9,0,0,false},
            {10,0,0,false},{11,0,0,false},{12,0,0,false},{13,0,0,false},
            {10,1,1,false},{11,1,1,false},{12,1,1,false},{13,1,1,false},
            {15,0,0,false},{16,1,1,false},{20,1,1,true},{21,0,0,true}};
        if(Stage>=UE_ARRAY_COUNT(Stages)){Finish();return false;}
        const auto& Current=Stages[Stage];Page=Current.Page;Language=Current.Language;
        const FString Key=FString::Printf(TEXT("%02d-%s-%s-%s"),Stage,Names[Page],Language?TEXT("zh"):TEXT("en"),Current.Research?TEXT("colour"):TEXT("archive"));
        if(Phase==0)
        {
            B->AbandonBattleToOperations();FrontlineReviewUIV41::Clear(C);S.Credits=false;S.ExitConfirmV61=false;S.Dossier=1917;S.SettingsTab=0;
            FrontlineLocalizationV35::SetLanguage(Language?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
            const bool Fixture=FrontlineCampaignV41::PrepareArtReviewOperationV71(B,EFrontlineFaction(Current.Faction),Page>=22?5:Page==18?4:19,Current.Research);
            Check(Key+TEXT("_fixture"),Fixture);if(!Fixture){Finish();return false;}
            B->SetActorTickEnabled(false);V28ReviewPointer=FVector2D(1880,888);
            FrontlineMapSceneV61::Update(C,false,Map.Centre,Map.Zoom,1,Map.AtlasSector,Map.Preview,Map.Detail);
            if(Page==0){S.Dossier=0;C->ResetCamera();}
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
            else if(Page==8||Page==9){S.Help=true;S.Credits=Page==9;}
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
            else if(Page>=22)
            {
                B->BeginBattle();B->SetActorTickEnabled(false);FrontlineReviewUIV41::Clear(C);
                if(Page==23){NavigationChecks(C,B,H);FrontlineReviewUIV41::Clear(C);B->SetPaused(false);}
                Check(Key+TEXT("_production_soldier_fixture"),FrontlineCampaignV41::PopulateArtReviewFieldV73(B));
                FrontlineFilmV32::SetEnabled(Page==22,false);
                FrontlineReviewUIV41::Camera(C,0,0,6500,400,true);
                S.Performance=false;V28ReviewPointer=FVector2D(1880,400);
                Check(Key+TEXT("_field_has_no_menu_overlay"),!S.Menu&&!S.Settings&&!S.Technology&&!S.Deployment);
            }
            else
            {
                B->BeginBattle();B->SetActorTickEnabled(false);S.Menu=true;S.bPausedByOverlay=true;B->SetPaused(true);
                FrontlineReviewUIV41::Camera(C,0,0,6500,400,true);
                Check(Key+TEXT("_battle_fixture"),B->Phase==EFrontlinePhase::Fighting);
                if(Page==21)NavigationChecks(C,B,H);
            }
            FrontlineReviewUIV41::BeginAudit();Phase=1;SampleStart=Now+.6;LastSample=0;WallMS.Empty();GPUMS.Empty();GameMS.Empty();RenderMS.Empty();RHIMS.Empty();Next=Now+(Page==0||Page==19||Page==20?2.8:1.25);return true;
        }
        if(Phase==1)
        {
            if(S.PhotoTransitionV37){Next=Now+.2;return true;}
            FScreenshotRequest::RequestScreenshot(Dir/TEXT("captures")/(Key+TEXT(".png")),true,false);Phase=2;Next=Now+.4;return true;
        }
        const auto Layout=FrontlineReviewUIV41::EndAudit();Save(Key+TEXT(".json"),Layout);
        Check(Key+TEXT("_actual_layout"),Layout->GetBoolField(TEXT("success")));
        const auto& Hits=Layout->GetArrayField(TEXT("hitboxes"));
        auto Has=[&](const TCHAR* Name)
        {for(const auto& Entry:Hits)if(Entry->AsObject()->GetStringField(TEXT("command"))==Name)return true;return false;};
        bool HitBounds=true;int32 Interactive=0;
        for(const auto& Entry:Hits){const auto J=Entry->AsObject();HitBounds&=J->GetBoolField(TEXT("fits"));if(J->GetStringField(TEXT("command"))!=TEXT("Blocked"))++Interactive;}
        Check(Key+TEXT("_real_hitboxes_fit"),HitBounds&&Interactive>0);
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
            {const auto J=Entry->AsObject();Explanation|=J->GetStringField(TEXT("text")).Contains(TEXT("nit"))&&J->GetNumberField(TEXT("x"))==1296&&J->GetNumberField(TEXT("height"))==496&&!J->GetBoolField(TEXT("truncated"));}
            Check(Key+TEXT("_full_calibration_explanation_in_fixed_rail"),Explanation);
        }
        auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("page"),Key);P->SetStringField(TEXT("capture"),TEXT("captures/")+Key+TEXT(".png"));P->SetNumberField(TEXT("interactive_hitboxes"),Interactive);P->SetNumberField(TEXT("visible_text_boxes"),Layout->GetArrayField(TEXT("typography")).Num());Pages.Add(MakeShared<FJsonValueObject>(P));
        P->SetObjectField(TEXT("warmed_wall"),Timing(WallMS));P->SetObjectField(TEXT("warmed_gpu"),Timing(GPUMS));P->SetObjectField(TEXT("warmed_game_thread"),Timing(GameMS));P->SetObjectField(TEXT("warmed_render_thread"),Timing(RenderMS));P->SetObjectField(TEXT("warmed_rhi_thread"),Timing(RHIMS));
        P->SetStringField(TEXT("timing_scope"),TEXT("Short held production UI sample after 0.6s settling, before screenshot. RHI-delayed GPU cycles and per-thread timers; offscreen SDR Quality SR, FG off. Not combat FPS, pass timing, HDR or cross-device certification."));
        ++Stage;Phase=0;return true;
    }
};
void StartFrontlineUIV74Review(AWarfareFrontlineController* C)
{
    FString U;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV74Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),U)||U.IsEmpty())return;
    auto R=MakeShared<FFrontlineUIReviewV74>();R->PC=C;R->Started=FPlatformTime::Seconds();R->Next=R->Started+4;
    R->Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V74/Review");IFileManager::Get().MakeDirectory(*(R->Dir/TEXT("captures")),true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}
