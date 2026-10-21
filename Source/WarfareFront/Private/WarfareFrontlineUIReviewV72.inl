// Focused actual-page acceptance for V72 identity, small portrait readability,
// archival technology states, region guards and bilingual field notes.
struct FFrontlineUIReviewV72 : public TSharedFromThis<FFrontlineUIReviewV72>
{
    TWeakObjectPtr<AWarfareFrontlineController> PC;
    int32 Page=-1,Phase=0;double Started=0,Next=0;bool Good=true;
    FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;
    void Check(const FString& Name,bool Pass)
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V72_UI_CHECK %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));}
    void Save(const FString& Name,const TSharedRef<FJsonObject>& J)
    {FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Dir/Name));}
    void Finish()
    {
        Check(TEXT("bilingual_runtime_message_coverage"),FrontlineLocalizationV35::WriteCoverageReport());
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);
        J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        J->SetStringField(TEXT("scope"),TEXT("V72 real 4K menu/credits/deployment/technology/introduction UI; archival/color/reveal states; no world render or performance claim."));
        Save(TEXT("review.json"),J);FPlatformMisc::RequestExit(false);
    }
    bool Tick(float)
    {
        const double Now=FPlatformTime::Seconds();if(Now<Next)return true;
        auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;
        if(Now-Started>240){Check(TEXT("bounded_ui_review"),false);Finish();return false;}
        if(!B||!B->LoadedSoldier||!C->GetHUD()){Next=Now+.25;return true;}
#if WITH_EDITOR
        if(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling())){Next=Now+.25;return true;}
