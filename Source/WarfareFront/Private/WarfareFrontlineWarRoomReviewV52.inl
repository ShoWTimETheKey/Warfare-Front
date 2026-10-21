namespace FrontlineWarRoomV52
{
void RunEntryChecksV828(AWarfareFrontlineController* PC,TFunctionRef<void(const FString&,bool)> Check)
{
 auto* B=FrontlineFor(PC);if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV828Review")))return;
 const auto Saved=WarRoom52::State(PC);ON_SCOPE_EXIT{WarRoom52::State(PC)=Saved;};
 for(auto F:{EFrontlineFaction::British,EFrontlineFaction::German})
 {
  B->AbandonBattleToOperations();const bool Began=B->StartNewCampaign(F);auto& S=WarRoom52::State(PC);
  WarRoom52::Synchronize(S,B);
  Check(FString::Printf(TEXT("new_campaign_%d_focuses_first_operation"),int32(F)),Began&&S.Preview==0&&S.AtlasMode==2&&S.FlightDuration>0);
  WarRoom52::AtlasTick(S,S.FlightStart+S.FlightDuration+.01);
  Check(FString::Printf(TEXT("new_campaign_%d_focus_arrives_on_dossier"),int32(F)),S.OperationSheet&&S.Detail==1);
  WarRoom52::BeginRoute(S,B);WarRoom52::Synchronize(S,B);
  Check(FString::Printf(TEXT("manual_overview_%d_is_not_forced_back"),int32(F)),S.AtlasMode==0&&S.FlightDuration==0);
 }
}
bool PrepareMenuReviewV61(AWarfareFrontlineController* PC,bool Confirm)
{
 auto* B=FrontlineFor(PC);if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV61Review")))return false;
 B->AbandonBattleToOperations();auto& S=ScreenState(PC);S.Dossier=0;S.Menu=S.Help=S.Credits=S.Settings=S.Technology=S.Preparing=S.Deployment=S.HistoricalDispatch=S.UnitIntroduction=false;
 S.ExitConfirmV61=Confirm;FrontlineReviewUIV41::Pointer(FVector2D(1900,1060));return true;
}
bool PrepareReview(AWarfareFrontlineController* PC,int32 Mission,int32 Tab,float Zoom,bool Dispatch)
{
 auto* B=FrontlineFor(PC);if(!PC||!B||!B->IsProgressionVolatile()||!(FParse::Param(FCommandLine::Get(),TEXT("WFV52Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV54Review"))))return false;
 FrontlineReviewUIV41::Dossier(PC);auto& S=WarRoom52::State(PC);WarRoom52::Synchronize(S,B);
 if(!B->GetCampaignMissions().IsValidIndex(Mission))return false;S.Preview=Mission;S.Stage=WarRoom52::Stage(Mission);S.Tab=Tab;S.Zoom=Zoom;
 S.Centre=Zoom>1.1f?WarRoom52::UV(B->GetCampaignMissions()[Mission].MapPosition):FVector2D(.5,.5);WarRoom52::Clamp(S);
 WarRoom52::StopFlight(S);S.RouteStarted=0.;S.OperationSheet=Zoom>2.55f;S.SheetStarted=FPlatformTime::Seconds()-2.;
 ScreenState(PC).HistoricalDispatch=Dispatch;FrontlineReviewUIV41::Pointer(FVector2D(1900,1060));return true;
}
bool PrepareReviewV54(AWarfareFrontlineController* PC,int32 Mission,float Zoom,bool OperationSheet,int32 Tab)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("WFV54Review"))||!PrepareReview(PC,Mission,Tab,1.f,false))return false;
 auto* B=FrontlineFor(PC);auto& S=WarRoom52::State(PC);WarRoom52::BeginRoute(S,B);S.WasActive=true;
 if(Zoom>1.1f&&Zoom<2.5f){WarRoom52::AtlasChooseSector(S,B,WarRoom52::AtlasSectorFor(Mission));WarRoom52::AtlasTick(S,S.FlightStart+S.FlightDuration+.01);}
 else if(Zoom>=2.5f){WarRoom52::FocusOperation(S,B,WarRoom52::UV(FrontlineCampaignV52::GetNode(B,Mission).LonLat),true,2.7f);
  WarRoom52::AtlasTick(S,S.FlightStart+S.FlightDuration*(OperationSheet?1.01:.59));
  if(!OperationSheet)WarRoom52::StopFlight(S);}
 S.SheetStarted=FPlatformTime::Seconds()-2.;return true;
}
bool PrepareLoadingReviewV54(AWarfareFrontlineController* PC)
{
 auto* B=FrontlineFor(PC);
 if(!PC||!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV54Review"))
  ||B->Phase!=EFrontlinePhase::Briefing||!B->GetSelectedCampaignMission())return false;
 // A focused scene 4 run must exercise the carried sheet as well. Previously
 // only a full scene 3 -> 4 run put it on the overlapping animation path.
 const int32 Mission=B->GetSelectedCampaignMission()->Id;
 const float Zoom=WarRoom52::HasOperation(Mission)?WarRoom52::MissionOperationZoom(WarRoom52::Stage(Mission)):3.6f;
 if(!PrepareReviewV54(PC,Mission,Zoom,true,0))return false;
 BeginOperationPreparation(PC,B);
 return ScreenState(PC).Preparing;
}
bool LoadingReviewSettledV59(AWarfareFrontlineController* PC)
{
 return PC&&ScreenState(PC).Preparing&&ScreenState(PC).PreparationVisualSeconds>=1.f;
}
void FinishLoadingReviewV54(AWarfareFrontlineController* PC)
{
 auto* B=FrontlineFor(PC);
 if(!PC||!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV54Review")))return;
 auto& S=ScreenState(PC);ClearPreparationWarmersV43(S);S.Preparing=false;
 B->SetPaused(false);
}
void RunFlightChecksV54(AWarfareFrontlineController* PC,TFunctionRef<void(const FString&,bool)> Check)
{
 auto* B=FrontlineFor(PC);
 if(!PC||!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV54Review"))){Check(TEXT("v60_atlas_isolated_fixture"),false);return;}
 const auto Saved=WarRoom52::State(PC);const auto UISaved=ScreenState(PC);
 ON_SCOPE_EXIT{WarRoom52::State(PC)=Saved;ScreenState(PC)=UISaved;};
 const int32 Selected=B->GetCurrentCampaign()->SelectedMission;const FString Updated=B->GetCurrentCampaign()->UpdatedUtc;
 Check(TEXT("v60_atlas_review_fixture"),PrepareReviewV54(PC,Selected,1.f,false,0));
 auto& S=WarRoom52::State(PC);WarRoom52::AtlasTick(S,FPlatformTime::Seconds()+10);
 Check(TEXT("v60_atlas_overview_waits_for_player"),S.AtlasMode==0&&S.Zoom==1.f&&S.FlightDuration==0.&&S.Detail==0.);
 bool Pick=true;for(const auto& Sector:WarRoom52::AtlasSectors())Pick&=WarRoom52::AtlasContains(Sector.Boundary,Sector.Centre);
 Check(TEXT("v60_every_sector_has_pickable_geographic_interior"),Pick&&WarRoom52::AtlasSectors().Num()==8);
 bool ClickPick=true;for(int32 I=0;I<WarRoom52::AtlasSectors().Num();++I)
  ClickPick&=WarRoom52::AtlasSectorAtPointV61(S,WarRoom52::AtlasPoint(S,WarRoom52::UV(WarRoom52::AtlasSectors()[I].Centre)))==I;
 Check(TEXT("v61_click_time_geographic_picking_does_not_depend_on_caption"),ClickPick&&WarRoom52::AtlasSectorAtPointV61(S,FVector2D(960,60))==INDEX_NONE);
 WarRoom52::Command(TEXT("V60Sector3"),PC,B);
 Check(TEXT("v60_region_click_starts_continuous_flight"),S.AtlasSector==3&&S.AtlasMode==1&&S.FlightDuration>2.&&S.FlightPhase==11);
 bool Inverse=true,Smooth=true;FVector2D Previous=S.Centre;
 for(int32 I=0;I<=100;++I){WarRoom52::AtlasTick(S,S.FlightStart+S.FlightDuration*I/101.);
  FrontlineMapSceneV61::Update(PC,true,S.Centre,S.Zoom,float(I)/101.f,S.AtlasSector,S.Preview,S.Detail);
  for(const auto& P:{FVector2D(100,180),FVector2D(950,500),FVector2D(1800,900)})Inverse&=WarRoom52::AtlasPoint(S,WarRoom52::AtlasInverse(S,P)).Equals(P,.0001);
  Smooth&=FVector2D::Distance(Previous,S.Centre)<.025;Previous=S.Centre;}
 Check(TEXT("v60_projected_paper_and_picking_share_inverse_through_flight"),Inverse);
 Check(TEXT("v62_flight_camera_is_continuous_and_bounded"),Smooth&&S.Zoom<=8.f&&FMath::Abs(S.Roll)<.037);
 WarRoom52::AtlasTick(S,S.FlightStart+S.FlightDuration+.01);
 Check(TEXT("v60_sector_arrival_does_not_open_operation_dossier"),S.AtlasMode==1&&!S.OperationSheet&&S.Detail==0);
 WarRoom52::Command(TEXT("V52Node19"),PC,B);
 Check(TEXT("v60_locked_preview_never_writes_campaign_selection"),S.Preview==19&&B->GetCurrentCampaign()->SelectedMission==Selected&&B->GetCurrentCampaign()->UpdatedUtc==Updated);
 WarRoom52::AtlasTick(S,S.FlightStart+S.FlightDuration*.5);
 Check(TEXT("v60_operation_approach_crossfades_detail_before_arrival"),S.Detail>0&&S.Detail<1&&!S.OperationSheet);
 WarRoom52::AtlasTick(S,S.FlightStart+S.FlightDuration+.01);
 Check(TEXT("v60_dossier_reveals_on_arrival_on_separate_native_detail"),S.OperationSheet&&S.Detail==1&&S.Zoom<=24.f&&S.Roll==0&&S.Tilt==0);
 WarRoom52::Command(TEXT("V52Overview"),PC,B);WarRoom52::AtlasTick(S,S.FlightStart+S.FlightDuration+.01);
 Check(TEXT("v60_overview_return_restores_persistent_full_map"),S.AtlasMode==0&&S.Zoom==1&&S.Detail==0&&!S.OperationSheet);
 WarRoom52::Command(TEXT("V60Sector6"),PC,B);WarRoom52::AtlasTick(S,S.FlightStart+S.FlightDuration+.01);
 Check(TEXT("v60_french_sector_preview_is_context_only"),S.AtlasSector==6&&S.AtlasMode==1&&!S.OperationSheet&&B->GetCurrentCampaign()->SelectedMission==Selected);
 WarRoom52::Command(TEXT("V60Current"),PC,B);
 Check(TEXT("v60_current_orders_returns_to_saved_operation"),S.Preview==Selected&&S.AtlasMode==2&&S.FlightDuration>0);
 bool Assets=WarRoom52::Map()!=nullptr;
 for(int32 I=0;I<5;++I)Assets&=WarRoom52::Region(I)!=nullptr;
 for(const auto& M:B->GetCampaignMissions())Assets&=CampaignMapTextureV41(B,&M)!=nullptr;
 Check(TEXT("v60_atlas_and_all_operation_art_available"),Assets);
 bool NativeMaps=true;for(int32 I=0;I<20;++I){const auto* T=WarRoom52::OperationV61(I);
#if WITH_EDITORONLY_DATA
  NativeMaps&=T&&T->Source.GetSizeX()==4096&&T->Source.GetSizeY()==4096;
#else
  NativeMaps&=T&&T->GetSizeX()==4096&&T->GetSizeY()==4096;
#endif
 }
 Check(TEXT("v61_twenty_independent_operation_plates"),NativeMaps);
 auto& Screen=ScreenState(PC);Screen.Dossier=0;Screen.Help=Screen.Credits=Screen.Technology=Screen.Settings=Screen.Menu=Screen.Deployment=false;
 Screen.ExitConfirmV61=false;PC->ToggleMenu();Check(TEXT("v61_escape_main_requests_confirmation"),Screen.ExitConfirmV61);
 const auto Phase=B->Phase;PC->BeginBattle();Check(TEXT("v61_exit_modal_blocks_enter"),Screen.ExitConfirmV61&&B->Phase==Phase);
 PC->ToggleMenu();Check(TEXT("v61_escape_cancels_exit"),!Screen.ExitConfirmV61&&Screen.Dossier==0);
 auto& H=NavigationV61::History(PC);H.Back.Empty();H.Forward.Empty();
 PC->ToggleSettings();const bool Open=Screen.Settings;PC->MenuBackV61();const bool Back=!Screen.Settings;
 PC->MenuForwardV61();Check(TEXT("v61_side_buttons_restore_view_only"),Open&&Back&&Screen.Settings&&B->GetCurrentCampaign()->SelectedMission==Selected);
 PC->ToggleSettings();H.Back.Empty();H.Forward.Empty();
 // Exercise pause ownership in actual fighting state as well as briefing.
 B->BeginBattle();Screen.Dossier=0;
 for(bool ManualPause:{false,true})
 {
  B->SetPaused(ManualPause);Screen.Menu=Screen.Settings=Screen.Help=Screen.Technology=false;
  Screen.bPausedByOverlay=Screen.bPausedForAuxOverlay=false;H.Back.Empty();H.Forward.Empty();
  PC->ToggleSettings();PC->MenuBackV61();PC->MenuForwardV61();PC->ToggleSettings();
  Check(ManualPause?TEXT("v61_side_navigation_preserves_manual_pause"):TEXT("v61_side_navigation_releases_owned_pause"),
   !Screen.Settings&&UGameplayStatics::IsGamePaused(PC)==ManualPause);
 }
 Screen.CampaignEndingV51=true;const auto BeforeCampaign=*B->GetCurrentCampaign();PC->MenuBackV61();
 Check(TEXT("v61_side_back_never_advances_ending_or_writes_archive"),Screen.CampaignEndingV51&&B->Phase==EFrontlinePhase::Fighting&&B->GetCurrentCampaign()->UpdatedUtc==BeforeCampaign.UpdatedUtc);
 Screen.CampaignEndingV51=false;B->SetPaused(false);B->AbandonBattleToOperations();H.Back.Empty();H.Forward.Empty();
 FrontlineMapSceneV61::Update(PC,false,S.Centre,S.Zoom,1.f,S.AtlasSector,S.Preview,S.Detail);
 Check(TEXT("v61_leave_map_restores_battle_view"),!FrontlineMapSceneV61::IsActive(PC));
 Check(TEXT("v60_no_extra_photo_fade_on_map_zoom_or_loading"),!FrontlinePresentationV37::UsePhotoTransition(5,6,3.)&&!FrontlinePresentationV37::UsePhotoTransition(15,9,3.));
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV52Review"))){Check(TEXT("v52_map_checks_require_isolated_profile"),false);return;}
 auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());if(!PC){Check(TEXT("v52_map_controller_exists"),false);return;}
 const auto Saved=ScreenState(PC);const auto MapSaved=WarRoom52::State(PC);
 ON_SCOPE_EXIT{ScreenState(PC)=Saved;WarRoom52::State(PC)=MapSaved;};
 B->AbandonBattleToOperations();const bool New=B->StartNewCampaign(EFrontlineFaction::British);Check(TEXT("v52_map_has_real_campaign"),New);if(!New)return;
 FrontlineReviewUIV41::Dossier(PC);auto& Map=WarRoom52::State(PC);WarRoom52::Synchronize(Map,B);
 const int32 Selected=B->GetCurrentCampaign()->SelectedMission;const FString Updated=B->GetCurrentCampaign()->UpdatedUtc;
 const bool Preview=WarRoom52::Command(TEXT("V52Node19"),PC,B);
 Check(TEXT("v52_locked_operation_preview_is_read_only"),Preview&&Map.Preview==19&&B->GetCurrentCampaign()->SelectedMission==Selected&&B->GetCurrentCampaign()->UpdatedUtc==Updated);
 WarRoom52::Command(TEXT("V52Begin"),PC,B);
 Check(TEXT("v52_locked_map_commit_cannot_begin_or_change_save"),B->Phase==EFrontlinePhase::Briefing&&B->GetCurrentCampaign()->SelectedMission==Selected&&!ScreenState(PC).HistoricalDispatch);
 WarRoom52::Command(TEXT("V52Deployment"),PC,B);
 Check(TEXT("v52_locked_preview_cannot_open_another_operations_loadout"),!ScreenState(PC).Deployment&&B->GetCurrentCampaign()->SelectedMission==Selected);
 WarRoom52::Command(TEXT("V52Node0"),PC,B);WarRoom52::Command(TEXT("V52Begin"),PC,B);
 Check(TEXT("v52_available_map_commit_opens_real_allocation_choice"),ScreenState(PC).HistoricalDispatch&&FrontlineCampaignV52::Dispatch(B).bPending);
 ScreenState(PC).HistoricalDispatch=false;
 // Exercise the production projection independently of a render viewport.
 Map.Zoom=2;Map.Centre=FVector2D(.5,.5);const FVector2D Anchor(.2,-.15),Before=Map.Centre+Anchor/Map.Zoom;
 WarRoom52::ZoomAt(Map,1.5f,Anchor);
 Check(TEXT("v52_map_zoom_preserves_geographic_mouse_anchor"),(Map.Centre+Anchor/Map.Zoom).Equals(Before,1e-6));
 for(int32 I=0;I<24;++I)WarRoom52::ZoomAt(Map,1.5f,FVector2D(.5,-.5));
 Check(TEXT("v52_map_zoom_bounded_and_geography_inside_texture"),Map.Zoom>=1&&Map.Zoom<=2.5f
  &&Map.Centre.X>=.5/Map.Zoom&&Map.Centre.X<=1-.5/Map.Zoom
  &&Map.Centre.Y>=.5/Map.Zoom&&Map.Centre.Y<=1-.5/Map.Zoom);
 WarRoom52::Command(TEXT("V52Overview"),PC,B);Check(TEXT("v52_map_overview_routes_to_theatre_projection"),Map.FlightToZoom==1&&Map.FlightToCentre.Equals(FVector2D(.5,.5)));
 Check(TEXT("v52_map_and_command_stars_assets_present"),WarRoom52::Map()&&LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V52/T_CommandStars_V52.T_CommandStars_V52")));
 const bool Fork=FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,2);
 WarRoom52::Synchronize(Map,B);WarRoom52::Command(TEXT("V52Node3"),PC,B);WarRoom52::Command(TEXT("V52Deployment"),PC,B);
 Check(TEXT("v52_loadout_button_commits_previewed_available_branch"),Fork&&ScreenState(PC).Deployment&&B->GetCurrentCampaign()->SelectedMission==3);
}
}
