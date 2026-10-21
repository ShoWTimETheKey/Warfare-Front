// Focused production-UI review. It never prepares an operation or runs an
// encounter; all writable account fixtures use the exact disposable UserDir.
namespace
{
void StartRibbonReviewV8443(AWarfareFrontlineController* Controller)
{
 FString User;
 if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8443RibbonReview"))
  ||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun
 {
  FString Dir;int32 Stage=0,Page=0,Shots=0;bool Good=true;
  double Started=FPlatformTime::Seconds(),Next=Started+1;uint64 BoundFrame=0;
  TArray<TSharedPtr<FJsonValue>> Checks,Captures;
  TSharedPtr<FJsonObject> InitialState,ReadyState;
 };
 auto R=MakeShared<FRun>();R->Dir=User;
 IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto DescribeInitial=[&]()
  {
   auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("wall_seconds"),Now-R->Started);
   J->SetBoolField(TEXT("controller_exists"),PC!=nullptr);J->SetBoolField(TEXT("game_mode_exists"),B!=nullptr);
   J->SetBoolField(TEXT("controller_begin_play_finished"),PC&&PC->HasActorBegunPlay());
   J->SetBoolField(TEXT("game_mode_begin_play_finished"),B&&B->HasActorBegunPlay());
   J->SetBoolField(TEXT("progression_volatile"),B&&B->IsProgressionVolatile());
   J->SetNumberField(TEXT("unit_count"),B?B->Units.Num():INDEX_NONE);J->SetNumberField(TEXT("phase"),B?int32(B->Phase):INDEX_NONE);
   J->SetBoolField(TEXT("fr_ru_campaign_active"),B&&WarfareFRRUCampaignV82::Active(B));
   J->SetBoolField(TEXT("no_save_flag"),FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")));
   J->SetBoolField(TEXT("account_review_flag"),FParse::Param(FCommandLine::Get(),TEXT("WFV76Review")));
   FString Current;const bool Allowed=FrontlineUltimateArchiveV8215::ReviewUserDir(Current)
    &&!Current.IsEmpty()&&!FPaths::IsRelative(Current)&&FPaths::IsSamePath(Current,R->Dir);
   J->SetBoolField(TEXT("absolute_disposable_user_dir_still_valid"),Allowed);return J;
  };
  if(R->Stage==0&&B){if(!R->InitialState.IsValid())R->InitialState=DescribeInitial();R->ReadyState=DescribeInitial();}
  auto Check=[&](const FString& Name,bool Pass)
  {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;};
  auto Finish=[&]()
  {
   auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);
   J->SetArrayField(TEXT("captures"),R->Captures);J->SetNumberField(TEXT("captures_requested"),R->Shots);
   J->SetNumberField(TEXT("wall_seconds"),Now-R->Started);
   if(R->InitialState.IsValid())J->SetObjectField(TEXT("initial_state"),R->InitialState);
   if(R->ReadyState.IsValid())J->SetObjectField(TEXT("ready_state"),R->ReadyState);
   J->SetStringField(TEXT("scope"),TEXT("Synthetic disposable accounts, ribbon promotion/atlas and mounted-medal geometry checks, actual 3840x2160 medal-wall/war-room and frozen empty Fighting HUD rendering. No operation preparation, natural award earning, campaign acceptance or desktop DPI/HDR claim."));
   FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Dir/TEXT("checks.json")));
   FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;
  };
  if(!PC||!B||!PC->GetHUD()){if(Now-R->Started<30)return true;Check(TEXT("v8443_ui_world_ready"),false);return Finish();}
  if(Now-R->Started>90){Check(TEXT("v8443_bounded_ui_review"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&]()
  {
   S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
   S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;
   S.Dossier=1917;S.FactionSelectionV82=FFrontlineFactionSelectionV82();
   MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;
   MedalMountV8216::CloseV836(PC);UltimateArchiveUIV8215::Close(PC);V28ReviewPointer=FVector2D(-10000,-10000);
   PC->CancelTargeting();UGameplayStatics::SetGamePaused(PC,false);
  };
  auto Safe=[&](){return B->IsProgressionVolatile()&&B->Units.IsEmpty()&&B->Phase==EFrontlinePhase::Briefing&&!WarfareFRRUCampaignV82::Active(B);};
  if(R->Stage==0)
  {
   FString CurrentUser;
   const bool Context=FParse::Param(FCommandLine::Get(),TEXT("WFV8443RibbonReview"))
    &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))
    &&FrontlineUltimateArchiveV8215::ReviewUserDir(CurrentUser)&&!CurrentUser.IsEmpty()&&!FPaths::IsRelative(CurrentUser)
    &&FPaths::IsSamePath(CurrentUser,R->Dir)&&B->GetWorld()&&B->GetWorld()->IsGameWorld();
   if(!Context){Check(TEXT("v8443_initial_exact_absolute_disposable_context"),false);return Finish();}
   const bool Ready=PC->HasActorBegunPlay()&&B->HasActorBegunPlay()&&B->IsProgressionVolatile();
   if(!Ready)
   {
    if(Now-R->Started<30){R->Next=Now+.1;return true;}
    Check(TEXT("v8443_initial_begin_play_and_volatile_context_ready_within_bound"),false);return Finish();
   }
   Check(TEXT("v8443_initial_exact_absolute_disposable_context"),Context);
   Check(TEXT("v8443_initial_begin_play_and_volatile_context_ready_within_bound"),Ready);
   Check(TEXT("v8443_initial_fr_ru_campaign_not_active"),!WarfareFRRUCampaignV82::Active(B));if(!R->Good)return Finish();
   // The normal BeginPlay has now completed in the exact temporary profile.
   // Establish the public empty operations state here, rather than assuming
   // an earlier review hook left the world in that state. No real profile can
   // pass the context/volatile checks above; the seed retains its own guards.
   if(!Safe())B->AbandonBattleToOperations();
   R->ReadyState=DescribeInitial();
   Check(TEXT("v8443_empty_volatile_briefing_only"),Safe());if(!R->Good)return Finish();
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
   {Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
   Clear();Check(TEXT("v8443_disposable_german_operations_record"),B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran));
   Check(TEXT("v8443_operations_record_does_not_spawn_a_battle"),Safe());if(!R->Good)return Finish();
   FrontlineMedalsV8215::RunCatalogueChecks(Check);
   MedalMountV8216::RunGeometryChecksV8443(Check);
   bool ClassProjection=true;
   for(int32 I=0;I<FrontlineMedalsV8215::Count;++I)
   {
    int32 Last=0;for(int32 G=1;G<=FrontlineMedalsV8215::Variants(I).Num();++G)
    {const int32 Chest=FrontlineMedalsV8215::RibbonGradeForHighestClassV8443(I,G);ClassProjection&=Chest>=0&&Chest<=G&&(Last==0||Chest>0);Last=Chest;}
   }
   Check(TEXT("v8443_class_projection_never_loses_an_earned_ribbon"),ClassProjection);
   Check(TEXT("v8443_pin_neck_only_catalogues_do_not_gain_fictional_ribbons"),FrontlineMedalsV8215::RibbonGradeForHighestClassV8443(5,1)==0
    &&FrontlineMedalsV8215::RibbonGradeForHighestClassV8443(34,4)==0&&FrontlineMedalsV8215::RibbonGradeForHighestClassV8443(35,4)==0
    &&FrontlineMedalsV8215::RibbonGradeForHighestClassV8443(36,1)==0);
   Check(TEXT("v8443_anna_weapon_badge_needs_the_actual_chest_class"),FrontlineMedalsV8215::RibbonGradeForHighestClassV8443(31,1)==0
    &&FrontlineMedalsV8215::RibbonGradeForHighestClassV8443(31,2)==2&&FrontlineMedalsV8215::RibbonGradeForHighestClassV8443(31,3)==2);
   TArray<int32> Before;
   for(int32 Step=0;Step<3;++Step)
   {
    Check(FString::Printf(TEXT("v8443_german_stage%d_persists_and_reloads_disposable_account"),Step),FrontlineAchievementsV76::SeedRibbonReviewV8443(B,EFRRUNationV81::German,Step));
    const auto A=MedalWallV8215::OrderedRibbonItems(EFRRUNationV81::German);
    Check(FString::Printf(TEXT("v8443_german_stage%d_expected_ribbon_count"),Step),A.Num()==(Step==0?6:7));
    if(Step==1){bool Retained=true;for(int32 I:Before)Retained&=A.Contains(I);Check(TEXT("v8443_seventh_ribbon_retains_all_previous_ids"),Retained);}
    if(Step==2)
    {
     Check(TEXT("v8443_promotion_preserves_seven_ribbon_ids_and_precedence"),A==Before);
     Check(TEXT("v8443_actual_highest_classes_survive_fixture_reload"),FrontlineMedalsV8215::Tier(3)==3&&FrontlineMedalsV8215::Tier(18)==3
      &&FrontlineMedalsV8215::Tier(20)==6&&FrontlineMedalsV8215::Tier(21)==3&&FrontlineMedalsV8215::Tier(22)==4);
    }
    Before=A;
   }
   for(int32 N=0;N<4;++N)
   {
    const auto Nation=EFRRUNationV81(N);Check(FString::Printf(TEXT("v8443_nation%d_transient_full_catalogue_fixture"),N),FrontlineAchievementsV76::SeedRibbonReviewV8443(B,Nation,3));
    const auto Indices=FrontlineMedalsV8215::NationIndices(Nation);bool Slots=Indices.Num()==9;
    for(int32 I:Indices)
    {
     const int32 Slot=Indices.Find(I);const auto Cut=MedalWallV8215::RibbonTextileRectV8218(N,Slot);
     const int32 OX=(Slot%3)*724,OY=(Slot/3)*241;
     Slots&=Slot>=0&&Slot<9&&Cut.Width()>0&&Cut.Height()>0&&OX+Cut.Min.X>=0&&OY+Cut.Min.Y>=0&&OX+Cut.Max.X<=2172&&OY+Cut.Max.Y<=724;
    }
    for(int32 I:MedalWallV8215::OrderedRibbonItems(Nation))Slots&=Indices.Contains(I);
    Check(FString::Printf(TEXT("v8443_nation%d_stable_atlas_slots_and_native_uv_bounds"),N),Slots);
    MedalWallV8215::PreloadVariants(Indices,false);
   }
   Check(TEXT("v8443_fr_ru_campaigns_remain_unavailable"),!FrontlineMedalsV8215::Available(EFRRUNationV81::French)&&!FrontlineMedalsV8215::Available(EFRRUNationV81::Russian));
   R->Stage=1;R->Next=Now+.1;return true;
  }
  if(R->Stage==1)
  {
   // Account seeding remains confined to empty Briefing. Fighting below is
   // a frozen HUD scene only, with no operation/actor preparation or ticks.
   B->Phase=EFrontlinePhase::Briefing;Clear();UIMotionReviewV83::TooltipAge=-1;
   const bool Battle=R->Page>=9,Room=R->Page==4||R->Page==5,ZH=R->Page==3||R->Page==5;
   const int32 Nation=R->Page<6||Battle?1:R->Page==6?0:R->Page==7?2:3;
   const int32 Fixture=R->Page<2?R->Page:R->Page<6||R->Page==9?2:3;
   Check(FString::Printf(TEXT("v8443_page%d_guarded_fixture"),R->Page),FrontlineAchievementsV76::SeedRibbonReviewV8443(B,EFRRUNationV81(Nation),Fixture));
   if(!R->Good)return Finish();
   FrontlineLocalizationV35::SetLanguage(ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
   if(Battle)
   {
    B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);
    FrontlineMapSceneV61::Update(PC,false,FVector2D(.5,.5),1,1,0,0,0);
    B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;PC->SupportTargetMode=0;
    auto& Mount=MedalMountV8216::View(PC);Mount.Loaded=true;Mount.Picker=false;Mount.OwnsPauseV836=false;
    Mount.Choice[1].Empty();Mount.Motion={};Mount.CameraSeen=false;
    if(R->Page==9)
    {
     const bool Left=B->GetTeamDirection(0)>0;V28ReviewPointer=FVector2D(Left?46.f:1876.f,32.f);
     UIMotionReviewV83::TooltipAge=.5;
    }
    else
    {
     const int32 I=R->Page==10?3:R->Page==11?19:18,G=I==18?2:1;
     Mount.Choice[1]=FString::Printf(TEXT("%s|%d"),FrontlineMedalsV8215::Definition(I).Id,G);
     Check(FString::Printf(TEXT("v8443_page%d_mount_uses_in_memory_awarded_selection"),R->Page),MedalMountV8216::Chosen(Mount,B)==I&&MedalMountV8216::SelectedGrade(Mount)==G);
     if(R->Page==13)
      Check(TEXT("v8443_picker_opened_through_production_hit_handler"),MedalMountV8216::Hit(PC,B,TEXT("V8216MedalMount"))&&Mount.Picker);
    }
    Check(FString::Printf(TEXT("v8443_page%d_frozen_empty_fighting_hud"),R->Page),B->Units.IsEmpty()&&B->IsProgressionVolatile()&&!B->IsActorTickEnabled()&&!PC->IsActorTickEnabled()&&!WarfareFRRUCampaignV82::Active(B)&&MedalMountV8216::Eligible(PC,B));
   }
   else if(Room)
   {
    auto& W=WarRoom52::State(PC);WarRoom52::Synchronize(W,B);WarRoom52::BeginRoute(W,B);W.WasActive=true;W.Tab=0;
    Check(FString::Printf(TEXT("v8443_page%d_actual_war_room_active"),R->Page),WarRoom52::Active(PC));
   }
   else
   {
    S.AchievementsV76=true;auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Nation=Nation;M.Focus=Nation==1?18:FrontlineMedalsV8215::NationIndices(EFRRUNationV81(Nation))[0];
    M.Row=M.ExhibitRowV8311=0;M.PreviewGrade=FMath::Max(1,MedalWallV8215::EntryTier(M.Focus));M.Gallery.Reset();M.Catalogue.Reset();M.ScrollNation=Nation;
   }
   R->BoundFrame=GFrameCounter;R->Stage=2;R->Next=Now+.65;return true;
  }
  if(R->Stage==2)
  {
   if(GFrameCounter<=R->BoundFrame+2)return true;
   bool Ready=true;
   for(const TCHAR* Code:{TEXT("uk"),TEXT("de"),TEXT("fr"),TEXT("ru")})
   {
    const FString Stem=FString(TEXT("T_RibbonAtlas_"))+Code+TEXT("_V8217");auto* T=FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V8217/")+Stem+TEXT(".")+Stem);
    Ready&=T&&T->GetSizeX()==2172&&T->GetSizeY()==724&&FrontlineTextureReady(T);
   }
   if(!Ready&&Now-R->Started<75){R->Next=Now+.1;return true;}
   Check(FString::Printf(TEXT("v8443_page%d_four_production_ribbon_atlases_ready"),R->Page),Ready);
   const bool Battle=R->Page>=9;
   int32 W=0,H=0;PC->GetViewportSize(W,H);Check(FString::Printf(TEXT("v8443_page%d_actual_4k_empty_ui_scene"),R->Page),W==3840&&H==2160
    &&(Battle?(B->IsProgressionVolatile()&&B->Units.IsEmpty()&&B->Phase==EFrontlinePhase::Fighting&&!B->IsActorTickEnabled()):Safe()));
   const bool Room=R->Page==4||R->Page==5;const int32 Nation=R->Page<6||Battle?1:R->Page==6?0:R->Page==7?2:3;
   const auto A=MedalWallV8215::OrderedRibbonItems(EFRRUNationV81(Nation));
   const auto Cells=MedalWallV8215::RibbonBarLayoutV8218(A.Num(),Room?52.f:126.f,Room?79.f:MedalWallV8215::ExhibitLayoutV8311::RackTop,Room?240.f:949.f,Room?42.f:54.f,!Room);
   Check(FString::Printf(TEXT("v8443_page%d_every_selected_ribbon_has_a_cell"),R->Page),Cells.Num()==A.Num());
   if(R->Page==9)
   {
    int32 Drawn=0;TSet<int32> Rows;
    for(const auto& Op:S.TooltipV83.Active.Ops)
     if(Op.Kind==ETooltipOpV83::Texture&&Op.Texture&&Op.Texture->GetName()==TEXT("T_RibbonAtlas_de_V8217"))
     {++Drawn;Rows.Add(FMath::RoundToInt(Op.Y*100.f));}
    Check(TEXT("v8443_actual_battle_flag_popup_draws_seven_ribbons_in_three_rows"),A.Num()==7&&S.TooltipV83.Active.Ready&&Drawn==7&&Rows.Num()==3);
   }
   if(R->Page>=10)
   {
    auto& Mount=MedalMountV8216::View(PC);const int32 I=MedalMountV8216::Chosen(Mount,B),G=MedalMountV8216::SelectedGrade(Mount);
    const auto Geometry=MedalMountV8216::GeometryV8443(MedalWallV8215::Art(I,G),914,8,92,138);
    Check(FString::Printf(TEXT("v8443_page%d_actual_selected_mount_crop_fills_the_bounded_aperture"),R->Page),Geometry.Valid&&Geometry.Cropped
     &&Geometry.Image.X>=913.999f&&Geometry.Image.Y>=7.999f&&Geometry.Image.X+Geometry.Image.W<=1006.001f&&Geometry.Image.Y+Geometry.Image.H<=146.001f);
   }
   if(!R->Good)return Finish();
   static const TCHAR* Names[]={TEXT("01-wall-six-en"),TEXT("02-wall-seven-en"),TEXT("03-wall-advanced-seven-en"),TEXT("04-wall-advanced-seven-zh"),TEXT("05-warroom-advanced-seven-en"),TEXT("06-warroom-advanced-seven-zh"),TEXT("07-wall-british-all-art-en"),TEXT("08-wall-french-ui-art-en"),TEXT("09-wall-russian-ui-art-en"),TEXT("10-battle-seven-ribbon-popup-en"),TEXT("11-mount-iron-cross-ii-en"),TEXT("12-mount-bavarian-cross-en"),TEXT("13-mount-hohenzollern-commander-en"),TEXT("14-mount-picker-en")};
   const FString Name(Names[R->Page]);FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(Name+TEXT(".png")),true,false);++R->Shots;
   auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetNumberField(TEXT("nation"),Nation);J->SetNumberField(TEXT("ribbon_count"),A.Num());J->SetNumberField(TEXT("viewport_width"),W);J->SetNumberField(TEXT("viewport_height"),H);
   TArray<TSharedPtr<FJsonValue>> Ids;for(int32 I:A)Ids.Add(MakeShared<FJsonValueNumber>(I));J->SetArrayField(TEXT("ribbon_indices"),Ids);R->Captures.Add(MakeShared<FJsonValueObject>(J));
   if(R->Page>=10)
   {
    auto& Mount=MedalMountV8216::View(PC);const int32 I=MedalMountV8216::Chosen(Mount,B),G=MedalMountV8216::SelectedGrade(Mount);
    const auto Geometry=MedalMountV8216::GeometryV8443(MedalWallV8215::Art(I,G),914,8,92,138);
    J->SetNumberField(TEXT("mounted_index"),I);J->SetNumberField(TEXT("mounted_grade"),G);
    J->SetNumberField(TEXT("mounted_x"),Geometry.Image.X);J->SetNumberField(TEXT("mounted_y"),Geometry.Image.Y);J->SetNumberField(TEXT("mounted_width"),Geometry.Image.W);J->SetNumberField(TEXT("mounted_height"),Geometry.Image.H);
   }
   ++R->Page;R->Stage=R->Page<14?1:3;R->Next=Now+.1;return true;
  }
  if(R->Stage==3)
  {
   bool Written=R->Shots==14;for(const auto& V:R->Captures){const auto J=V->AsObject();Written&=J.IsValid()&&IFileManager::Get().FileSize(*(R->Dir/TEXT("Captures")/(J->GetStringField(TEXT("name"))+TEXT(".png"))))>1024;}
   Check(TEXT("v8443_fourteen_actual_capture_files_written"),Written);return Finish();
  }
  Check(TEXT("v8443_known_review_stage"),false);return Finish();
 }),0.f);
}
}
