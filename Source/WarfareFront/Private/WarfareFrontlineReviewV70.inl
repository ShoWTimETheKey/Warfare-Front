// Focused production-page review, guarded by a disposable no-save profile.
struct FFrontlineReviewV70 : public TSharedFromThis<FFrontlineReviewV70>
{
    TWeakObjectPtr<AWarfareFrontlineController> PC;
    int32 Page=-1,Phase=0;double Started=0,Next=0,ArtRequestedAt=0;bool Good=true;
    FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;
    void Check(const FString& Name,bool Pass)
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V70_CHECK %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));}
    void Save(const FString& Name,const TSharedRef<FJsonObject>& J)
    {FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Dir/Name));}
    void Finish()
    {auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);J->SetStringField(TEXT("scope"),TEXT("Both campaign support rosters and branch/selection rules; actual 4K HUD, national support paintings, introduction/deployment names and layout. Isolated profile; no performance or all-campaign combat claim."));Save(TEXT("review.json"),J);FPlatformMisc::RequestExit(false);}
    bool Tick(float)
    {
        const double Now=FPlatformTime::Seconds();if(Now<Next)return true;
        auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;
        if(Now-Started>240){Check(TEXT("bounded_review"),false);Finish();return false;}
        if(!B||!B->LoadedSoldier||!C->GetHUD()){Next=Now+.25;return true;}
#if WITH_EDITOR
        if(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling())){Next=Now+.25;return true;}
