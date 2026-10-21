namespace
{
void StartFrontlineV8222Review(AWarfareFrontlineController* Controller)
{
 if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8222Review")))return;
 FString User;if(!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{int32 Stage=0,Shots=0;bool Good=true;double Began=FPlatformTime::Seconds(),Next=Began+2.,Prepared=0;FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[R](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V8222_CHECK %s %d"),*Name,int32(Pass));};
  auto Finish=[&](){V28ReviewPointer.Reset();auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetStringField(TEXT("scope"),TEXT("V82.22 focused actual Shipping campaign-scoped cheats, natural award recovery, genuine infantry-armour deed exhibits and inspection, shared ordered ribbon racks, national emblems and foreground bilingual tooltips. Isolated fixtures; no real records, FR/RU campaign acceptance or whole-campaign/performance claim."));FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("checks.json")));FPlatformMisc::RequestExit(false);return false;};
  if(!B||!FrontlineEffectsReviewSafetyV8216::Allowed(B)){Check(TEXT("v8222_strict_disposable_world"),false);return Finish();}
  if(Now-R->Began>100.){Check(TEXT("v8222_bounded_review"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.Dossier=1917;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::View(PC).Picker=false;PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);};
  auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(FString(Name)+TEXT(".png")),true,false);++R->Shots;};
  auto Language=[&](bool ZH){FrontlineLocalizationV35::SetLanguage(ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);};
  auto Wall=[&](int32 Nation,bool ZH,bool Hover)
  {
   Clear();Language(ZH);S.AchievementsV76=true;auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Nation=Nation;
   const auto N=EFRRUNationV81(Nation);const int32 Id=MedalWallV8215::DeedBaseV8222+Nation;const auto Items=MedalWallV8215::Items(N);
   const int32 Position=Items.Find(Id);M.Row=FMath::Clamp(Position/3-1,0,FMath::Max(0,(Items.Num()+2)/3-2));
   MedalWallV8215::Hit(PC,FName(*FString::Printf(TEXT("V8216MedalSelect%d"),Id)));
   Check(FString::Printf(TEXT("v8222_nation_%d_deed_uses_regular_catalogue_selection"),Nation),Position!=INDEX_NONE&&M.Focus==Id);
   UIMotionReviewV83::TooltipAge=Hover?.6f:-1.f;
   if(Hover)
   {
    const auto Ribbons=MedalWallV8215::OrderedRibbonItems(N);const auto Cells=FrontlineRibbonRackV8222::Layout(Ribbons.Num(),126,508,949,64,true);
    const int32 Ribbon=Ribbons.Find(Id);Check(FString::Printf(TEXT("v8222_nation_%d_deed_hover_has_actual_ribbon_cell"),Nation),Cells.IsValidIndex(Ribbon));
    if(Cells.IsValidIndex(Ribbon))V28ReviewPointer=Cells[Ribbon].GetCenter();
   }
  };
  auto SeedEarnedDeed=[&](int32 Nation)
  {
   B->Phase=EFrontlinePhase::Briefing;const auto F=EFrontlineFaction(Nation);const auto N=EFRRUNationV81(Nation);const int32 Id=MedalWallV8215::DeedBaseV8222+Nation;
   Check(FString::Printf(TEXT("v8222_nation_%d_fresh_clean_deed_campaign"),Nation),B->StartNewCampaign(F,EFrontlineDifficulty::Veteran)&&!FrontlineDebugV37::HasCheatState(B));
   const int32 Before=FrontlineArmourKillTallyV8220::Count(N);const auto Historical=FrontlineMedalsV8215::Ribbons(N);
   const bool Seeded=FrontlineArmourKillTallyV8220::SeedNotificationReview(B);const auto Notice=FrontlineArmourKillTallyV8220::TakeNotification();
   Check(FString::Printf(TEXT("v8222_nation_%d_genuine_fatal_hull_record"),Nation),Seeded&&Notice.Nation==N&&Notice.Receipt.IsValid()&&Notice.Total==Before+1&&FrontlineArmourKillTallyV8220::Count(N)==Before+1);
   const auto Joined=MedalWallV8215::OrderedRibbonItems(N);bool PreservesOrder=Joined.Num()==Historical.Num()+1&&!Joined.IsEmpty()&&Joined.Last()==Id;
   for(int32 K=0;K<Historical.Num();++K)PreservesOrder&=Joined.IsValidIndex(K)&&Joined[K]==Historical[K];
   Check(FString::Printf(TEXT("v8222_nation_%d_deed_joins_existing_ribbon_precedence_once"),Nation),PreservesOrder);
   const auto Exhibits=MedalWallV8215::Exhibits(N);int32 DeedExhibits=0;for(const auto& E:Exhibits)DeedExhibits+=E.X==Id;
   Check(FString::Printf(TEXT("v8222_nation_%d_earned_deed_is_one_normal_exhibit"),Nation),MedalWallV8215::EntryTier(Id)==1&&DeedExhibits==1&&Exhibits.Contains(FIntPoint(Id,1))&&FrontlineTextureReady(MedalWallV8215::Art(Id,1)));
   auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Nation=Nation;M.Focus=Id;M.Inspect=false;
   MedalWallV8215::Hit(PC,FName(*FString::Printf(TEXT("V8217MedalExhibit%d_1"),Id)));
   Check(FString::Printf(TEXT("v8222_nation_%d_earned_exhibit_opens_regular_inspection"),Nation),M.Inspect&&M.Focus==Id&&M.PreviewGrade==1);
   MedalWallV8215::Hit(PC,TEXT("V8216MedalInspectBack"));
   Check(FString::Printf(TEXT("v8222_nation_%d_deed_inspection_returns_to_wall"),Nation),!M.Inspect&&M.Focus==Id);
   MedalWallV8215::Hit(PC,FName(*FString::Printf(TEXT("V8216MedalSelect%d"),MedalWallV8215::DeedBaseV8222+(Nation+1)%4)));
   Check(FString::Printf(TEXT("v8222_nation_%d_foreign_deed_cannot_cross_national_catalogue"),Nation),M.Focus==Id);
  };
  double Delay=.7;
  switch(R->Stage)
  {
   case 0:
   {
    for(int32 Nation=0;Nation<4;++Nation)
    {
     if(!FrontlineTextureReady(ArmourKillTallyUIV8220::Badge(EFRRUNationV81(Nation))))
     {if(Now-R->Began<35.){R->Next=Now+.1;return true;}Check(FString::Printf(TEXT("v8222_national_emblem_%d_rhi_ready"),Nation),false);return Finish();}
    }
    for(int32 Nation=0;Nation<4;++Nation)Check(FString::Printf(TEXT("v8222_national_emblem_%d_rhi_ready"),Nation),FrontlineTextureReady(ArmourKillTallyUIV8220::Badge(EFRRUNationV81(Nation))));
    Clear();B->Phase=EFrontlinePhase::Briefing;Check(TEXT("v8222_initial_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
    FrontlineRibbonRackV8222::RunChecks(Check);FrontlineArmourValourV8221::RunChecks(Check);
    bool FeltFits=true;for(int32 Count=1;Count<=10;++Count)for(const auto& Cell:FrontlineRibbonRackV8222::Layout(Count,126,508,949,64,true))FeltFits&=Cell.Min.X>=126&&Cell.Max.X<=1075.001&&Cell.Min.Y>=508&&Cell.Max.Y<=572.001;
    Check(TEXT("v8222_shared_wall_rack_inside_felt"),FeltFits);
    FrontlineAchievementsV76::RunCheatCampaignIsolationChecksV8222(B,Check);
    FrontlineAchievementsV76::RunCheatGrantChecksV8218(B,Check);
    Check(TEXT("v8222_maximum_natural_rack_fixture"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,true,3));
    for(int32 Nation=0;Nation<4;++Nation)
    {
     const auto N=EFRRUNationV81(Nation);const int32 Id=MedalWallV8215::DeedBaseV8222+Nation;const auto Items=MedalWallV8215::Items(N);const auto Ribbons=MedalWallV8215::OrderedRibbonItems(N);
     bool NoExhibit=true;for(const auto& E:MedalWallV8215::Exhibits(N))NoExhibit&=E.X!=Id;
     Check(FString::Printf(TEXT("v8222_nation_%d_zero_deed_has_no_artificial_exhibit_or_ribbon"),Nation),FrontlineArmourKillTallyV8220::Count(N)==0&&MedalWallV8215::EntryTier(Id)==0&&NoExhibit&&!Ribbons.Contains(Id));
     Check(FString::Printf(TEXT("v8222_nation_%d_deed_is_one_normal_catalogue_card"),Nation),Items.Contains(Id)&&Items.Num()==FrontlineMedalsV8215::NationIndices(N).Num()+1);
    }
    SeedEarnedDeed(0);
    break;
   }
   case 1:Wall(0,false,true);break;
   case 2:Shot(TEXT("01-british-rack-tooltip-en"));break;
   case 3:Wall(0,true,false);break;
   case 4:Shot(TEXT("02-british-rack-zh"));break;
   case 5:SeedEarnedDeed(1);Wall(1,false,true);break;
   case 6:Shot(TEXT("03-german-rack-tooltip-en"));break;
   case 7:Wall(1,true,false);MedalWallV8215::Hit(PC,TEXT("V8216MedalInspect"));Check(TEXT("v8222_deed_regular_detail_button_opens_inspection"),MedalWallV8215::View(PC).Inspect&&MedalWallV8215::View(PC).Focus==MedalWallV8215::DeedBaseV8222+1);break;
   case 8:Shot(TEXT("04-german-deed-inspect-zh"));break;
   case 9:Wall(2,true,false);break;
   case 10:Shot(TEXT("05-french-closed-emblem-zh"));break;
   case 11:Wall(3,false,false);break;
   case 12:Shot(TEXT("06-russian-closed-emblem-en"));break;
   case 13:Clear();Language(true);B->Phase=EFrontlinePhase::Briefing;Check(TEXT("v8222_assisted_archive_visual_fixture"),FrontlineAchievementsV76::SeedAssistedArchiveReviewV8222(B));Check(TEXT("v8222_clean_archive_after_assisted"),B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran));S.Dossier=1919;break;
   case 14:Shot(TEXT("07-assisted-and-clean-archives-zh"));break;
   case 15:Language(false);break;
   case 16:Shot(TEXT("08-assisted-and-clean-archives-en"));break;
   case 17:
   {
    Clear();Language(false);
    const auto Ribbons=MedalWallV8215::OrderedRibbonItems(EFRRUNationV81::German);
    const auto Cells=FrontlineRibbonRackV8222::Layout(Ribbons.Num(),1451,82,400,42,false);
    const int32 Deed=Ribbons.Find(MedalWallV8215::DeedBaseV8222+1);
    Check(TEXT("v8222_warroom_hover_uses_actual_shared_deed_ribbon_cell"),Cells.IsValidIndex(Deed));
    UIMotionReviewV83::TooltipAge=.6f;if(Cells.IsValidIndex(Deed))V28ReviewPointer=Cells[Deed].GetCenter();
    break;
   }
   case 18:Shot(TEXT("09-warroom-rack-tooltip-en"));break;
   case 19:
    Clear();Language(true);if(auto* C=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){C->CompletedMissions=19;C->WonOperationMaskV52=(1u<<19)-1;C->DispatchResolvedMaskV52=(1u<<20)-1;C->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;C->Research.ResearchPoints=60;}
    Check(TEXT("v8222_existing_operation_fixture"),B->SelectCampaignMission(16));S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;Delay=.1;break;
   case 20:
    if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<50.){R->Next=Now+.1;return true;}
    Check(TEXT("v8222_actual_prepared_battle"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty());
    B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);Clear();V28ReviewPointer=FVector2D(1855,24);UIMotionReviewV83::TooltipAge=.6;break;
   case 21:Shot(TEXT("10-field-flag-rack-zh"));break;
   case 22:Language(false);break;
   case 23:Shot(TEXT("11-field-flag-rack-en"));break;
   default:Clear();UIMotionReviewV83::TooltipAge=-1;return Finish();
  }
  ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
 }),0.f);
}
}
