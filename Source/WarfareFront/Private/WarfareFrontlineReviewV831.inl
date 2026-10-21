#include "WarfareFrontlineInfantryV45.h"

// A bounded disposable presentation/gameplay check, not a campaign completion.
namespace
{
void StartFrontlineReviewV831(AWarfareFrontlineController* Controller)
{
 if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV831Review"))
  ||!FParse::Param(FCommandLine::Get(),TEXT("WFV8221Review")))return;
 FString User;if(!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun
 {
  int32 Stage=0,Shots=0,Samples=0;bool Good=true;
  double Began=FPlatformTime::Seconds(),Next=Began+2.,Prepared=0,SampleFrom=0,SampleUntil=0,SampleLast=0;
  double WallMs=0,GameMs=0,RenderMs=0,RHIMs=0,GPUMs=0;
  FString Dir,CostMode;TArray<TSharedPtr<FJsonValue>> Checks,Costs;
  TWeakObjectPtr<UStaticMeshComponent> HiddenWater;bool WaterWasVisible=false;
 };
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[R](const FString& Name,bool Pass)
  {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);
   R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V831_CHECK %s %d"),*Name,int32(Pass));};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J)
  {FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/Name));};
  auto Finish=[&]()
  {
   if(auto* Water=R->HiddenWater.Get())Water->SetVisibility(R->WaterWasVisible);
   R->HiddenWater.Reset();
   if(B)FrontlineImpactV831::ClearVisualFixture(B);V28ReviewPointer.Reset();UIMotionReviewV83::TooltipAge=-1;
   auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);
   J->SetArrayField(TEXT("short_effect_cost_samples"),R->Costs);J->SetNumberField(TEXT("captures_requested"),R->Shots);
   J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);
   J->SetStringField(TEXT("scope"),FParse::Param(FCommandLine::Get(),TEXT("WFV832Review"))?
    TEXT("V83.2 focused actual Shipping bilingual language endonyms, settings paint selection, headquarters ribbon placement and locked/unlocked archive input and footer. Disposable profile; no real player records, paused national campaigns or battle/performance certification."):
    TEXT("V83.1 focused actual Shipping rifle firing-step movement/sight tests, bilingual selector/settings and headquarters ribbon placement; coherent old/new five-profile explosion fixtures at tactical/close/oblique cameras. Disposable profile. No real player records, paused national campaigns, whole-campaign/fun or sustained hardware certification."));
   Write(TEXT("checks.json"),J);FPlatformMisc::RequestExit(false);return false;
  };
  if(!B||!FrontlineEffectsReviewSafetyV8216::Allowed(B)){Check(TEXT("v831_strict_disposable_world"),false);return Finish();}
  if(Now-R->Began>165.){Check(TEXT("v831_bounded_review"),false);return Finish();}
  if(R->SampleFrom>0&&Now>=R->SampleFrom&&Now<=R->SampleUntil)
  {
   if(R->SampleLast>0){R->WallMs+=(Now-R->SampleLast)*1000.;R->GameMs+=FPlatformTime::ToMilliseconds(GGameThreadTime);
    R->RenderMs+=FPlatformTime::ToMilliseconds(GRenderThreadTime);R->RHIMs+=FPlatformTime::ToMilliseconds(GRHIThreadTime);
    R->GPUMs+=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());++R->Samples;}
   R->SampleLast=Now;
  }
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&]()
  {
   UGameplayStatics::SetGamePaused(PC,false);
   S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
   S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.Dossier=1917;
   S.FactionSelectionV82=FFrontlineFactionSelectionV82();MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;
   UltimateArchiveUIV8215::Close(PC);PageCanvasV73::Audit=false;PageCanvasV73::Hits.Empty();
   MedalMountV8216::View(PC).Picker=false;PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);UIMotionReviewV83::TooltipAge=-1;
  };
  auto Language=[&](bool ZH){FrontlineLocalizationV35::SetLanguage(ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);};
  auto Shot=[&](const TCHAR* Name)
  {
   if(S.FactionSelectionV82.Visible)
   {
    const auto& Painted=FactionSelectionV82::LayoutV831::Checks;
    Check(FString(Name)+TEXT("_actual_shaped_text_drawn"),FactionSelectionV82::LayoutV831::Frame!=MAX_uint64&&GFrameCounter-FactionSelectionV82::LayoutV831::Frame<=2);
    for(const auto& V:Painted){const auto J=V->AsObject();Check(FString(Name)+TEXT("_")+J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));}
    float ExpectedDPI=1.f;FParse::Value(FCommandLine::Get(),TEXT("WFCanvasDPIReview="),ExpectedDPI);
    Check(FString(Name)+TEXT("_actual_canvas_dpi_matches_requested"),FMath::IsNearlyEqual(FactionSelectionV82::LayoutV831::CanvasDPI,ExpectedDPI,.0001f));
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("actual_canvas_dpi"),FactionSelectionV82::LayoutV831::CanvasDPI);J->SetNumberField(TEXT("output_scale"),FactionSelectionV82::LayoutV831::OutputScale);J->SetArrayField(TEXT("checks"),Painted);Write(FString(Name)+TEXT("-layout.json"),J);
   }
   if(S.Settings)
   {
    Check(FString(Name)+TEXT("_actual_settings_geometry_drawn"),SettingsGeometryV831::Frame!=MAX_uint64&&GFrameCounter-SettingsGeometryV831::Frame<=2&&!SettingsGeometryV831::Rows.IsEmpty());
    int32 Tabs=0,Selected=0;for(const auto& V:SettingsGeometryV831::Rows){auto J=V->AsObject();if(J->GetBoolField(TEXT("tab"))){++Tabs;Selected+=J->GetBoolField(TEXT("selected"));}}
    Check(FString(Name)+TEXT("_one_active_tab"),Tabs==4&&Selected==1);
    bool EnglishName=false,ChineseName=false;
    for(const auto& V:SettingsGeometryV831::Rows){const auto J=V->AsObject();const FString Command=J->GetStringField(TEXT("command"));
     if(Command==TEXT("LanguageEnglish"))EnglishName=J->GetStringField(TEXT("caption"))==TEXT("English");
     if(Command==TEXT("LanguageChinese"))ChineseName=J->GetStringField(TEXT("caption"))==TEXT("简体中文");}
    Check(FString(Name)+TEXT("_language_names_remain_in_their_own_language"),EnglishName&&ChineseName);
    auto J=MakeShared<FJsonObject>();J->SetArrayField(TEXT("painted_controls"),SettingsGeometryV831::Rows);Write(FString(Name)+TEXT("-geometry.json"),J);
   }
   if(WarRoom52::Active(PC)&&!S.Settings&&!S.FactionSelectionV82.Visible)
   {
    TArray<TSharedPtr<FJsonValue>> Hits;int32 ArchiveSlots=0;bool CorrectGate=false,OldSectorButton=false;
    const bool Available=FrontlineUltimateArchiveV8215::Available(B->GetPlayerFaction());
    for(const auto& P:PageCanvasV73::Hits){const auto J=P.Value;Hits.Add(MakeShared<FJsonValueObject>(J));
     const FString Command=J->GetStringField(TEXT("command"));OldSectorButton|=Command.StartsWith(TEXT("V52Stage"));
     if(FMath::IsNearlyEqual(float(J->GetNumberField(TEXT("x"))),953.f)&&FMath::IsNearlyEqual(float(J->GetNumberField(TEXT("y"))),1018.f))
     {++ArchiveSlots;CorrectGate=Command==(Available?TEXT("V8215ArchiveOpen"):TEXT("Blocked"));}}
    Check(FString(Name)+TEXT("_one_footer_archive_slot_respects_route_unlock"),ArchiveSlots==1&&CorrectGate&&!OldSectorButton);
    auto J=MakeShared<FJsonObject>();J->SetArrayField(TEXT("painted_hit_regions"),Hits);Write(FString(Name)+TEXT("-hit-regions.json"),J);
   }
   if(PC->PlayerCameraManager)
   {
    const auto& V=PC->PlayerCameraManager->GetCameraCacheView();auto J=MakeShared<FJsonObject>();
    J->SetStringField(TEXT("location"),V.Location.ToString());J->SetStringField(TEXT("rotation"),V.Rotation.ToString());
    J->SetStringField(TEXT("forward"),V.Rotation.Vector().ToString());J->SetNumberField(TEXT("ortho_width"),V.OrthoWidth);
    J->SetNumberField(TEXT("projection_mode"),int32(V.ProjectionMode));
    if(PC->BattleCamera){J->SetStringField(TEXT("actor_location"),PC->BattleCamera->GetActorLocation().ToString());J->SetStringField(TEXT("actor_rotation"),PC->BattleCamera->GetActorRotation().ToString());}
    Write(FString(Name)+TEXT("-camera.json"),J);
   }
   FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(FString(Name)+TEXT(".png")),true,false);++R->Shots;
  };
  auto Selector=[&](int32 Army,bool ZH)
  {Clear();Language(ZH);S.Dossier=1918;auto& F=S.FactionSelectionV82;F.Visible=true;F.Selected=F.Hover=Army;F.FreshInput=true;
   F.PointerOwnsSelectionV826=false;F.LastTick=Now;F.LayoutFrame=GFrameCounter;
   for(int32 I=0;I<4;++I)F.Widths[I]=Army<0?480.f:I==Army?768.f:384.f;PC->SetActorTickEnabled(false);};
  auto Settings=[&](int32 Tab,bool ZH){Clear();Language(ZH);LoadDisplayChoices(S);S.Settings=true;S.SettingsTab=Tab;PC->SetActorTickEnabled(false);};
  auto WarRoom=[&](bool ZH,bool Hover)
  {Clear();Language(ZH);B->Phase=EFrontlinePhase::Briefing;auto& W=WarRoom52::State(PC);WarRoom52::Synchronize(W,B);
   W.FlightStart=Now-10;WarRoom52::AtlasTick(W,Now);W.WasActive=true;PC->SetActorTickEnabled(false);PageCanvasV73::Audit=true;
   if(Hover){UIMotionReviewV83::TooltipAge=.6f;
    const auto Items=MedalWallV8215::OrderedRibbonItems(EFRRUNationV81(uint8(B->GetPlayerFaction())));
    const auto Cells=FrontlineRibbonRackV8222::Layout(Items.Num(),52,79,240,42,false);
    if(!Cells.IsEmpty())V28ReviewPointer=Cells[0].GetCenter();}};
  auto Prepare=[&]()
  {
   Clear();B->Phase=EFrontlinePhase::Briefing;B->SetActorTickEnabled(true);PC->SetActorTickEnabled(true);
   Check(TEXT("v831_clean_british_battle_fixture"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* C=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))
   {C->CompletedMissions=19;C->WonOperationMaskV52=(1u<<19)-1;C->DispatchResolvedMaskV52=(1u<<20)-1;
    C->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;C->Research.ResearchPoints=60;}
   Check(TEXT("v831_existing_operation_selected"),B->SelectCampaignMission(16));S.AcceptedLoadoutMission=INDEX_NONE;
   BeginOperationPreparation(PC,B);R->Prepared=Now;
  };
  auto Fixture=[&](float Age,bool Legacy,int32 Camera)
  {
   if(auto* Water=R->HiddenWater.Get())Water->SetVisibility(R->WaterWasVisible);
   R->HiddenWater.Reset();
   Clear();FrontlineImpactV831::ClearVisualFixture(B);
   const float Z=AWarfareFrontlineGround::HeightAt(0,250);
   FFrontlineReviewV32::Camera(PC,0,Camera==0?0:250,Camera==0?10500.f:2600.f,Z+150.f,true);
   if(Camera==2)
   {const FVector Focus(0,250,Z+250),From=Focus+FVector(7500,11000,11000);
    PC->BattleCamera->SetActorLocation(From);PC->BattleCamera->SetActorRotation((Focus-From).Rotation());}
   // Frozen actors must bake their depth clipping and retained-card orientation
   // from this camera, not the previous capture's camera-manager cache.
   if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);
   const int32 Profiles[]={0,1,2,4,5};
   const int32 Count=Camera==0?5:1;
   for(int32 I=0;I<Count;++I)
   {
    FVector At(Camera==0?(I-2)*1700.f:0,Camera==0?0:250,0);At.Z=AWarfareFrontlineGround::HeightAt(At.X,At.Y);
    if(Profiles[I]==4)At.Z+=430;
    auto* FX=FrontlineImpactV831::SetupVisualFixture(B,Profiles[I],Age,Legacy,At,2);
    Check(FString::Printf(TEXT("v831_effect_actor_%d_%03d_%d"),Legacy,int32(Age*1000),I),FX!=nullptr);
   }
   auto FixtureDescription=FrontlineImpactV831::DescribeVisualFixture(B);
   Check(FString::Printf(TEXT("v831_exact_simultaneous_effect_count_%02d"),R->Stage),FixtureDescription->GetArrayField(TEXT("actors")).Num()==Count);
   Write(FString::Printf(TEXT("effects-%02d.json"),R->Stage),FixtureDescription);
  };
  auto HideWater=[&](const TCHAR* Name)
  {
   auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Points;int32 Count=0;
   for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
   {
    auto* Water=It->BasinWaterV44.Get();if(!Water||!Water->IsRegistered())continue;
    ++Count;R->HiddenWater=Water;R->WaterWasVisible=Water->IsVisible();
    J->SetStringField(TEXT("component"),Water->GetPathName());J->SetBoolField(TEXT("was_visible"),R->WaterWasVisible);
    J->SetStringField(TEXT("mesh"),Water->GetStaticMesh()?Water->GetStaticMesh()->GetPathName():TEXT("none"));
    J->SetStringField(TEXT("world_location"),Water->GetComponentLocation().ToString());
    J->SetStringField(TEXT("bounds_origin"),Water->Bounds.Origin.ToString());J->SetStringField(TEXT("bounds_extent"),Water->Bounds.BoxExtent.ToString());
    if(auto* M=Water->GetMaterial(0)){J->SetStringField(TEXT("material"),M->GetPathName());J->SetNumberField(TEXT("blend_mode"),int32(M->GetBlendMode()));}
    Water->SetVisibility(false);
   }
   for(float X:{-280.f,0.f,280.f})for(float Y:{100.f,250.f,400.f})
   {
    float Level=0,Depth=0,Group=0;const bool Wet=AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth,&Group);
    auto P=MakeShared<FJsonObject>();P->SetNumberField(TEXT("x"),X);P->SetNumberField(TEXT("y"),Y);P->SetNumberField(TEXT("ground"),AWarfareFrontlineGround::HeightAt(X,Y));
    P->SetBoolField(TEXT("wet"),Wet);P->SetNumberField(TEXT("water_level"),Level);P->SetNumberField(TEXT("depth"),Depth);P->SetNumberField(TEXT("group"),Group);
    Points.Add(MakeShared<FJsonValueObject>(P));
   }
   J->SetArrayField(TEXT("physical_surface_queries"),Points);Write(Name,J);
   Check(TEXT("v831_diagnostic_only_actual_basin_water_hidden"),Count==1&&R->WaterWasVisible);
  };
  auto BeginCost=[&](const FString& Mode)
  {R->CostMode=Mode;R->Samples=0;R->SampleLast=0;R->WallMs=R->GameMs=R->RenderMs=R->RHIMs=R->GPUMs=0;R->SampleFrom=Now+1.;R->SampleUntil=R->SampleFrom+.7;};
  auto EndCost=[&]()
  {
   Check(R->CostMode+TEXT("_short_warmed_sample"),R->Samples>0);const double D=FMath::Max(1,R->Samples);auto J=MakeShared<FJsonObject>();
   J->SetStringField(TEXT("mode"),R->CostMode);J->SetNumberField(TEXT("samples"),R->Samples);J->SetNumberField(TEXT("wall_ms"),R->WallMs/D);
   J->SetNumberField(TEXT("game_ms"),R->GameMs/D);J->SetNumberField(TEXT("render_ms"),R->RenderMs/D);J->SetNumberField(TEXT("rhi_ms"),R->RHIMs/D);J->SetNumberField(TEXT("gpu_ms"),R->GPUMs/D);
   J->SetStringField(TEXT("scope"),TEXT("Five frozen simultaneous effects, one camera, 0.7-second local comparison. Not sustained battle/FG acceptance."));
   R->Costs.Add(MakeShared<FJsonValueObject>(J));R->SampleFrom=0;
  };
  double Delay=.65;
  switch(R->Stage)
  {
   case 0:
    FactionSelectionV82::Preload();if(!FactionSelectionV82::PreloadSettled()&&Now-R->Began<30.){R->Next=Now+.1;return true;}
    Check(TEXT("v831_selector_images_ready"),FactionSelectionV82::Images().Failure.IsEmpty());
    Clear();B->Phase=EFrontlinePhase::Briefing;Check(TEXT("v831_clean_ui_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
    RunInterfaceChecksV831(Check);
    Check(TEXT("v831_ui_honour_fixture_disposable"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,true,3));
    {auto* Rnd=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());Check(TEXT("v831_actual_render_settings_available"),Rnd!=nullptr);
     if(Rnd){Rnd->SetHDREnabled(false,false);Rnd->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Rnd->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Rnd->SetQuality(EFrontlineRenderQuality::Quality,false);Write(TEXT("renderer.json"),Rnd->GetDiagnostics());}}
    break;
   case 1:Selector(INDEX_NONE,false);break;
   case 2:Shot(TEXT("01-selector-unselected-en"));break;
   case 3:Selector(0,false);break;
   case 4:Shot(TEXT("02-selector-british-en"));break;
   case 5:Selector(2,true);break;
   case 6:Shot(TEXT("03-selector-german-zh"));break;
   case 7:Settings(0,true);break;
   case 8:Shot(TEXT("04-settings-video-zh"));break;
   case 9:Settings(2,false);break;
   case 10:Shot(TEXT("05-settings-image-en"));break;
   case 11:WarRoom(true,true);Delay=1.;break;
   case 12:Shot(TEXT("06-warroom-rack-zh"));break;
   case 13:WarRoom(false,false);break;
   case 14:Shot(TEXT("07-warroom-rack-en"));break;
   case 15:
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV831UIOnly")))
    {
     if(!FParse::Param(FCommandLine::Get(),TEXT("WFV832Review"))){Clear();return Finish();}
     auto* H=Cast<AWarfareFrontlineHUD>(PC->GetHUD());Check(TEXT("v832_archive_input_has_actual_hud"),H!=nullptr);
     if(H){H->NotifyHitBoxClick(TEXT("V8215ArchiveOpen"));Check(TEXT("v832_unlocked_footer_opens_existing_archive"),UltimateArchiveUIV8215::Active(PC));UltimateArchiveUIV8215::Close(PC);}
     Check(TEXT("v832_locked_archive_fixture_is_disposable"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,false,0));
     if(H){H->NotifyHitBoxClick(TEXT("V8215ArchiveOpen"));Check(TEXT("v832_locked_footer_rejects_direct_archive_command"),!UltimateArchiveUIV8215::Active(PC));}
     WarRoom(true,false);V28ReviewPointer=FVector2D(1092,1038);UIMotionReviewV83::TooltipAge=.6f;R->Stage=69;Delay=1.;break;
    }
    Prepare();Delay=.1;break;
   case 70:Shot(TEXT("08-warroom-archive-locked-zh"));break;
   case 71:Clear();return Finish();
   case 16:
    if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<60.){R->Next=Now+.1;return true;}
    Check(TEXT("v831_actual_prepared_battle"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty());
    if(B->Phase!=EFrontlinePhase::Fighting||!B->AssetFailure.IsEmpty())return Finish();
    B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);Clear();
    Write(TEXT("fire-control.json"),FrontlineInfantryV45::RunFireControlChecksV831(B,Check));FrontlineImpactV831::RunChecks(B,Check);break;
   case 17:Fixture(.20f,true,0);BeginCost(TEXT("legacy_five_effects"));Delay=1.8;break;
   case 18:EndCost();Shot(TEXT("08-legacy-five-effects-020"));break;
   case 19:Fixture(.20f,false,0);BeginCost(TEXT("candidate_five_effects"));Delay=1.8;break;
   case 20:EndCost();Shot(TEXT("09-candidate-five-effects-020"));break;
   case 21:Fixture(.08f,false,0);break;
   case 22:Shot(TEXT("10-candidate-early-008"));break;
   case 23:Fixture(.45f,false,0);break;
   case 24:Shot(TEXT("11-candidate-falling-fragments-045"));break;
   case 25:Fixture(.90f,false,0);break;
   case 26:Shot(TEXT("12-retained-smoke-090"));break;
   case 27:Fixture(.025f,false,1);break;
   case 28:Shot(TEXT("13-close-short-hot-core-0025"));break;
   case 29:Fixture(.18f,false,1);break;
   case 30:Shot(TEXT("14-close-soil-detail-018"));break;
   case 31:Fixture(.18f,false,2);break;
   case 32:Shot(TEXT("15-oblique-volume-018"));break;
   case 33:Fixture(.18f,true,2);break;
   case 34:Shot(TEXT("16-legacy-oblique-volume-018"));break;
   case 35:
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV831SurfaceSequence"))){R->Stage=50;break;}
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV831LayerDiagnostic"))){Clear();return Finish();}
    Fixture(.18f,false,2);Check(TEXT("v831_diagnostic_only_flat_cards_hidden"),FrontlineImpactV831::HideVisualFixtureLayer(B,1)>0);break;
   case 36:Shot(TEXT("17-oblique-without-flat-cards"));break;
   case 37:Fixture(.18f,false,2);Check(TEXT("v831_diagnostic_only_retained_volume_hidden"),FrontlineImpactV831::HideVisualFixtureLayer(B,2)>0);break;
   case 38:Shot(TEXT("18-oblique-without-retained-volume"));break;
   case 39:Fixture(.18f,false,2);Check(TEXT("v831_diagnostic_only_new_composition_hidden"),FrontlineImpactV831::HideVisualFixtureLayer(B,3)>0);break;
   case 40:Shot(TEXT("19-oblique-without-new-composition"));break;
   case 41:Fixture(.18f,false,2);Check(TEXT("v831_diagnostic_bound_cube_mid_density_zero"),FrontlineImpactV831::HideVisualFixtureLayer(B,4)>0);
    Write(TEXT("zero-density-bound-parameters.json"),FrontlineImpactV831::DescribeVisualFixture(B));break;
   case 42:Shot(TEXT("20-oblique-retained-density-zero"));break;
   case 43:
    if(auto* Rnd=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
    {Check(TEXT("v831_diagnostic_native_upscaler_requested"),Rnd->SetQuality(EFrontlineRenderQuality::Native,false));Rnd->SetRayReconstruction(false,false);}
    Fixture(.18f,false,2);Delay=2.;break;
   case 44:Shot(TEXT("21-oblique-without-nvidia-upscaler"));
    if(auto* Rnd=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))Write(TEXT("native-diagnostic-renderer.json"),Rnd->GetDiagnostics());break;
   case 45:Fixture(.18f,false,2);Check(TEXT("v831_diagnostic_raster_proxy_enlarged_physical_volume_unchanged"),FrontlineImpactV831::HideVisualFixtureLayer(B,5)>0);
    Write(TEXT("enlarged-proxy-bound-parameters.json"),FrontlineImpactV831::DescribeVisualFixture(B));break;
   case 46:Shot(TEXT("22-oblique-enlarged-raster-proxy"));break;
   case 47:Fixture(.18f,false,2);HideWater(TEXT("water-depth-diagnostic.json"));break;
   case 48:Shot(TEXT("23-oblique-without-basin-water"));break;
   case 49:Fixture(.18f,false,1);HideWater(TEXT("water-depth-close-diagnostic.json"));break;
   case 50:Shot(TEXT("24-close-without-basin-water"));break;
   case 51:
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV831SurfaceSequence"))){Clear();return Finish();}
    Fixture(.08f,false,1);break;
   case 52:Shot(TEXT("17-close-water-contact-008"));break;
   case 53:Fixture(.20f,false,1);break;
   case 54:Shot(TEXT("18-close-water-contact-020"));break;
   case 55:Fixture(.45f,false,1);break;
   case 56:Shot(TEXT("19-close-water-contact-045"));break;
   case 57:Fixture(.90f,false,1);break;
   case 58:Shot(TEXT("20-close-water-contact-090"));break;
   case 59:Fixture(.08f,false,2);break;
   case 60:Shot(TEXT("21-oblique-water-contact-008"));break;
   case 61:Fixture(.45f,false,2);break;
   case 62:Shot(TEXT("22-oblique-water-contact-045"));break;
   default:Clear();return Finish();
  }
  ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
 }),0.f);
}
}