#endif
        if(PipelineStateCache::GetNumActivePipelinePrecompileTasks()>0){Next=Now+.25;return true;}
        auto Report=[&](const FString& Name,bool Pass){Check(Name,Pass);};auto& S=ScreenState(C);auto& Map=WarRoom52::State(C);Next=Now+.15;
        if(Page<0)
        {
            if(ArtRequestedAt==0)
            {
                Check(TEXT("disposable_profile"),B->IsProgressionVolatile());
                FrontlineCampaignV32::RunSupportChecksV70(B,Report);
                FrontlineLocalizationV35::RunFormationNameChecksV70(Report);
                ArtRequestedAt=Now;
            }
            bool Ready=true,Separated=true;
            for(const auto Kind:FrontlineSupportCards)
            {
                auto UK=ResolveSupportArtworkV70(EFrontlineFaction::British,Kind),DE=ResolveSupportArtworkV70(EFrontlineFaction::German,Kind);
                Ready&=FrontlineTextureReady(UK.Texture)&&FrontlineTextureReady(DE.Texture);
                if(Kind==EFrontlineSupport::AntiTank||Kind==EFrontlineSupport::Mortar||Kind==EFrontlineSupport::Gas||Kind==EFrontlineSupport::AirStrike)
                    Separated&=UK.Texture!=DE.Texture&&DE.Texture&&DE.Texture->GetPathName().Contains(TEXT("/V70/"));
            }
            // LoadObject can precede creation of the render resource by a frame.
            // Check readiness after a bounded wait; missing assets still fail.
            if(!Ready&&Now-ArtRequestedAt<8){Next=Now+.25;return true;}
            Check(TEXT("all_eighteen_national_support_resolutions_are_resident"),Ready);
            Check(TEXT("national_gun_crew_mortar_gas_and_aircraft_art_is_distinct"),Separated);
            const auto Shrapnel=ResolveSupportArtworkV70(EFrontlineFaction::German,EFrontlineSupport::AirRecon);
            Check(TEXT("shrapnel_uses_current_art_not_retired_recon_atlas"),Shrapnel.Texture&&Shrapnel.Texture->GetName()==TEXT("T_Shrapnel_V64")&&Shrapnel.AtlasSlot==INDEX_NONE);
            Page=0;return true;
        }
        static const TCHAR* Names[]={TEXT("british-early-hud-en"),TEXT("german-early-hud-zh"),TEXT("british-late-hud-zh"),TEXT("german-late-hud-en"),TEXT("british-support-deployment-en"),TEXT("german-support-deployment-zh"),TEXT("shrapnel-introduction-en"),TEXT("german-antitank-introduction-zh"),TEXT("german-armour-deployment-zh"),TEXT("british-infantry-deployment-zh")};
        if(Page>=UE_ARRAY_COUNT(Names)){Finish();return false;}
        const FString Key=FString::Printf(TEXT("%02d-%s"),Page,Names[Page]);
        const bool German=Page==1||Page==3||Page==5||Page==7||Page==8;
        const bool Chinese=Page==1||Page==2||Page==5||Page==7||Page==8||Page==9;
        const int32 Operation=Page<2||Page==6?3:Page==7?8:Page==4||Page==5?17:19;
        if(Phase==0)
        {
            B->AbandonBattleToOperations();FrontlineReviewUIV41::Clear(C);
            S.Credits=false;S.ExitConfirmV61=false;S.Dossier=1917;
            FrontlineLocalizationV35::SetLanguage(Chinese?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
            Check(Key+TEXT("_fixture"),FrontlineCampaignV41::PrepareReviewOperation(B,German?EFrontlineFaction::German:EFrontlineFaction::British,Operation));
            B->SetActorTickEnabled(false);V28ReviewPointer=FVector2D(1890,810);
            if(Page<4)
            {
                B->BeginBattle();B->SetPaused(false);B->SetActorTickEnabled(false);
                Check(Key+TEXT("_fighting"),B->Phase==EFrontlinePhase::Fighting);
                FrontlineMapSceneV61::Update(C,false,Map.Centre,Map.Zoom,1,Map.AtlasSector,Map.Preview,Map.Detail);
                FrontlineReviewUIV41::Camera(C,0,0,6500,650,true);
                if(Page<2)
                {
                    Check(Key+TEXT("_visible_shrapnel_slot"),FieldSupportsV32(B).Contains(EFrontlineSupport::AirRecon));
                    B->Elapsed=1000;FrontlineSupportV32::ResetReadinessForDebug(B);
                    Cast<AWarfareFrontlineHUD>(C->GetHUD())->NotifyHitBoxClick(TEXT("SupportKind8"));
                    Check(Key+TEXT("_real_hud_selects_shrapnel_target"),C->SupportTargetMode==SupportTargetMode(EFrontlineSupport::AirRecon));
                    Check(Key+TEXT("_real_shrapnel_order_accepted"),B->RequestSupport(EFrontlineSupport::AirRecon,FVector(0,0,AWarfareFrontlineGround::HeightAt(0,0)),0));
                    C->CancelTargeting();
                }
            }
            else if(Page==6||Page==7)
            {
                S.UnitIntroduction=true;const auto Cards=UnitIntroductionCards(B);
                const auto Kind=Page==6?EFrontlineSupport::AirRecon:EFrontlineSupport::AntiTank;
                S.UnitIntroductionPage=Cards.IndexOfByPredicate([&](const FUnitIntroductionCard& Card){return Card.Support==Kind;});
                Check(Key+TEXT("_current_support_introduction_exists"),S.UnitIntroductionPage>=0);
                if(S.UnitIntroductionPage>=0)Check(Key+TEXT("_title_is_english"),!FFrontlineCanvas::ContainsCJKV45(FrontlineLocalizationV35::Text(Cards[S.UnitIntroductionPage].Title)));
            }
            else
            {
                S.Deployment=true;S.DeploymentTabV63=Page==8?1:0;
                V28ReviewPointer=Page==4||Page==5?FVector2D(900,630):FVector2D(320,365);
            }
            FrontlineReviewUIV41::BeginAudit();Phase=1;Next=Now+(Page<4?3.8:1.8);return true;
        }
        if(Phase==1)
        {
            if(S.PhotoTransitionV37){Next=Now+.2;return true;}
            FScreenshotRequest::RequestScreenshot(Dir/TEXT("captures")/(Key+TEXT(".png")),true,false);
            Phase=2;Next=Now+.4;return true;
        }
        const auto Layout=FrontlineReviewUIV41::EndAudit();Save(Key+TEXT(".json"),Layout);
        Check(Key+TEXT("_layout"),Layout->GetBoolField(TEXT("success")));
        if(Page<4)RunConsoleTypographyChecksV70(Layout,Report);
        ++Page;Phase=0;return true;
    }
};
void StartFrontlineV70Review(AWarfareFrontlineController* C)
{
    FString U;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV70Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),U)||U.IsEmpty())return;
    auto R=MakeShared<FFrontlineReviewV70>();R->PC=C;R->Started=FPlatformTime::Seconds();R->Next=R->Started+4;
    R->Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V70/Review");IFileManager::Get().MakeDirectory(*(R->Dir/TEXT("captures")),true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}
