#include "WarfareFrontlineCombatV64.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineSupportV28.h"
TSharedRef<FJsonObject> RunFrontlineContactV835Review(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>);
TSharedRef<FJsonObject> RunFrontlineArmourV836Review(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>);
bool PrepareFrontlineArmourVisualV836(AWarfareFrontlineGameMode*,bool);
#include "WarfareFrontlineReviewV837.inl"
#include "WarfareFrontlineReviewV838.inl"
#include "WarfareFrontlineReviewV839.inl"
#include "WarfareFrontlineReviewV8310.inl"
#include "WarfareFrontlineReviewV8311.inl"
#include "WarfareFrontlineReviewV84.inl"
#include "WarfareFrontlineStencilReviewV842.inl"
#include "WarfareFrontlineReviewV844.inl"
#include "WarfareFrontlineRibbonReviewV8443.inl"
#include "WarfareFrontlineReviewV8444.inl"
#include "WarfareFrontlineReviewV8445.inl"
#include "WarfareFrontlineReviewV8446.inl"
#include "WarfareFrontlineReviewV8447.inl"
#include "WarfareFrontlineReviewV8448.inl"
#include "WarfareFrontlineReviewV8449.inl"
#include "WarfareFrontlineReviewV8450.inl"
#include "WarfareFrontlineReviewV84411.inl"
#include "WarfareFrontlineReviewV84412.inl"
#include "WarfareFrontlineReviewV84413.inl"
#include "WarfareFrontlineReviewV84414.inl"
#include "WarfareFrontlineReviewV84415.inl"
#include "WarfareFrontlineReviewV85.inl"
#include "WarfareFrontlineReviewV851.inl"
#include "WarfareFrontlineReviewV86.inl"