#endif
        if(PipelineStateCache::GetNumActivePipelinePrecompileTasks()>0){Next=Now+.25;return true;}
        auto& S=ScreenState(C);auto& A=Artwork();auto& Map=WarRoom52::State(C);Next=Now+.15;
        if(Page<0)
        {
            Check(TEXT("disposable_profile"),B->IsProgressionVolatile());
            Check(TEXT("wordmark_emblem_and_two_national_covers_resident"),FrontlineTextureReady(A.BrandWordmarkV72)&&FrontlineTextureReady(A.BrandEmblemV72)&&FrontlineTextureReady(A.MenuCoversV72[0].Texture)&&FrontlineTextureReady(A.MenuCoversV72[1].Texture));
            Check(TEXT("archival_canvas_material_loaded"),A.ArchivePhotoMaterialV72!=nullptr);
            Check(TEXT("saved_role_and_support_identities_unchanged"),int32(EFrontlineRole::AntiTankRifle)==8&&int32(EFrontlineRole::Count)==11&&int32(EFrontlineSupport::AirRecon)==8);
            FrontlineLocalizationV35::RunCopyChecksV66([&](const FString& N,bool P){Check(N,P);});
            FrontlineLocalizationV35::RunFormationNameChecksV70([&](const FString& N,bool P){Check(N,P);});
            Page=0;return true;
        }
        static const TCHAR* Names[]={TEXT("british-menu"),TEXT("german-menu"),TEXT("english-credits"),TEXT("chinese-credits"),TEXT("german-archival-research"),TEXT("german-colour-research"),TEXT("british-archival-research"),TEXT("german-armour-archival"),TEXT("german-armour-mixed"),TEXT("german-deployment"),TEXT("british-deployment"),TEXT("german-narrative-introduction"),TEXT("research-reveal-start"),TEXT("research-reveal-finish")};
        if(Page>=UE_ARRAY_COUNT(Names)){Finish();return false;}
        const FString Key=FString::Printf(TEXT("%02d-%s"),Page,Names[Page]);
        const bool British=Page==0||Page==2||Page==6||Page==10;
        if(Phase==0)
        {
            if(Page!=13)
            {
                B->AbandonBattleToOperations();FrontlineReviewUIV41::Clear(C);S.Credits=false;S.ExitConfirmV61=false;S.Dossier=1917;
                FrontlineLocalizationV35::SetLanguage(Page==3||Page==6||Page==10?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
                const bool Fixture=FrontlineCampaignV41::PrepareArtReviewOperationV71(B,British?EFrontlineFaction::British:EFrontlineFaction::German,Page==11?4:19,Page==5||Page==9||Page==10||Page==11);
                Check(Key+TEXT("_fixture"),Fixture);
                if(!Fixture){Finish();return false;}
                B->SetActorTickEnabled(false);V28ReviewPointer=FVector2D(1880,802);
                FrontlineMapSceneV61::Update(C,false,Map.Centre,Map.Zoom,1,Map.AtlasSector,Map.Preview,Map.Detail);
            }
            if(Page<2){S.Dossier=0;C->ResetCamera();Check(Key+TEXT("_cover"),MenuCoverV63(B)==A.MenuCoversV72[British?0:1].Texture.Get());}
            else if(Page<4){S.Help=S.Credits=true;}
            else if(Page<=8||Page>=12)
            {
                S.Technology=true;S.TechnologyPage=Page==7||Page==8?2:0;S.TechnologyFocus=INDEX_NONE;
                if(Page==8)for(int32 Id:{0,1,2,3,4})Check(FString::Printf(TEXT("armour_research_%d"),Id),FrontlineArmourV63::BuyResearch(B,Id));
                if(Page>=12)
                {
                    if(Page==12)
                    {
                        const auto* R=B->GetCurrentCampaign();S.ResearchCampaignV72=R?R->Id:FGuid();S.ResearchRevealsV72.Empty();
                        Check(TEXT("unresearched_card_remains_archive"),ResearchColourV72(B,0,false)==0.f);
                        Check(TEXT("focused_research_purchase"),B->PurchaseTechnology(0));
                        S.ResearchRevealsV72.Add(ResearchCardKeyV72(B,0,false),Now+.65);
                    }
                    else Check(TEXT("purchased_card_reaches_colour"),ResearchColourV72(B,0,true)>.99f);
                }
            }
            else if(Page==9||Page==10)
            {
                S.Deployment=true;S.DeploymentTabV63=0;
                Check(Key+TEXT("_compact_officer_not_narrative"),ResolveRoleArtworkV71(B,EFrontlineRole::Officer).Texture!=ResolveRoleArtworkV71(B,EFrontlineRole::Officer,EFrontlinePortraitContextV72::Narrative).Texture);
            }
            else
            {
                S.UnitIntroduction=true;const auto Cards=UnitIntroductionCards(B);
                S.UnitIntroductionPage=Cards.IndexOfByPredicate([](const FUnitIntroductionCard& Card){return Card.Role==EFrontlineRole::Officer;});
                Check(TEXT("narrative_officer_introduction_exists"),Cards.IsValidIndex(S.UnitIntroductionPage));
                if(Cards.IsValidIndex(S.UnitIntroductionPage))Check(TEXT("introduction_preserves_v71_scene"),Cards[S.UnitIntroductionPage].Portrait==ResolveRoleArtworkV71(B,EFrontlineRole::Officer,EFrontlinePortraitContextV72::Narrative).Texture.Get());
            }
            FrontlineReviewUIV41::BeginAudit();Phase=1;Next=Now+(Page==12?.1:Page==13?1.2:1.8);return true;
        }
        if(Phase==1)
        {
            if(S.PhotoTransitionV37){Next=Now+.2;return true;}
            if(Page==12)S.ResearchRevealsV72.Add(ResearchCardKeyV72(B,0,false),Now-.15);
            FScreenshotRequest::RequestScreenshot(Dir/TEXT("captures")/(Key+TEXT(".png")),true,false);Phase=2;Next=Now+.4;return true;
        }
        const auto Layout=FrontlineReviewUIV41::EndAudit();Save(Key+TEXT(".json"),Layout);Check(Key+TEXT("_layout"),Layout->GetBoolField(TEXT("success")));
        bool BrandDrawn=false;for(const auto& Entry:Layout->GetArrayField(TEXT("images")))BrandDrawn|=Entry->AsObject()->GetStringField(TEXT("texture")).Contains(TEXT("T_Brand_Wordmark_V72"));
        if(Page<4)Check(Key+TEXT("_shared_wordmark_drawn"),BrandDrawn);
        bool BadSlogan=false;for(const auto& Entry:Layout->GetArrayField(TEXT("typography")))
        {
            FString Value;Entry->AsObject()->TryGetStringField(TEXT("text"),Value);
            BadSlogan|=Value.Contains(TEXT("THE WESTERN FRONT"))||Value.Contains(TEXT("FRANCE & FLANDERS"))||Value.Contains(TEXT("先完成前置操练"));
        }
        Check(Key+TEXT("_no_removed_slogans"),!BadSlogan);
        ++Page;Phase=0;return true;
    }
};
void StartFrontlineUIV72Review(AWarfareFrontlineController* C)
{
    FString U;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV72Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),U)||U.IsEmpty())return;
    auto R=MakeShared<FFrontlineUIReviewV72>();R->PC=C;R->Started=FPlatformTime::Seconds();R->Next=R->Started+4;
    R->Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V72/Review");IFileManager::Get().MakeDirectory(*(R->Dir/TEXT("captures")),true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}
