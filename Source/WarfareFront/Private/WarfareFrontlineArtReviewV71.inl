// Actual production pages and region bindings only. Historical equipment and
// composition quality still require visual comparison with the reference set.
struct FFrontlineArtReviewV71 : public TSharedFromThis<FFrontlineArtReviewV71>
{
    TWeakObjectPtr<AWarfareFrontlineController> PC;
    int32 Page=-1,Phase=0;double Started=0,Next=0,RequestedAt=0;bool Good=true;
    FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;
    void Check(const FString& Name,bool Pass)
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V71_CHECK %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));}
    void Save(const FString& Name,const TSharedRef<FJsonObject>& J)
    {FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Dir/Name));}
    void Finish()
    {auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);J->SetStringField(TEXT("scope"),TEXT("V71 texture residency, unchanged saved identities, exact national region bindings and actual 4K menu/HUD/deployment/technology/introduction pages. No combat, performance or automatic historical-accuracy claim."));Save(TEXT("review.json"),J);FPlatformMisc::RequestExit(false);}
    bool Tick(float)
    {
        const double Now=FPlatformTime::Seconds();if(Now<Next)return true;
        auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;
        if(Now-Started>240){Check(TEXT("bounded_art_review"),false);Finish();return false;}
        if(!B||!B->LoadedSoldier||!C->GetHUD()){Next=Now+.25;return true;}
#if WITH_EDITOR
        if(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling())){Next=Now+.25;return true;}
