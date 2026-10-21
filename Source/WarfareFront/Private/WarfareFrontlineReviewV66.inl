// Isolated bilingual UI review. It exercises production pages, not replacement widgets.
struct FFrontlineReviewV66 : public TSharedFromThis<FFrontlineReviewV66>
{
 TWeakObjectPtr<AWarfareFrontlineController> PC;
 int32 Page=-1,Language=0,Phase=0;double Started=0,Next=0;bool Good=true,DeploymentOnly=false;int32 DeploymentIndex=0;
 FString Dir;TArray<TSharedPtr<FJsonValue>> Checks,Pages;
 void Check(const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V66_CHECK %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));}
 void Save(const FString& File,const TSharedRef<FJsonObject>& J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Dir/File));}
 void Finish(){Check(TEXT("visible_translation_coverage"),FrontlineLocalizationV35::WriteCoverageReport());auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("pages"),Pages);J->SetStringField(TEXT("scope"),DeploymentOnly?TEXT("Six actual bilingual deployment views with infantry, support and armour hover prose. Whole-message translation and layout checks; disposable profile. No unrelated pages, lock render formats, performance or HDR claim."):TEXT("Bilingual production UI, localized prose/template aliases and SDR/linear-alpha lock coverage. Disposable profile; no balance, battle-performance or HDR presentation claim."));Save(TEXT("review.json"),J);FPlatformMisc::RequestExit(false);}
 bool Tick(float)
 {
  const double Now=FPlatformTime::Seconds();if(Now<Next)return true;auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;
  if(Now-Started>240){Check(TEXT("bounded_review"),false);Finish();return false;}
  if(!B||!B->LoadedSoldier||!C->GetHUD()){Next=Now+.2;return true;}
#if WITH_EDITOR
  if(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling())){Next=Now+.25;return true;}
