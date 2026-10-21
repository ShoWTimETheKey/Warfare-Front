// View history only. No recruitment, research, campaign selection or destructive
// command is replayed by the operating system's XBUTTON navigation keys.
namespace NavigationV61
{
struct FPage
{
 FGuid Campaign;EFrontlinePhase Phase;int32 Dossier,Tab,TechPage;
 bool Menu,Help,Credits,Settings,Technology,Deployment,Achievements,FRRUArchives;
 int32 HonourFocus=0;float HonourScroll=0;
 WarRoom52::FState Map;
 bool NationalMap=false;FRRUWarRoomV82::FNavigationView NationalView;
};
struct FHistory {TWeakObjectPtr<AWarfareFrontlineController> Owner;TArray<FPage> Back,Forward;bool Restoring=false;int32 Depth=0;};
FHistory& History(AWarfareFrontlineController* PC){static FHistory H;if(H.Owner.Get()!=PC){H={};H.Owner=PC;}return H;}
FPage Capture(AWarfareFrontlineController* PC)
{
 auto& S=ScreenState(PC);auto* B=FrontlineFor(PC);FPage P{};
 P.Campaign=B&&WarfareFRRUCampaignV82::Active(B)?WarfareFRRUCampaignV82::CampaignId(B):B&&B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();P.Phase=B?B->Phase:EFrontlinePhase::Briefing;
 P.FRRUArchives=S.FRRUArchivesV82;
 P.Dossier=S.Dossier;P.Tab=S.SettingsTab;P.TechPage=S.TechnologyPage;P.Menu=S.Menu;P.Help=S.Help;P.Credits=S.Credits;
 P.Settings=S.Settings;P.Technology=S.Technology;P.Deployment=S.Deployment;P.Achievements=S.AchievementsV76;P.Map=WarRoom52::State(PC);
 P.HonourFocus=S.AchievementFocusV76;P.HonourScroll=S.AchievementScrollTargetV82;
 P.NationalMap=B&&WarfareFRRUCampaignV82::Active(B)&&B->Phase==EFrontlinePhase::Briefing;
 if(P.NationalMap)P.NationalView=FRRUWarRoomV82::CaptureNavigation(PC);return P;
}
bool Same(const FPage& A,const FPage& B)
{return A.Campaign==B.Campaign&&A.Phase==B.Phase&&A.Dossier==B.Dossier&&A.Tab==B.Tab&&A.TechPage==B.TechPage&&A.Menu==B.Menu&&A.Help==B.Help&&A.Credits==B.Credits&&A.Settings==B.Settings&&A.Technology==B.Technology&&A.Deployment==B.Deployment&&A.Achievements==B.Achievements&&A.FRRUArchives==B.FRRUArchives&&A.NationalMap==B.NationalMap
 &&(A.NationalMap?FRRUWarRoomV82::SameNavigation(A.NationalView,B.NationalView):A.Map.AtlasMode==B.Map.AtlasMode&&A.Map.AtlasSector==B.Map.AtlasSector&&A.Map.Preview==B.Map.Preview);}
bool Allowed(AWarfareFrontlineController* PC)
{const auto& S=ScreenState(PC);return S.Settings||(!S.Preparing&&!S.UnitIntroduction&&!S.CampaignEndingV51&&!S.HistoricalDispatch&&S.ArchivePendingDelete.IsEmpty()&&!S.ExitConfirmV61);}
struct FRecord
{
 AWarfareFrontlineController* PC;FPage Before;bool Valid;
 FRecord(AWarfareFrontlineController* C):PC(C),Before(Capture(C)),Valid(Allowed(C)&&!History(C).Restoring&&History(C).Depth==0){++History(C).Depth;}
 ~FRecord(){--History(PC).Depth;if(!Valid||!Allowed(PC))return;auto& H=History(PC);const auto After=Capture(PC);
  if(Before.Campaign!=After.Campaign||Before.Phase!=After.Phase){H.Back.Empty();H.Forward.Empty();return;}
  if(!Same(Before,After)){H.Back.Add(Before);if(H.Back.Num()>24)H.Back.RemoveAt(0);H.Forward.Empty();}}
};
bool Restore(AWarfareFrontlineController* PC,const FPage& P)
{
 auto& S=ScreenState(PC);auto* B=FrontlineFor(PC);const auto Current=Capture(PC);
 if(!B||P.Campaign!=Current.Campaign||P.Phase!=Current.Phase)return false;
 if(P.NationalMap!=Current.NationalMap)return false;
 if(P.NationalMap&&!FRRUWarRoomV82::RestoreNavigation(PC,P.NationalView,P.Dossier==1917&&!P.Menu&&!P.Help&&!P.Credits&&!P.Settings&&!P.Technology&&!P.Deployment&&!P.Achievements))return false;
 if(S.Settings&&!P.Settings){FinishVideoChange(S,false);if(auto* R=UFrontlineRenderSettingsSubsystem::Get(PC->GetWorld()))R->SetHDRCalibrationVisible(false);}
 if(P.Settings&&!S.Settings)LoadDisplayChoices(S);
 const bool WasOverlay=S.Menu||S.Help||S.Settings||S.Technology||S.AchievementsV76;
 const bool OwnedPause=S.bPausedByOverlay||S.bPausedForAuxOverlay;
 S.Dossier=P.Dossier;S.SettingsTab=P.Tab;S.TechnologyPage=P.TechPage;S.Menu=P.Menu;S.Help=P.Help;S.Credits=P.Credits;S.Settings=P.Settings;S.Technology=P.Technology;S.Deployment=P.Deployment;S.AchievementsV76=P.Achievements;
 S.FRRUArchivesV82=P.FRRUArchives;
 S.AchievementFocusV76=FMath::Clamp(P.HonourFocus,0,FrontlineAchievementsV76::Count-1);
 S.AchievementScrollV82=S.AchievementScrollTargetV82=FMath::Clamp(P.HonourScroll,0.f,HonoursListV82::Maximum());
 S.ArchiveSelection.Empty();if(!(Current.Settings&&P.Settings&&Current.Dossier==1919&&P.Dossier==1919))S.ArchivePendingDelete.Empty();
 if(P.Phase==EFrontlinePhase::Briefing&&!P.NationalMap){auto& M=WarRoom52::State(PC);const auto From=M;M=P.Map;M.WasActive=true;
  if(S.Dossier==1917){const auto Centre=M.FlightDuration>0?M.FlightToCentre:M.Centre;const float Zoom=M.FlightDuration>0?M.FlightToZoom:M.Zoom;
   M.Centre=From.Centre;M.Zoom=From.Zoom;M.FromDetail=From.Detail;M.ToDetail=M.AtlasMode==2?1.f:0.f;M.OperationSheet=false;
   WarRoom52::Flight(M,Zoom,Centre,1.6f,14);}}
 if(B->Phase==EFrontlinePhase::Fighting){const bool Overlay=S.Menu||S.Help||S.Settings||S.Technology||S.AchievementsV76;
  const bool Own=OwnedPause||(!WasOverlay&&!UGameplayStatics::IsGamePaused(PC));
  if(Overlay){B->SetPaused(true);S.bPausedByOverlay=Own&&(S.Menu||S.Help);S.bPausedForAuxOverlay=Own&&(S.Settings||S.Technology||S.AchievementsV76)&&!S.Menu;}
  else if(WasOverlay&&OwnedPause){WarfareFRRUWorldV82::ResumeAfterOwnedPause(B);S.bPausedByOverlay=S.bPausedForAuxOverlay=false;}}
 return true;
}
void Travel(AWarfareFrontlineController* PC,bool Forward)
{
 auto& Modal=ScreenState(PC);
 if(Modal.AchievementsV76&&(Modal.HistoricalDispatch||!Modal.ArchivePendingDelete.IsEmpty()))
 {if(!Forward)PC->ToggleAchievementsV76();return;}
 if(!Allowed(PC))
 {
  auto& S=ScreenState(PC);if(Forward||S.Preparing||S.CampaignEndingV51)return;
  if(S.ExitConfirmV61)S.ExitConfirmV61=false;
  else if(!S.ArchivePendingDelete.IsEmpty())S.ArchivePendingDelete.Empty();
  else if(S.HistoricalDispatch)S.HistoricalDispatch=false;
  else if(S.UnitIntroduction){if(S.UnitIntroductionPage>0)--S.UnitIntroductionPage;else S.UnitIntroduction=false;}
  return;
 }
 auto& H=History(PC);auto& Source=Forward?H.Forward:H.Back;auto& Destination=Forward?H.Back:H.Forward;
 const auto Current=Capture(PC);H.Restoring=true;ON_SCOPE_EXIT{H.Restoring=false;};
 while(!Source.IsEmpty()){const auto P=Source.Pop();if(Restore(PC,P)){Destination.Add(Current);return;}}
 if(!Forward)PC->ToggleMenu();
}
int32 EmittedClicksV64=0;
bool AuthenticHUDInputV75=false;
uint64 LastClickFrameV75=MAX_uint64;
TWeakObjectPtr<UAudioComponent> ActiveClickV82;
TStrongObjectPtr<USoundWave> OrderSoundV8213;
TSharedPtr<FStreamableHandle> OrderHandleV8213;
bool OrderRequestedV8213=false,OrderSettledV8213=false;
void PreloadClickSoundV8213()
{
 if(FrontlineUIAssetsV8213::BaselineMode()||OrderRequestedV8213)return;
 OrderRequestedV8213=true;
 const FSoftObjectPath Path(TEXT("/Game/Warfare/Campaign/V61/S_ConsoleOrder.S_ConsoleOrder"));
 OrderHandleV8213=UAssetManager::GetStreamableManager().RequestAsyncLoad(Path,
  FStreamableDelegateWithHandle::CreateLambda([Path](TSharedPtr<FStreamableHandle> Handle)
  {
   OrderSoundV8213.Reset(Cast<USoundWave>(Path.ResolveObject()));OrderSettledV8213=true;
   if(Handle.IsValid())Handle->ReleaseHandle();OrderHandleV8213.Reset();
  }),FStreamableManager::DefaultAsyncLoadPriority,false,false,TEXT("WarfareFrontConsoleAudioV8213"));
 if(!OrderHandleV8213.IsValid())OrderSettledV8213=true;
}
bool ClickSoundSettledV8213(){return FrontlineUIAssetsV8213::BaselineMode()||OrderSettledV8213;}
void SilenceClickForLoadingV82()
{
 if(auto* Click=ActiveClickV82.Get())Click->Stop();
 ActiveClickV82.Reset();
}
bool EntersLoading(AWarfareFrontlineController* PC,FName Command)
{
 if(!PC)return false;auto* B=FrontlineFor(PC);const auto& S=ScreenState(PC);
 if(!B||B->Phase!=EFrontlinePhase::Briefing||!B->GetSelectedCampaignMission()||!B->AssetFailure.IsEmpty())return false;
 const bool Begin=Command==TEXT("Begin")||Command==TEXT("V52Begin")||Command==TEXT("UnitIntroductionBegin");
 if(S.UnitIntroduction&&Begin)
 {const auto Cards=UnitIntroductionCards(B);return S.UnitIntroductionPage+1>=Cards.Num();}
 if(!WarfareFRRUCampaignV82::Active(B)&&(Begin||Command==TEXT("V32DeploymentConfirm")))return true;
 if(WarfareFRRUCampaignV82::Active(B)&&S.Deployment&&S.Dossier==1917
    &&(Command==TEXT("V32DeploymentConfirm")||Command==TEXT("Begin")))
    return WarfareFRRUCampaignV82::DeploymentBlockReason(B).IsEmpty()&&WarfareFRRUCampaignV82::WorldReady(B);
 return false;
}
void Click(AWarfareFrontlineController* PC,FName Command)
{
 // Synthetic transitions and stale hit boxes during warm-up are silent.
 if(!PC||ScreenState(PC).Preparing||EntersLoading(PC,Command)||History(PC).Restoring||History(PC).Depth>1
    ||Command==TEXT("Blocked")||Command==TEXT("V52MapSurface")
    ||Command==TEXT("PreparationBack"))return;
 if(!AuthenticHUDInputV75&&!PC->IsInputKeyDown(EKeys::LeftMouseButton)&&!PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))return;
 if(LastClickFrameV75==GFrameCounter)return;
 auto* Audio=PC->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();if(!Audio||Audio->IsMuted())return;
 if(FrontlineUIAssetsV8213::BaselineMode()&&!OrderSoundV8213.IsValid())
  OrderSoundV8213.Reset(LoadObject<USoundWave>(nullptr,TEXT("/Game/Warfare/Campaign/V61/S_ConsoleOrder.S_ConsoleOrder")));
 if(auto* Sound=OrderSoundV8213.Get())
 {
 LastClickFrameV75=GFrameCounter;++EmittedClicksV64;
 SilenceClickForLoadingV82();
 ActiveClickV82=UGameplayStatics::SpawnSound2D(PC,Sound,.52f*Audio->GetVolume(EFrontlineAudioBus::Master)*Audio->GetVolume(EFrontlineAudioBus::Effects),1.f,0.f,nullptr,false,true);
#if WITH_EDITOR
 UE_LOG(LogTemp,Display,TEXT("WF_UI_CLICK_SOUND emitted=%d command=%s component=%d"),EmittedClicksV64,*Command.ToString(),ActiveClickV82.IsValid());
#endif
 }
}
}
