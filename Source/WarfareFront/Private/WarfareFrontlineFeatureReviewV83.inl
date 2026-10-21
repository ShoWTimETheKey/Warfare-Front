#include "WarfareFrontlineWaterReviewV83.h"

namespace
{
void StartFrontlineFeatureReviewV83(AWarfareFrontlineController* Controller)
{
 if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV83Review"))
  ||!FParse::Param(FCommandLine::Get(),TEXT("WFV8221Review"))
  ||!FParse::Param(FCommandLine::Get(),TEXT("WFV8222Review")))return;
 FString User;if(!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun
 {
  int32 Stage=0,Shots=0;bool Good=true,WaterChanged=false;
  double Began=FPlatformTime::Seconds(),Next=Began+2.,Prepared=0;uint64 DrawAt=0;
  FString Dir,CostMode;FVector WaterFocus=FVector::ZeroVector;TArray<TSharedPtr<FJsonValue>> Checks,Costs;
  double SampleFrom=0,SampleUntil=0,SampleLast=0,WallMs=0,GameMs=0,RenderMs=0,RHIMs=0,GPUMs=0;
  int32 Samples=0;
 };
 auto R=MakeShared<FRun>();R->Dir=User;
 IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  if(R->SampleFrom>0&&Now>=R->SampleFrom&&Now<=R->SampleUntil)
  {
   if(R->SampleLast>0)
   {
    R->WallMs+=(Now-R->SampleLast)*1000.;R->GameMs+=FPlatformTime::ToMilliseconds(GGameThreadTime);
    R->RenderMs+=FPlatformTime::ToMilliseconds(GRenderThreadTime);R->RHIMs+=FPlatformTime::ToMilliseconds(GRHIThreadTime);
    R->GPUMs+=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());++R->Samples;
   }
   R->SampleLast=Now;
  }
  auto Check=[R](const FString& Name,bool Pass)
  {
   auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);
   R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;
   UE_LOG(LogTemp,Display,TEXT("WF_V83_CHECK %s %d"),*Name,int32(Pass));
  };
  auto Finish=[&]()
  {
   if(R->WaterChanged&&B){FVector Focus;Check(TEXT("v83_water_finish_restores_scene"),FrontlineWaterReviewV83::SetContribution(B,FrontlineWaterReviewV83::EContribution::Restore,Focus));R->WaterChanged=false;}
   V28ReviewPointer.Reset();UIMotionReviewV83::TooltipAge=-1;
   auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);
   J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetArrayField(TEXT("water_cost_samples"),R->Costs);
   J->SetStringField(TEXT("scope"),TEXT("V83 focused actual Shipping honours lifecycle and rollback checks, four-army selector drawing fixtures, early British/German armour allocation, four national deed inspection pages, and paired existing British battlefield water contributions. Selector state is an isolated presentation fixture, not authentic human hold acceptance. French/Russian campaigns remain closed; no real player records or whole-campaign/performance claim."));
   FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("checks.json")));
   FPlatformMisc::RequestExit(false);return false;
  };
  if(!B||!FrontlineEffectsReviewSafetyV8216::Allowed(B)){Check(TEXT("v83_strict_disposable_world"),false);return Finish();}
  if(Now-R->Began>150.){Check(TEXT("v83_bounded_review"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&]()
  {
   UGameplayStatics::SetGamePaused(PC,false);
   S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
   S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.Dossier=1917;
   S.FactionSelectionV82=FFrontlineFactionSelectionV82();
   MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::View(PC).Picker=false;
   PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);UIMotionReviewV83::TooltipAge=-1;
  };
  auto Language=[&](bool ZH){FrontlineLocalizationV35::SetLanguage(ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);};
  auto Shot=[&](const TCHAR* Name)
  {
   if(R->SampleFrom>0)
   {
    Check(R->CostMode+TEXT("_short_cost_sample_available"),R->Samples>0);
    auto Cost=MakeShared<FJsonObject>();Cost->SetStringField(TEXT("mode"),R->CostMode);Cost->SetNumberField(TEXT("samples"),R->Samples);
    Cost->SetNumberField(TEXT("settle_seconds"),1.5);Cost->SetNumberField(TEXT("sampling_window_seconds"),.5);
    const double Divisor=FMath::Max(1,R->Samples);
    Cost->SetNumberField(TEXT("wall_ms"),R->WallMs/Divisor);Cost->SetNumberField(TEXT("game_ms"),R->GameMs/Divisor);
    Cost->SetNumberField(TEXT("render_ms"),R->RenderMs/Divisor);Cost->SetNumberField(TEXT("rhi_ms"),R->RHIMs/Divisor);Cost->SetNumberField(TEXT("gpu_ms"),R->GPUMs/Divisor);
    Cost->SetStringField(TEXT("scope"),TEXT("Short contribution-cost comparison in one frozen battle camera; not sustained gameplay, frame-generation or hardware acceptance."));
    R->Costs.Add(MakeShared<FJsonValueObject>(Cost));R->SampleFrom=0;
   }
   FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(FString(Name)+TEXT(".png")),true,false);++R->Shots;
  };
  auto CleanCampaign=[&](EFrontlineFaction F,const FString& Key)
  {
   Clear();B->Phase=EFrontlinePhase::Briefing;
   const bool Started=B->StartNewCampaign(F,EFrontlineDifficulty::Veteran);
   Check(Key,Started&&B->GetCurrentCampaign()&&!FrontlineDebugV37::HasCheatState(B)&&!WarfareFRRUCampaignV82::Active(B));
   return Started;
  };
  auto Selector=[&](int32 Army,bool ZH)
  {
   Clear();Language(ZH);S.Dossier=1918;auto& F=S.FactionSelectionV82;
   F.Visible=true;F.Selected=Army;F.Hover=Army;F.FreshInput=true;F.PointerOwnsSelectionV826=false;
   F.LastTick=Now;F.LayoutFrame=GFrameCounter;
   for(int32 I=0;I<4;++I)F.Widths[I]=Army<0?480.f:I==Army?768.f:384.f;
   // HUD rendering remains real. Only controller polling is suspended, so an
   // offscreen review cannot impersonate or loosen authentic production input.
   PC->SetActorTickEnabled(false);
  };
  auto Allocation=[&](EFrontlineFaction F,bool ZH,const FString& Key)
  {
   CleanCampaign(F,Key+TEXT("_clean_campaign"));Language(ZH);S.Deployment=true;S.DeploymentTabV63=1;
   R->DrawAt=GFrameCounter;
  };
  auto CheckAllocation=[&](const FString& Key)
  {
   const auto& Last=ArmourDeploymentUIV83::Last;bool AllLocked=!Last.Cards.IsEmpty(),Centred=AllLocked,Inside=AllLocked;
   for(const auto& C:Last.Cards)
   {
    AllLocked&=!C.Eligible&&!C.Caption&&!C.Variant;
    Centred&=C.Bounds.GetCenter().Equals(C.LockBounds.GetCenter(),.01f);
    Inside&=C.LockBounds.Min.X>=C.Bounds.Min.X&&C.LockBounds.Min.Y>=C.Bounds.Min.Y
      &&C.LockBounds.Max.X<=C.Bounds.Max.X&&C.LockBounds.Max.Y<=C.Bounds.Max.Y;
   }
   Check(Key+TEXT("_actual_allocation_page_drawn"),Last.Canvas&&Last.Frame>R->DrawAt);
   Check(Key+TEXT("_early_cards_locked_without_name_or_variant"),AllLocked);
   Check(Key+TEXT("_locks_centred_inside_full_cards"),Centred&&Inside);
  };
  auto Deed=[&](int32 Nation,bool ZH)
  {
   if(Nation<2)
   {
    CleanCampaign(EFrontlineFaction(Nation),FString::Printf(TEXT("v83_deed_%d_clean_campaign"),Nation));
    const int32 Before=FrontlineArmourKillTallyV8220::Count(EFRRUNationV81(Nation));
    Check(FString::Printf(TEXT("v83_deed_%d_genuine_fatal_hull_fixture"),Nation),FrontlineArmourKillTallyV8220::SeedNotificationReview(B)
      &&FrontlineArmourKillTallyV8220::Count(EFRRUNationV81(Nation))==Before+1);
    FrontlineArmourKillTallyV8220::TakeNotification();
   }
   Clear();Language(ZH);S.AchievementsV76=true;auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Nation=Nation;
   const int32 Id=MedalWallV8215::DeedBaseV8222+Nation;
   MedalWallV8215::Hit(PC,FName(*FString::Printf(TEXT("V8216MedalSelect%d"),Id)));
   MedalWallV8215::Hit(PC,TEXT("V8216MedalInspect"));
   Check(FString::Printf(TEXT("v83_deed_%d_regular_inspection_selected"),Nation),M.Focus==Id&&M.Inspect&&FrontlineTextureReady(MedalWallV8215::Art(Id,1)));
   if(Nation>=2)Check(FString::Printf(TEXT("v83_deed_%d_closed_ui_has_no_campaign_or_award"),Nation),!WarfareFRRUCampaignV82::Active(B)
     &&FrontlineArmourKillTallyV8220::Count(EFRRUNationV81(Nation))==0&&!FactionSelectionV82::Available(Nation==2?1:3));
  };
  auto Water=[&](FrontlineWaterReviewV83::EContribution Mode,const FString& Key)
  {
   FVector Focus;const bool Changed=FrontlineWaterReviewV83::SetContribution(B,Mode,Focus);
   Check(Key,Changed);if(Changed)
   {
    R->WaterChanged=Mode!=FrontlineWaterReviewV83::EContribution::Restore;
    if(Mode==FrontlineWaterReviewV83::EContribution::Baseline)
    {R->WaterFocus=Focus;FFrontlineReviewV32::Camera(PC,Focus.X,Focus.Y,2200.f,Focus.Z+650.f,true);}
    else if(Mode!=FrontlineWaterReviewV83::EContribution::Restore)
     Check(Key+TEXT("_same_camera_focus"),Focus.Equals(R->WaterFocus,.01f)&&PC->BattleCamera&&FMath::IsNearlyEqual(PC->BattleCamera->GetCameraComponent()->OrthoWidth,2200.f,.01f));
    if(Mode!=FrontlineWaterReviewV83::EContribution::Restore)
    {
     R->CostMode=Key;R->Samples=0;R->SampleLast=0;R->WallMs=R->GameMs=R->RenderMs=R->RHIMs=R->GPUMs=0;
     R->SampleFrom=FPlatformTime::Seconds()+1.5;R->SampleUntil=R->SampleFrom+.5;
    }
   }
   return Changed;
  };
  double Delay=.65;
  switch(R->Stage)
  {
   case 0:
   {
    FactionSelectionV82::Preload();bool Ready=FactionSelectionV82::PreloadSettled()&&FactionSelectionV82::Images().Failure.IsEmpty();
    for(int32 N=0;N<4;++N)Ready&=FrontlineTextureReady(ArmourKillTallyUIV8220::Badge(EFRRUNationV81(N)));
    if(!Ready&&Now-R->Began<35.){R->Next=Now+.1;return true;}
    Check(TEXT("v83_selector_and_four_emblems_rhi_ready"),Ready);if(!Ready)return Finish();
    CleanCampaign(EFrontlineFaction::British,TEXT("v83_initial_clean_british_campaign"));
    FrontlineAchievementsV76::RunHonoursLifecycleChecksV83(B,Check);
    MedalWallV8215::RunDeedGeometryChecksV83(Check);DeploymentCardV83::RunChecks(Check);
    break;
   }
   case 1:Selector(INDEX_NONE,false);break;
   case 2:Check(TEXT("v83_initial_selector_has_four_equal_unselected_panes"),S.FactionSelectionV82.Selected==INDEX_NONE&&S.FactionSelectionV82.Widths[0]==480.f&&S.FactionSelectionV82.Widths[3]==480.f);Shot(TEXT("01-selector-four-grey-en"));break;
   case 3:Selector(0,false);break;
   case 4:Shot(TEXT("02-selector-british-controls-en"));break;
   case 5:Selector(2,true);break;
   case 6:Shot(TEXT("03-selector-german-controls-zh"));break;
   case 7:
    S.FactionSelectionV82.CommitAt=Now-.98;S.FactionSelectionV82.CommitPainted=false;
    for(int32 I=0;I<4;++I){S.FactionSelectionV82.CommitWidths[I]=S.FactionSelectionV82.Widths[I];S.FactionSelectionV82.Widths[I]=I==2?1920.f:0.f;}Delay=.15;break;
   case 8:Check(TEXT("v83_confirmation_partial_black_draw_fixture"),FactionSelectionV82::CommitBlackV83(Now-S.FactionSelectionV82.CommitAt)>.15f&&FactionSelectionV82::CommitBlackV83(Now-S.FactionSelectionV82.CommitAt)<.85f);Shot(TEXT("04-confirmation-partial-black-zh"));break;
   case 9:S.FactionSelectionV82.CommitAt=Now-1.92;S.FactionSelectionV82.CommitPainted=false;R->DrawAt=GFrameCounter;Delay=.15;break;
   case 10:
    // Core ticker can run after this frame's HUD draw. Production input ticks
    // retain their strict previous-frame test; this drawing fixture instead
    // verifies that Draw actually recorded its guard after the state change.
    Check(TEXT("v83_full_black_opaque_and_actual_draw_guard_recorded"),FactionSelectionV82::CommitBlackV83(Now-S.FactionSelectionV82.CommitAt)==1.f
      &&S.FactionSelectionV82.CommitPainted&&S.FactionSelectionV82.PaintedFrame>=R->DrawAt&&S.FactionSelectionV82.PaintedFrame<=GFrameCounter
      &&FactionSelectionV82::LastCommitPaintedFrameForInputReview==S.FactionSelectionV82.PaintedFrame
      &&FactionSelectionV82::LastCommitPaintedArmyForInputReview==2);
    Shot(TEXT("05-confirmation-full-black-zh"));break;
   case 11:Allocation(EFrontlineFaction::British,false,TEXT("v83_british_allocation"));break;
   case 12:CheckAllocation(TEXT("v83_british_allocation"));Shot(TEXT("06-british-early-armour-allocation-en"));break;
   case 13:Allocation(EFrontlineFaction::German,true,TEXT("v83_german_allocation"));break;
   case 14:CheckAllocation(TEXT("v83_german_allocation"));Shot(TEXT("07-german-early-armour-allocation-zh"));break;
   case 15:Deed(0,false);MedalWallV8215::View(PC).Inspect=false;break;
   case 16:Shot(TEXT("08-british-deed-wall-detail-en"));break;
   case 17:MedalWallV8215::Hit(PC,TEXT("V8216MedalInspect"));break;
   case 18:Shot(TEXT("09-british-deed-inspection-en"));break;
   case 19:Deed(1,true);break;
   case 20:Shot(TEXT("10-german-deed-inspection-zh"));break;
   case 21:Deed(2,true);break;
   case 22:Shot(TEXT("11-french-closed-deed-inspection-zh"));break;
   case 23:Deed(3,false);break;
   case 24:Shot(TEXT("12-russian-closed-deed-inspection-en"));break;
   case 25:
    CleanCampaign(EFrontlineFaction::British,TEXT("v83_water_clean_british_campaign"));Language(false);PC->SetActorTickEnabled(true);
    if(auto* C=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))
    {C->CompletedMissions=19;C->WonOperationMaskV52=(1u<<19)-1;C->DispatchResolvedMaskV52=(1u<<20)-1;C->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;C->Research.ResearchPoints=60;}
    Check(TEXT("v83_existing_british_water_operation_fixture"),B->SelectCampaignMission(16));
    S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;Delay=.1;break;
   case 26:
    if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<55.){R->Next=Now+.1;return true;}
    Check(TEXT("v83_actual_prepared_british_battle"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty());
    if(B->Phase!=EFrontlinePhase::Fighting||S.Preparing||!B->AssetFailure.IsEmpty())return Finish();
    B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);Clear();S.Dossier=0;
    FrontlineWaterReviewV83::RunChecks(B,Check);
    if(!Water(FrontlineWaterReviewV83::EContribution::Baseline,TEXT("v83_water_baseline_contribution")))return Finish();Delay=2.05;break;
   case 27:Shot(TEXT("13-water-baseline"));break;
   case 28:if(!Water(FrontlineWaterReviewV83::EContribution::Hidden,TEXT("v83_water_hidden_contribution")))return Finish();Delay=2.05;break;
   case 29:Shot(TEXT("14-water-hidden"));break;
   case 30:if(!Water(FrontlineWaterReviewV83::EContribution::NoReflection,TEXT("v83_water_no_reflection_contribution")))return Finish();Delay=2.05;break;
   case 31:Shot(TEXT("15-water-no-reflection"));break;
   case 32:if(!Water(FrontlineWaterReviewV83::EContribution::Candidate,TEXT("v83_water_candidate_contribution")))return Finish();Delay=2.05;break;
   case 33:Shot(TEXT("16-water-candidate"));break;
   default:
    Water(FrontlineWaterReviewV83::EContribution::Restore,TEXT("v83_water_original_material_mesh_and_priority_restored"));
    Check(TEXT("v83_sixteen_requested_images"),R->Shots==16);return Finish();
  }
  ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
 }),0.f);
}
}