#endif
  if(PipelineStateCache::GetNumActivePipelinePrecompileTasks()>0){Next=Now+.25;return true;}
  auto& S=ScreenState(C);auto& Map=WarRoom52::State(C);Next=Now+.15;
  if(Page<0)
  {
   Check(TEXT("disposable_profile"),B->IsProgressionVolatile());
   if(DeploymentOnly){Page=5;return true;}
   FrontlineLocalizationV35::RunCopyChecksV66([&](const FString& N,bool P){Check(N,P);});
   const auto Lock=FrontlineLockV39::RunCoverageChecks(B->GetWorld());Save(TEXT("lock-coverage.json"),Lock);Check(TEXT("opaque_lock_transparent_openings_both_ui_formats"),Lock->GetBoolField(TEXT("success")));
   Page=0;return true;
  }
  static const TCHAR* Names[]={TEXT("main-menu"),TEXT("campaign-selection"),TEXT("company-research"),TEXT("attachment-research"),TEXT("armour-research"),TEXT("deployment"),TEXT("armour-deployment"),TEXT("sector-map"),TEXT("video-settings"),TEXT("field-manual"),TEXT("locked-hud"),TEXT("battle-menu"),TEXT("campaign-return")};
  if(DeploymentOnly&&DeploymentIndex>=3){DeploymentIndex=0;Phase=0;++Language;if(Language==2){Finish();return false;}}
  if(DeploymentOnly)Page=DeploymentIndex==2?6:5;
  if(Page>=UE_ARRAY_COUNT(Names)){Page=0;Phase=0;++Language;if(Language==2){Finish();return false;}}
  const FString Key=FString::Printf(TEXT("%02d-%s-%s"),Page,Names[Page],Language?TEXT("zh"):TEXT("en"))+(DeploymentOnly?(DeploymentIndex==0?TEXT("-unit-hover"):DeploymentIndex==1?TEXT("-support-hover"):TEXT("-armour-hover")):TEXT(""));
  if(Phase==0)
  {
   FrontlineReviewUIV41::Clear(C);S.Credits=false;S.ExitConfirmV61=false;S.Dossier=1917;
   FrontlineLocalizationV35::SetLanguage(Language?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
   V28ReviewPointer=FVector2D(1890,790);
   if(DeploymentOnly&&DeploymentIndex==0)
   {
    Check(TEXT("deployment_volatile_campaign_")+FString::FromInt(Language),FrontlineCampaignV41::PrepareReviewOperation(B,Language?EFrontlineFaction::German:EFrontlineFaction::British,19));
    B->SetActorTickEnabled(false);
    auto Guide=[&](const FString& Name,const FString& Body)
    {
     const FString Expected=FrontlineLocalizationV35::Text(Body),Actual=FrontlineLocalizationV35::Text(DeploymentProseV69(Name,Body));
     return !Expected.IsEmpty()&&Actual.Contains(Expected)&&FFrontlineCanvas::ContainsCJKV45(Expected)==bool(Language);
    };
    bool Units=true,Supports=true,Armour=true;
    for(int32 I=0;I<int32(EFrontlineRole::Count);++I)if(FrontlineGameplayV52::SelectableRole(EFrontlineRole(I))&&FrontlineCampaignV41::RoleAllowed(B->GetPlayerFaction(),EFrontlineRole(I)))
     Units&=Guide(B->GetRecruitName(EFrontlineRole(I)),RoleTipV32(EFrontlineRole(I),B));
    for(const auto Kind:FrontlineSupportCards)Supports&=Guide(B->GetSupportName(Kind),B->GetSupportDescription(Kind));
    if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const auto& D:Vehicles->GetRoster())Armour&=Guide(D.Name,FrontlinePlayerGuideV43::Armour(D.Type));else Armour=false;
    Check(TEXT("whole_unit_guides_localize_before_composition_")+FString::FromInt(Language),Units);
    Check(TEXT("whole_support_guides_localize_before_composition_")+FString::FromInt(Language),Supports);
    Check(TEXT("whole_armour_guides_localize_before_composition_")+FString::FromInt(Language),Armour);
   }
   if(Page==0)
   {
    Check(TEXT("select_volatile_campaign_")+FString::FromInt(Language),FrontlineCampaignV41::PrepareReviewOperation(B,Language?EFrontlineFaction::German:EFrontlineFaction::British,0));
    B->SetActorTickEnabled(false);S.Dossier=0;
   }
   if(Page==1)S.Dossier=1918;
   if(Page>=2&&Page<=4)
   {S.Technology=true;S.TechnologyPage=Page-2;V28ReviewPointer=Page==4?FVector2D(1055,401):FVector2D(488,370);}
   if(Page==5||Page==6){S.Deployment=true;S.DeploymentTabV63=Page==6?1:0;if(DeploymentOnly)V28ReviewPointer=DeploymentIndex==1?FVector2D(300,630):FVector2D(320,365);}
   if(Page==7)
   {WarRoom52::Synchronize(Map,B);WarRoom52::BeginRoute(Map,B);Map.WasActive=true;WarRoom52::AtlasChooseSector(Map,B,1);WarRoom52::AtlasTick(Map,Map.FlightStart+Map.FlightDuration+.01);}
   if(Page==8){S.Settings=true;S.SettingsTab=0;LoadDisplayChoices(S);}
   if(Page==9)S.Help=true;
   if(Page==10)
   {
    B->BeginBattle();B->SetPaused(false);B->SetActorTickEnabled(false);
    FrontlineMapSceneV61::Update(C,false,Map.Centre,Map.Zoom,1,Map.AtlasSector,Map.Preview,Map.Detail);
    FrontlineReviewUIV41::Camera(C,0,0,6500,400,true);V28ReviewPointer=FVector2D(60,920);
    if(Language){FrontlineHUDV45::BeginAudit();V66HUDHoverIndex=1;}
   }
   if(Page==11)S.Menu=true;
   if(Page==12){B->AbandonBattleToOperations();S.CampaignEndingV51=true;}
   FrontlineReviewUIV41::BeginAudit();
   // Collect the existing engine glyph-loop bounds in this isolated HUD scope,
   // without starting another review or changing any font/layout/DPI settings.
   if(Page==10&&Language&&V66HUDModernAudit)V825FontAuditActive=true;
   Phase=1;Next=Now+(Page==0||Page==7||Page==10?3.5:1.2);return true;
  }
  if(Phase==1)
  {
   // A photo transition must finish visibly before a page capture is accepted.
   if(S.PhotoTransitionV37){Next=Now+.2;return true;}
   FScreenshotRequest::RequestScreenshot(Dir/TEXT("captures")/(Key+TEXT(".png")),true,false);
   Phase=2;Next=Now+.25;return true;
  }
  if(Phase==2&&Page==10&&Language)
  {
   // Sample another real explanation before checking CJK drawing and wrapping.
   V66HUDHoverIndex=2;V28ReviewPointer=FVector2D(1074,920);Phase=3;Next=Now+.75;return true;
  }
  const auto Layout=FrontlineReviewUIV41::EndAudit();Save(Key+TEXT(".json"),Layout);
  Check(Key+TEXT("_layout"),Layout->GetBoolField(TEXT("success")));auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("page"),Key);Item->SetNumberField(TEXT("visible_text_boxes"),Layout->GetArrayField(TEXT("typography")).Num());Pages.Add(MakeShared<FJsonValueObject>(Item));
  if(Page==10&&Language){const auto Hud=FrontlineHUDV45::EndAudit();Save(TEXT("hud-language-scope.json"),Hud);Check(TEXT("primary_hud_english_localized_tooltips"),Hud->GetBoolField(TEXT("success")));}
  if(DeploymentOnly)++DeploymentIndex;else ++Page;Phase=0;return true;
 }
};
void StartFrontlineV66Review(AWarfareFrontlineController* C)
{
 FString U;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV66Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),U)||U.IsEmpty())return;
 auto R=MakeShared<FFrontlineReviewV66>();R->PC=C;R->Started=FPlatformTime::Seconds();R->Next=R->Started+4;R->DeploymentOnly=FParse::Param(FCommandLine::Get(),TEXT("WFV69DeploymentReview"));R->Dir=FPaths::ProjectSavedDir()/(R->DeploymentOnly?TEXT("Gameplay/V69/DeploymentReview"):TEXT("Gameplay/V66/Review"));IFileManager::Get().MakeDirectory(*(R->Dir/TEXT("captures")),true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}