namespace
{
void StartFrontlineReviewV833(AWarfareFrontlineController* Controller)
{
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))){StartFrontlineReviewV86(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV851Focused"))){if(FParse::Param(FCommandLine::Get(),TEXT("WFV851Feasibility")))StartFrontlineReviewV838(Controller);else StartFrontlineReviewV851(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV85Focused"))){StartFrontlineReviewV85(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV84415Focused"))){StartFrontlineReviewV84415(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV84414Focused"))){StartFrontlineReviewV84414(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV84413Focused"))){StartFrontlineReviewV84413(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV84412GameplayFocused"))){StartFrontlineReviewV84412(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"))){StartFrontlineReviewV84411(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV8450Review"))){StartFrontlineReviewV8450(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV8449Review"))){StartFrontlineReviewV8449(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV8448Review"))){StartFrontlineReviewV8448(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV8447Review"))){StartFrontlineReviewV8447(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV8446Review"))){StartFrontlineReviewV8446(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV8445Review"))){StartFrontlineReviewV8445(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV8444Review"))){StartFrontlineReviewV8444(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV8443RibbonReview"))){StartRibbonReviewV8443(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV844Review"))){StartFrontlineReviewV844(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV842StencilReview"))){StartFrontlineStencilReviewV842(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV84Review"))){StartFrontlineReviewV84(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review"))){StartFrontlineReviewV8311(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV8310Review"))){StartFrontlineReviewV8310(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV839Review"))){StartFrontlineReviewV839(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV838Review"))){StartFrontlineReviewV838(Controller);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV837Review"))){StartFrontlineReviewV837(Controller);return;}
 if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV833Review")))return;
 FString User;if(!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun
 {
  int32 Stage=0,Shots=0;bool Good=true;double Began=FPlatformTime::Seconds(),Next=Began+2.,Prepared=0;
  FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;TWeakObjectPtr<AWarfareFrontlineImpact> LiveBlast;
 };
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  const bool Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV833Baseline"));
  const auto Check=[R](const FString& Name,bool Pass)
  {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);
   R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V833_CHECK %s %d"),*Name,int32(Pass));};
  const auto Write=[&](const TCHAR* Name,TSharedRef<FJsonObject> J)
  {FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/Name));};
  const auto Finish=[&]()
  {V28ReviewPointer.Reset();auto J=MakeShared<FJsonObject>();
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV834Review")))
   {
    const auto Target=FrontlineTargetReuseV834::Review(),Support=FrontlineSupportReuseV834::Review();
    Check(TEXT("v834_same_frame_target_selection_exactly_matches_original"),Target->GetNumberField(TEXT("same_frame_base_target_checks"))>0&&Target->GetNumberField(TEXT("same_frame_base_target_failures"))==0);
    Check(TEXT("v834_sight_query_lists_reused_without_skipping_traces"),Target->GetNumberField(TEXT("avoided_local_query_builds"))>0);
    Check(TEXT("v834_machinegun_support_tick_pointers_match_original"),Support->GetNumberField(TEXT("same_frame_pointer_checks"))>0&&Support->GetNumberField(TEXT("same_frame_pointer_failures"))==0);
    Check(TEXT("v834_machinegun_support_local_component_queries_reduced"),Support->GetNumberField(TEXT("avoided_explicit_lookups"))>0);
    J->SetObjectField(TEXT("target_reuse_v834"),Target);J->SetObjectField(TEXT("support_reuse_v834"),Support);
   }
   J->SetBoolField(TEXT("success"),R->Good);
   J->SetBoolField(TEXT("baseline"),Baseline);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);
   J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);
   J->SetStringField(TEXT("scope"),FParse::Param(FCommandLine::Get(),TEXT("WFV835Review"))
    ?TEXT("Disposable UK/DE production contact/duel, mine-action, unlock-navigation and fragment-history checks; three actual 4K mine-action/live-mortar captures. No real player records or FR/RU campaign content; no whole-campaign balance or performance claim.")
    :TEXT("Bounded disposable UK/DE production-actor mine clearance and anti-armour combat scenarios, all six archival research pages and bilingual headquarters. No real player data, national campaign development or whole-campaign enjoyment claim."));
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV836Review")))J->SetStringField(TEXT("scope"),TEXT("Disposable UK/DE armour/infantry contact, actual recruitment queues, gas actions and permanent masks, allocation refund/configuration, 1.5x speed and fixed-area antitank support. Actual 4K vehicle/gas/fire images. No real player records or FR/RU campaign development; no whole-campaign balance or performance guarantee."));
   Write(TEXT("checks.json"),J);FPlatformMisc::RequestExit(false);return false;};
  if(!B||!FrontlineEffectsReviewSafetyV8216::Allowed(B)){Check(TEXT("v833_disposable_world"),false);return Finish();}
  if(FParse::Param(FCommandLine::Get(),TEXT("WFV836Review"))&&R->Stage>=26&&R->Stage<=30)
    if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld()))
      for(TActorIterator<AFrontlineTank> It(B->GetWorld());It;++It)if(!It->IsOperational())
        Fire->EmitBurningWreck(It->AimPoint(),.016f,1.25f,float(It->Serial)*.37f);
  if(Now-R->Began>210.){Check(TEXT("v833_bounded_review"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  {auto Progress=MakeShared<FJsonObject>();Progress->SetNumberField(TEXT("stage"),R->Stage);Progress->SetNumberField(TEXT("wall_seconds"),Now-R->Began);Progress->SetBoolField(TEXT("checks_so_far"),R->Good);Write(TEXT("stage.json"),Progress);}
  auto& S=ScreenState(PC);
  const auto Clear=[&]()
  {UGameplayStatics::SetGamePaused(PC,false);
   S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
   S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.Dossier=1917;
   S.FactionSelectionV82=FFrontlineFactionSelectionV82();PC->CancelTargeting();
   MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::CloseV836(PC);
   UltimateArchiveUIV8215::Close(PC);V28ReviewPointer=FVector2D(-10000,-10000);};
  const auto Archive=[&](EFrontlineFaction Nation)
  {Clear();B->Phase=EFrontlinePhase::Briefing;Check(TEXT("v833_ui_campaign"),B->StartNewCampaign(Nation,EFrontlineDifficulty::Veteran));
   if(auto* C=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))
   {C->CompletedMissions=19;C->WonOperationMaskV52=(1u<<19)-1;C->DispatchResolvedMaskV52=(1u<<20)-1;
    C->Research.TechnologyMask=0;C->ArmourResearchMaskV63=0;C->Research.ResearchPoints=60;}
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);};
  const auto Technology=[&](int32 Page,bool ZH)
  {Clear();FrontlineLocalizationV35::SetLanguage(ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
   Check(TEXT("v833_authored_archive_display_material_loaded"),Artwork().ArchivePhotoMaterialV72&&Artwork().ArchivePhotoMaterialV72->GetName()==TEXT("M_ArchivePhoto_V833"));
   S.Technology=true;S.TechnologyPage=Page;S.TechnologyFocus=INDEX_NONE;};
  const auto WarRoom=[&](bool ZH)
  {Clear();FrontlineLocalizationV35::SetLanguage(ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
   auto& W=WarRoom52::State(PC);WarRoom52::Synchronize(W,B);W.FlightStart=Now-10;WarRoom52::AtlasTick(W,Now);W.WasActive=true;};
  const auto Shot=[&](const TCHAR* Name)
  {FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(FString(Name)+TEXT(".png")),true,false);++R->Shots;};
  double Delay=.75;
  switch(R->Stage)
  {
   case 0:
    Clear();B->Phase=EFrontlinePhase::Briefing;Check(TEXT("v833_real_battle_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
    if(auto* C=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))
    {C->CompletedMissions=19;C->WonOperationMaskV52=(1u<<19)-1;C->DispatchResolvedMaskV52=(1u<<20)-1;
     C->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;C->Research.ResearchPoints=60;}
    if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
    {Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);
     Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);}
    Check(TEXT("v833_current_operation"),B->SelectCampaignMission(16));S.AcceptedLoadoutMission=INDEX_NONE;
    BeginOperationPreparation(PC,B);R->Prepared=Now;break;
   case 1:
    if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<65.)
    {R->Next=Now+.2;return true;}
    Check(TEXT("v833_actual_battle_loaded"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty());
    if(!R->Good)return Finish();B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);Clear();break;
   case 2:
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV836Review")))break;
    Write(TEXT("anti-armour.json"),FrontlineCombatV64::RunAntiArmourChecksV833(B,Check));
    if(Baseline)return Finish();break;
   case 3:
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV836Review")))
    {
     FrontlineArmourV63::RunAllocationChecksV836(B,Check);
     FrontlineDebugV37::RunSpeedChecksV836(B,Check);
     FrontlineMotionV40::RunFocusedChecks(B,Check);
     FrontlineAchievementsV76::RunIsolatedChecks(B,Check);
     for(int32 I=FrontlineAchievementsV76::TributeStartV836;I<FrontlineAchievementsV76::Count;++I)
       Check(FString::Printf(TEXT("v836_tribute_badge_%d_cooked_and_loadable"),I),LoadObject<UTexture2D>(nullptr,*FrontlineAchievementsV76::BadgeObjectPathV82(I))!=nullptr);
     FrontlineArmourValourV8221::RunChecks(Check);MedalWallV8215::RunDeedGeometryChecksV83(Check);
     for(int32 N=0;N<4;++N){Check(FString::Printf(TEXT("v836_national_valour_badge_%d_cooked_and_loadable"),N),LoadObject<UTexture2D>(nullptr,*FrontlineArmourValourV8221::BadgePath(EFRRUNationV81(N)))!=nullptr);
       Check(FString::Printf(TEXT("v836_national_valour_ribbon_%d_cooked_and_loadable"),N),LoadObject<UTexture2D>(nullptr,*FrontlineArmourValourV8221::RibbonPath(EFRRUNationV81(N)))!=nullptr);}
     Clear();B->Phase=EFrontlinePhase::Fighting;
     PC->ToggleAchievementsV76();Check(TEXT("v836_battle_honours_page_owns_automatic_pause"),S.AchievementsV76&&UGameplayStatics::IsGamePaused(PC));
     PC->ToggleAchievementsV76();Check(TEXT("v836_battle_honours_return_releases_only_owned_pause"),!S.AchievementsV76&&!UGameplayStatics::IsGamePaused(PC));
     UGameplayStatics::SetGamePaused(PC,true);PC->ToggleAchievementsV76();PC->ToggleAchievementsV76();
     Check(TEXT("v836_battle_honours_preserves_explicit_pause"),UGameplayStatics::IsGamePaused(PC));Clear();
     Check(TEXT("v836_battle_medal_picker_opens"),MedalMountV8216::Hit(PC,B,TEXT("V8216MedalMount")));
     Check(TEXT("v836_battle_medal_picker_owns_automatic_pause"),MedalMountV8216::View(PC).Picker&&UGameplayStatics::IsGamePaused(PC));
     PC->TogglePause();Check(TEXT("v836_medal_picker_space_cannot_release_owned_pause"),MedalMountV8216::View(PC).Picker&&UGameplayStatics::IsGamePaused(PC));
     MedalMountV8216::Back(PC);Check(TEXT("v836_medal_picker_back_resumes_only_owned_pause"),!MedalMountV8216::View(PC).Picker&&!UGameplayStatics::IsGamePaused(PC));
     UGameplayStatics::SetGamePaused(PC,true);MedalMountV8216::Hit(PC,B,TEXT("V8216MedalMount"));MedalMountV8216::Back(PC);
     Check(TEXT("v836_medal_picker_preserves_explicit_pause"),UGameplayStatics::IsGamePaused(PC));Clear();
     Write(TEXT("armour-v836.json"),RunFrontlineArmourV836Review(B,Check));
     Write(TEXT("support-v836.json"),RunFrontlineSupportV836Review(B,Check));
     Write(TEXT("contact-v835.json"),RunFrontlineContactV835Review(B,Check));
     Check(TEXT("v836_headquarters_asset_preload_has_completed"),FrontlineMapSceneV61::ContinuePreloadReadyV8214(PC));
     R->Stage=25;break;
    }
    FrontlineFieldV32::RunEngineerMineChecksV833(B,Check);
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV835Review")))
    {
     NewUnlockUIV8215::RunNavigationChecksV835(B,Check);
     AWarfareFrontlineImpact::RunDebrisChecksV835(B,Check);
     Write(TEXT("contact-v835.json"),RunFrontlineContactV835Review(B,Check));
     auto Partial=MakeShared<FJsonObject>();Partial->SetBoolField(TEXT("success"),R->Good);
     Partial->SetArrayField(TEXT("checks"),R->Checks);Write(TEXT("logic-checks-v835.json"),Partial);
     // This round changes contact/action/debris, not archival artwork. Avoid
     // replaying all old research screens before the actual action captures.
     R->Stage=19;
    }
    break;
   case 4:Archive(EFrontlineFaction::British);Technology(0,false);break;
   case 5:Shot(TEXT("01-uk-infantry-archive-en"));break;
   case 6:Technology(1,true);break;
   case 7:Shot(TEXT("02-uk-attachments-archive-zh"));break;
   case 8:Technology(2,false);break;
   case 9:Shot(TEXT("03-uk-armour-archive-en"));break;
   case 10:WarRoom(true);Delay=1.2;break;
   case 11:Shot(TEXT("04-uk-headquarters-zh"));break;
   case 12:Archive(EFrontlineFaction::German);Technology(0,true);break;
   case 13:Shot(TEXT("05-de-infantry-archive-zh"));break;
   case 14:Technology(1,false);break;
   case 15:Shot(TEXT("06-de-attachments-archive-en"));break;
   case 16:Technology(2,true);break;
   case 17:Shot(TEXT("07-de-armour-archive-zh"));break;
   case 18:WarRoom(false);Delay=1.2;break;
   case 19:Shot(TEXT("08-de-headquarters-en"));break;
   case 20:
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV835Review"))){Clear();return Finish();}
    Clear();B->Phase=EFrontlinePhase::Fighting;
    if(auto* Worker=FrontlineFieldV32::PrepareMineWorkVisualReviewV835(B))
    {
     Check(TEXT("v835_actual_mine_work_visual_fixture"),true);
     PC->ResetCamera();
     const FVector Focus=Worker->Feet()+FVector(0,0,50);
     PC->BattleCamera->SetActorLocation(Focus+FVector(0,4000,1550));
     PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());
     PC->BattleCamera->GetCameraComponent()->OrthoWidth=1800.f;PC->SetViewTarget(PC->BattleCamera);
     Write(TEXT("mine-work-pose-v835.json"),FrontlineMotionV40::MineWorkReviewStateV835(Worker));
     UGameplayStatics::SetGamePaused(PC,true);Delay=1.2;
    }
    else Check(TEXT("v835_actual_mine_work_visual_fixture"),false);
    break;
   case 21:Shot(TEXT("09-actual-engineer-mine-work"));break;
   case 22:
    {
     Clear();B->Phase=EFrontlinePhase::Fighting;
     const FVector OldFocus=PC->BattleCamera->GetActorLocation()-FVector(0,4000,1550);
     const FVector Focus=OldFocus+FVector(850,0,0);
     PC->BattleCamera->SetActorLocation(Focus+FVector(0,6000,2400));
     PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());
     PC->BattleCamera->GetCameraComponent()->OrthoWidth=4200.f;
     FVector Point=Focus;Point.Z=FrontlineInfantryV45::SurfaceHeight(B,Point)+3.f;
     auto* Blast=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(AWarfareFrontlineImpact::StaticClass(),Point,FRotator::ZeroRotator);
     Check(TEXT("v835_live_debris_visual_actor"),Blast!=nullptr);
     if(Blast){Blast->ConfigureBlast(false,true);R->LiveBlast=Blast;}
     Delay=.12;break;
    }
   case 23:
    {
     if(auto* Blast=R->LiveBlast.Get())Write(TEXT("live-blast-v835.json"),Blast->DescribeCompositionV8221());
     Shot(TEXT("10-live-mortar-fragments"));Delay=.30;break;
    }
   case 24:
    {
     auto Shutter=AWarfareFrontlineImpact::DescribeFragmentShutterV835(B->GetWorld());
     Check(TEXT("v835_object_shutter_applied_to_actual_unpaused_battle_views"),Shutter->GetNumberField(TEXT("actual_battle_views_applied"))>0);
     Write(TEXT("fragment-shutter-v835.json"),Shutter);Shot(TEXT("11-live-mortar-dust"));break;
    }
   case 26:
    {
     Clear();B->Phase=EFrontlinePhase::Fighting;
     Check(TEXT("v836_actual_british_vehicle_visual_fixture"),PrepareFrontlineArmourVisualV836(B,false));
     const FVector Focus(0,0,160);PC->BattleCamera->SetActorLocation(Focus+FVector(0,6500,2400));
     PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());
     PC->BattleCamera->GetCameraComponent()->OrthoWidth=7500;PC->SetViewTarget(PC->BattleCamera);Delay=1.1;break;
    }
   case 27:
    {
     auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld());
     const auto Stats=Fire?Fire->VisualStateV8214():FFrontlineFuelVisualStatsV8214{};
     Check(TEXT("v836_actual_wreck_uses_ready_active_volumetric_flame"),Fire&&Fire->IsReady()&&Stats.FlameCoresV836>0);
     auto FX=MakeShared<FJsonObject>();FX->SetNumberField(TEXT("active_flame_cores"),Stats.FlameCoresV836);
     FX->SetNumberField(TEXT("core_budget"),AFrontlineFuelFireV33::FlameCoreBudgetV836);
     FX->SetNumberField(TEXT("contact_budget"),AFrontlineFuelFireV33::ContactBudgetV836);
     FX->SetNumberField(TEXT("retained_soot_volumes"),Stats.VolumeSootV8216);
     for(TActorIterator<AFrontlineTank> It(B->GetWorld());It;++It)if(!It->IsOperational())
     {
      const auto Wreck=FFrontlineArmourWreckV8220Access::Describe(*It);
      Check(TEXT("v836_actual_whippet_wreck_body_geometry_replaced"),Wreck->GetBoolField(TEXT("body_geometry_replaced")));
      Check(TEXT("v836_actual_whippet_wreck_body_surfaces_complete"),Wreck->GetBoolField(TEXT("body_surface_complete")));
      Write(TEXT("actual-whippet-wreck-v836.json"),Wreck);
     }
     Write(TEXT("actual-flame-volume-v836.json"),FX);Shot(TEXT("12-uk-armour-mechanical-finish"));break;
    }
   case 28:
    {
     Check(TEXT("v836_actual_german_rail_vehicle_visual_fixture"),PrepareFrontlineArmourVisualV836(B,true));
     const FVector Focus(-1500,0,160);PC->BattleCamera->SetActorLocation(Focus+FVector(0,7600,2750));
     PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());
     PC->BattleCamera->GetCameraComponent()->OrthoWidth=11200;Delay=1.;break;
    }
   case 29:
    for(TActorIterator<AFrontlineTank> It(B->GetWorld());It;++It)if(!It->IsOperational())
    {
     const auto Wreck=FFrontlineArmourWreckV8220Access::Describe(*It);
     Check(TEXT("v836_actual_beute_wreck_body_geometry_replaced"),Wreck->GetBoolField(TEXT("body_geometry_replaced")));
     Check(TEXT("v836_actual_beute_wreck_body_surfaces_complete"),Wreck->GetBoolField(TEXT("body_surface_complete")));
     Write(TEXT("actual-beute-wreck-v836.json"),Wreck);
    }
    Shot(TEXT("13-de-armour-rail-finish"));break;
   case 30:
    {
     const FVector Focus(0,550,100);Check(TEXT("v836_actual_gas_visual_fixture"),PrepareFrontlineGasVisualV836(B,Focus));
     PC->BattleCamera->SetActorLocation(Focus+FVector(0,5200,1900));
     PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());
     PC->BattleCamera->GetCameraComponent()->OrthoWidth=4200;Delay=.4;break;
    }
   case 31:Shot(TEXT("14-rounded-gas-volumes"));break;
   case 32:
    {
     for(TActorIterator<AFrontlineSupportMissionV28> It(B->GetWorld());It;++It)It->Destroy();
     FVector P(650,0,0);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P)+3;
     auto* Blast=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(AWarfareFrontlineImpact::StaticClass(),P,FRotator::ZeroRotator);
     Check(TEXT("v836_actual_taller_aftermath_smoke_fixture"),Blast!=nullptr);if(Blast){Blast->ConfigureBlast(false,true);R->LiveBlast=Blast;}
     Delay=.52;break;
    }
   case 33:Shot(TEXT("15-rounded-explosion-aftermath"));break;
   case 34:
    {
     Check(TEXT("v836_actual_vehicle_detail_fixture"),PrepareFrontlineArmourVisualV836(B,false));
     const FVector Focus(2000,450,160);PC->BattleCamera->SetActorLocation(Focus+FVector(0,3500,1500));
     PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());
     PC->BattleCamera->GetCameraComponent()->OrthoWidth=2100;Delay=.8;break;
    }
   case 35:Shot(TEXT("16-uk-armour-finish-detail"));break;
   case 36:
    {
     Clear();B->AbandonBattleToOperations();WarRoom(false);
     FrontlineMapSceneV61::PreloadContinueV8214(PC);PC->SetActorTickEnabled(true);
     R->Prepared=Now;Delay=.2;break;
    }
   case 37:
    {
     const bool Ready=FrontlineMapSceneV61::ContinuePreloadReadyV8214(PC)
       &&FrontlineMapSceneV61::IsAtlasReadyV64(PC)&&FrontlineMapSceneV61::ContinueViewWarmedV8214(PC);
     if(!Ready&&Now-R->Prepared<15.){R->Next=Now+.2;return true;}
     Check(TEXT("v836_actual_headquarters_complete_atlas_and_render_warmth"),Ready);
     Shot(TEXT("17-headquarters-complete-map"));PC->SetActorTickEnabled(false);break;
    }
   case 38:
    {
     Clear();FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);
     S.AchievementsV76=true;HonoursListV82::Reveal(S,FrontlineAchievementsV76::Count-1,true);Delay=.7;break;
    }
   case 39:Shot(TEXT("18-tribute-honours-list-zh"));break;
   case 40:
    {
     auto& Wall=MedalWallV8215::View(PC);Wall.Wall=true;Wall.Nation=int32(EFRRUNationV81::German);
     Wall.Focus=MedalWallV8215::DeedBaseV8222+Wall.Nation;Wall.Row=2;Delay=.7;break;
    }
   case 41:Shot(TEXT("19-german-independent-valour-art"));break;
   case 42:
    {
     Clear();S.Dossier=1918;auto& Selection=S.FactionSelectionV82;Selection.Selected=0;
     Selection.Widths[0]=1920;for(int32 I=1;I<4;++I)Selection.Widths[I]=0;
     Selection.CommitAt=Now-.9;Delay=.1;break;
    }
   case 43:Shot(TEXT("20-uniform-faction-blackout"));break;
   default:Clear();return Finish();
  }
  ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
 }),0.f);
}
}