#endif
        if(PipelineStateCache::GetNumActivePipelinePrecompileTasks()>0){Next=Now+.25;return true;}
        auto& S=ScreenState(C);auto& Map=WarRoom52::State(C);auto& A=Artwork();Next=Now+.15;
        auto IsNamed=[](const FFrontlineArtRegionV71& Region,const TCHAR* Name,int32 Slot=INDEX_NONE,int32 Columns=1,int32 Rows=1)
        {return Region.Texture&&Region.Texture->GetName()==Name&&Region.Slot==Slot&&Region.Columns==Columns&&Region.Rows==Rows;};
        if(Page<0)
        {
            if(RequestedAt==0){Check(TEXT("disposable_profile"),B->IsProgressionVolatile());RequestedAt=Now;}
            bool Ready=true,Regions=true;TSet<FString> Textures;TArray<TSharedPtr<FJsonValue>> Entries;
            auto Record=[&](const FString& Key,const FFrontlineArtRegionV71& Region)
            {
                if(!Region.Texture)return;
                Ready&=FrontlineTextureReady(Region.Texture);Textures.Add(Region.Texture->GetName());
                Regions&=Region.Texture->GetPathName().Contains(TEXT("/UI/V71/"))&&(Region.Slot==INDEX_NONE||(Region.Columns>0&&Region.Rows>0&&Region.Slot>=0&&Region.Slot<Region.Columns*Region.Rows));
                auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("binding"),Key);J->SetStringField(TEXT("texture"),Region.Texture->GetPathName());J->SetNumberField(TEXT("slot"),Region.Slot);J->SetNumberField(TEXT("columns"),Region.Columns);J->SetNumberField(TEXT("rows"),Region.Rows);J->SetNumberField(TEXT("width"),Region.Texture->GetSizeX());J->SetNumberField(TEXT("height"),Region.Texture->GetSizeY());Entries.Add(MakeShared<FJsonValueObject>(J));
            };
            for(int32 F=0;F<2;++F)
            {
                for(int32 I=0;I<int32(EFrontlineRole::Count);++I)Record(FString::Printf(TEXT("role_%d_%d"),F,I),A.RolePaintingsV71[F][I]);
                for(int32 I=0;I<3;++I)Record(FString::Printf(TEXT("variant_%d_%d"),F,I),A.RoleVariantsV71[F][I]);
                for(int32 I=0;I<28;++I)Record(FString::Printf(TEXT("technology_%d_%d"),F,I),A.TechnologyPaintingsV71[F][I]);
                for(int32 I=0;I<8;++I)Record(FString::Printf(TEXT("armour_technology_%d_%d"),F,I),A.ArmourTechnologyPaintingsV71[F][I]);
                for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)Record(FString::Printf(TEXT("support_%d_%d"),F,I),A.SupportPaintingsV71[F][I]);
                Record(FString::Printf(TEXT("menu_%d"),F),A.MenuCoversV71[F]);Record(FString::Printf(TEXT("background_%d"),F),A.BackgroundsV71[F]);
            }
            if(!Ready&&Now-RequestedAt<8){Next=Now+.25;return true;}
            Check(TEXT("twenty_eight_independent_v71_textures_are_resident"),Ready&&Textures.Num()==28&&Regions);
            auto Manifest=MakeShared<FJsonObject>();Manifest->SetArrayField(TEXT("bindings"),Entries);Manifest->SetNumberField(TEXT("unique_textures"),Textures.Num());Save(TEXT("bindings.json"),Manifest);
            const int32 TechIds[4][4]={{0,1,2,3},{4,5,7,8},{9,10,11,13},{14,15,24,26}};bool Technology=true;
            for(int32 Sheet=0;Sheet<4;++Sheet)for(int32 Cell=0;Cell<4;++Cell)
                Technology&=IsNamed(A.TechnologyPaintingsV71[1][TechIds[Sheet][Cell]],*FString::Printf(TEXT("T_Tech_German_%c_V71"),TCHAR('A'+Sheet)),Cell,2,2);
            Check(TEXT("four_german_technology_atlases_use_authored_cells"),Technology);
            const int32 ArmourIds[]={3,4,6,7};bool Armour=true;
            for(int32 Cell=0;Cell<4;++Cell)Armour&=IsNamed(ResolveArmourTechnologyArtworkV71(EFrontlineFaction::German,ArmourIds[Cell]),TEXT("T_ArmourTech_German_V71"),Cell,2,2);
            Check(TEXT("four_german_armour_technologies_use_authored_cells"),Armour);
            Check(TEXT("saved_role_and_support_identities_remain_stable"),int32(EFrontlineRole::AntiTankRifle)==8&&int32(EFrontlineRole::Count)==11&&int32(EFrontlineSupport::AirRecon)==8);
            Check(TEXT("technology_uses_same_commissioned_and_stormtrooper_portraits"),A.TechnologyPaintingsV71[1][6].Texture==A.RoleVariantsV71[1][2].Texture&&A.TechnologyPaintingsV71[1][12].Texture==A.RoleVariantsV71[1][1].Texture&&A.TechnologyPaintingsV71[0][9].Texture==A.RoleVariantsV71[0][2].Texture);
            Check(TEXT("german_shrapnel_has_its_own_national_painting"),ResolveSupportArtworkV70(EFrontlineFaction::German,EFrontlineSupport::AirRecon).Texture==A.SupportPaintingsV71[1][int32(EFrontlineSupport::AirRecon)].Texture.Get());
            Check(TEXT("unchanged_british_base_portrait_preserved"),!A.RolePaintingsV71[0][int32(EFrontlineRole::Rifle)].Texture&&A.BasicPortraits[0]&&A.BasicPortraits[0]->GetName()==TEXT("T_Portrait_British_Rifle"));
            Check(TEXT("unchanged_german_technology_twenty_five_preserved"),!A.TechnologyPaintingsV71[1][25].Texture);
            Page=0;return true;
        }
        static const TCHAR* Names[]={TEXT("german-menu"),TEXT("british-menu"),TEXT("german-early-hud"),TEXT("german-late-hud"),TEXT("german-base-deployment"),TEXT("german-upgraded-deployment"),TEXT("british-upgraded-deployment"),TEXT("german-company-technology"),TEXT("german-forward-technology"),TEXT("german-armour-technology"),TEXT("german-officer-introduction"),TEXT("german-flamethrower-introduction"),TEXT("german-antitank-support-introduction"),TEXT("british-officer-introduction")};
        if(Page>=UE_ARRAY_COUNT(Names)){Finish();return false;}
        const FString Key=FString::Printf(TEXT("%02d-%s"),Page,Names[Page]);
        const bool British=Page==1||Page==6||Page==13;
        const int32 Operation=Page==2?3:Page==10||Page==13?4:Page==11?6:Page==12?8:19;
        if(Phase==0)
        {
            B->AbandonBattleToOperations();FrontlineReviewUIV41::Clear(C);S.Credits=false;S.ExitConfirmV61=false;S.Dossier=1917;
            FrontlineLocalizationV35::SetLanguage(Page%2?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
            const bool Fixture=FrontlineCampaignV41::PrepareArtReviewOperationV71(B,British?EFrontlineFaction::British:EFrontlineFaction::German,Operation,Page!=2&&Page!=4);
            Check(Key+TEXT("_fixture"),Fixture);
            if(!Fixture||!B->GetCurrentCampaign()){Check(Key+TEXT("_campaign"),false);Finish();return false;}
            B->SetActorTickEnabled(false);V28ReviewPointer=FVector2D(1890,810);
            FrontlineMapSceneV61::Update(C,false,Map.Centre,Map.Zoom,1,Map.AtlasSector,Map.Preview,Map.Detail);
            if(Page<2)
            {
                S.Dossier=0;C->ResetCamera();
                Check(Key+TEXT("_cover"),MenuCoverV63(B)&&MenuCoverV63(B)->GetName()==(British?TEXT("T_MainMenu_V62"):TEXT("T_MainMenu_German_V71")));
            }
            else if(Page==2||Page==3)
            {
                B->BeginBattle();B->SetPaused(false);B->SetActorTickEnabled(false);FrontlineReviewUIV41::Camera(C,0,0,6500,650,true);
                Check(Key+TEXT("_fighting"),B->Phase==EFrontlinePhase::Fighting);
            }
            else if(Page>=7&&Page<=9){S.Technology=true;S.TechnologyPage=Page-7;S.TechnologyFocus=INDEX_NONE;}
            else if(Page>=10)
            {
                S.UnitIntroduction=true;const auto Cards=UnitIntroductionCards(B);const auto Role=Page==11?EFrontlineRole::Flamethrower:EFrontlineRole::Officer;
                S.UnitIntroductionPage=Cards.IndexOfByPredicate([&](const FUnitIntroductionCard& Card){return Page==12?Card.Support==EFrontlineSupport::AntiTank:Card.Role==Role;});
                Check(Key+TEXT("_introduction_exists"),S.UnitIntroductionPage>=0);
                if(S.UnitIntroductionPage>=0&&Page!=12)
                    Check(Key+TEXT("_same_role_binding"),Cards[S.UnitIntroductionPage].Portrait==ResolveRoleArtworkV71(B,Role).Texture.Get());
            }
            else {S.Deployment=true;S.DeploymentTabV63=0;}
            FrontlineReviewUIV41::BeginAudit();Phase=1;Next=Now+(Page==2||Page==3?3.8:1.8);return true;
        }
        if(Phase==1)
        {
            if(S.PhotoTransitionV37){Next=Now+.2;return true;}
            FScreenshotRequest::RequestScreenshot(Dir/TEXT("captures")/(Key+TEXT(".png")),true,false);Phase=2;Next=Now+.4;return true;
        }
        const auto Layout=FrontlineReviewUIV41::EndAudit();Save(Key+TEXT(".json"),Layout);Check(Key+TEXT("_layout"),Layout->GetBoolField(TEXT("success")));
        if(Page!=1)
        {
            bool NewArtDrawn=false;for(const auto& Entry:Layout->GetArrayField(TEXT("images")))
                NewArtDrawn|=Entry->AsObject()->GetStringField(TEXT("texture")).Contains(TEXT("/UI/V71/"));
            Check(Key+TEXT("_actual_page_draws_v71_art"),NewArtDrawn);
        }
        if(Page==2||Page==3)RunConsoleTypographyChecksV70(Layout,[&](const FString& Name,bool Pass){Check(Name,Pass);});
        ++Page;Phase=0;return true;
    }
};
void StartFrontlineArtV71Review(AWarfareFrontlineController* C)
{
    FString U;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV71Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),U)||U.IsEmpty())return;
    auto R=MakeShared<FFrontlineArtReviewV71>();R->PC=C;R->Started=FPlatformTime::Seconds();R->Next=R->Started+4;
    R->Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V71/Review");IFileManager::Get().MakeDirectory(*(R->Dir/TEXT("captures")),true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}
