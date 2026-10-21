#include "WarfareFrontlineLightingV42.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineSoldierSurfaceV62.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineMedalsV8215.h"
#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineVehiclesV28.h"
namespace {
void StartFrontlineReviewV84(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun {int32 Stage=0,Scene=0,Operation=INDEX_NONE,Nation=0,Shots=0,Page=0,Samples=0;bool Good=true;double Started=FPlatformTime::Seconds(),Next=Started+2,Prepared=0,GPU=0,Game=0;FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto R=MakeShared<FRun>();R->Dir=User;int32 Scene=0;FParse::Value(FCommandLine::Get(),TEXT("WFV84Scene="),Scene);R->Scene=FMath::Clamp(Scene,0,2);
 int32 Operation=INDEX_NONE,Nation=0;
 if(FParse::Value(FCommandLine::Get(),TEXT("WFV842Operation="),Operation))R->Operation=FMath::Clamp(Operation,0,19);
 if(FParse::Value(FCommandLine::Get(),TEXT("WFV842Nation="),Nation))R->Nation=FMath::Clamp(Nation,0,1);
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV842UIFocused")))R->Page=12;
 IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float){
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[R](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Dir/Name));};
  auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Started);J->SetNumberField(TEXT("requested_operation"),R->Operation);J->SetNumberField(TEXT("requested_nation"),R->Nation);J->SetStringField(TEXT("scope"),TEXT("Disposable prepared-world production API fixtures and fixed-camera presentation; not a natural campaign or battle feasibility test."));Write(TEXT("checks.json"),J);FPlatformMisc::RequestExit(false);return false;};
  if(!B||!PC||!PC->BattleCamera){if(Now-R->Started<45)return true;Check(TEXT("world_ready"),false);return Finish();}
  if(Now-R->Started>240){Check(TEXT("bounded_review"),false);return Finish();}
  if(R->Stage==3&&Now-R->Prepared>5){++R->Samples;R->GPU+=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());R->Game+=FPlatformTime::ToMilliseconds(GGameThreadTime);}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;auto& S=ScreenState(PC);
  auto Clear=[&](){S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;V28ReviewPointer=FVector2D(-10000,-10000);PC->CancelTargeting();FrontlineMapSceneV61::Update(PC,false,FVector2D(.5,.5),1,1,0,0,0);};
  auto Shot=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(Name+TEXT(".png")),true,false);++R->Shots;};
  if(R->Stage==0){if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetHDREnabled(false,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetRayReconstruction(!FParse::Param(FCommandLine::Get(),TEXT("WFV84RROff")),false);}
   Check(TEXT("disposable_available_campaign"),B->StartNewCampaign(EFrontlineFaction(R->Nation),EFrontlineDifficulty::Veteran));
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=19;P->WonOperationMaskV52=(1u<<19)-1;P->DispatchResolvedMaskV52=(1u<<20)-1;}
   const int32 Ops[]={0,5,14};Check(TEXT("real_operation_selected"),B->SelectCampaignMission(R->Operation==INDEX_NONE?Ops[R->Scene]:R->Operation));S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;R->Next=Now+.1;return true;}
  if(R->Stage==1){if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<80){R->Next=Now+.1;return true;}
   Check(TEXT("operation_prepared"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&B->AssetFailure.IsEmpty());if(!R->Good)return Finish();Clear();B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV84UIOnly"))){R->Stage=10;R->Next=Now+.1;return true;}
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV842Mechanics")))
   {
    TArray<TSharedPtr<FJsonValue>> Cases;bool Passed=true;
    auto Capture=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Cases.Add(MakeShared<FJsonValueObject>(J));Passed&=Pass;Check(Name,Pass);};
    Capture(TEXT("v842_mechanics_runs_before_visual_fixture_in_prepared_volatile_world"),B->Phase==EFrontlinePhase::Fighting&&B->IsProgressionVolatile());
    FrontlineAchievementsV76::RunBalanceChecksV842(B,Capture);
    FrontlineAchievementsV76::RunAllQuietChecksV8310(B,Capture);
    FrontlineMedalsV8215::RunCatalogueChecks(Capture);
    FrontlineArmourV63::RunRailAllocationChecksV842(B,Capture);
    if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
    {
     const auto Mobility=Fleet->RunMobilityChecksV842();Write(TEXT("mobility.json"),Mobility);
     const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;bool Success=false;
     Capture(TEXT("v842_mobility_report_has_a_check_array"),Mobility->TryGetArrayField(TEXT("checks"),Rows)&&Rows!=nullptr);
     if(Rows)for(const auto& Value:*Rows)
     {
      TSharedPtr<FJsonObject> Row;if(Value.IsValid())Row=Value->AsObject();FString Name;bool Pass=false;
      if(Row.IsValid()&&Row->TryGetStringField(TEXT("name"),Name)&&Row->TryGetBoolField(TEXT("passed"),Pass))Capture(Name,Pass);
      else Capture(TEXT("v842_mobility_check_schema"),false);
     }
     Capture(TEXT("v842_mobility_report_success"),Mobility->TryGetBoolField(TEXT("success"),Success)&&Success);
    }
    else Capture(TEXT("v842_mechanics_actual_fleet_present"),false);
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),Passed);Result->SetArrayField(TEXT("checks"),Cases);
    Result->SetStringField(TEXT("scope"),TEXT("Actual prepared-world API fixtures: honour criteria, forced rail allocation and grounded production vehicle movement. Criteria and physical routes are exercised; natural battle completion/difficulty and durable awards are not claimed."));Write(TEXT("mechanics.json"),Result);
    if(!R->Good)return Finish();
   }
   const float Mid=B->Trenches.IsEmpty()?0.f:B->Trenches[B->Trenches.Num()/2].X;
   for(int32 Team=0;Team<2;++Team)for(int32 I=0;I<3;++I){auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U){Check(TEXT("actual_soldier_spawn"),false);continue;}U->Initialize(B,84001+Team,Team,I==1?EFrontlineRole::Pioneer:EFrontlineRole::Rifle,I);U->bFieldEntered=true;U->SetActorTickEnabled(false);U->ReturnToIdle();const FVector P(Mid+(Team?510.f:-510.f)+I*120.f,-280.f-I*110.f,0);U->SetActorLocation(FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P)+88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));B->Units.Add(U);Check(TEXT("actual_soldier_material_assets"),U->Soldier!=nullptr&&U->Soldier->GetSkeletalMeshAsset()!=nullptr);}
   for(int32 Team=0;Team<2;++Team){auto* T=B->GetWorld()->SpawnActor<AFrontlineTank>();Check(TEXT("actual_armour_configured"),T&&T->Configure(B,Team?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale,Team,8400+Team));if(T){const FVector P(Mid+(Team?1350.f:-1350.f),150,0);T->SetActorTransform(T->GroundPoseForContactV8218(P,B->GetTeamDirection(Team)>0?0:180,0));T->SetActorTickEnabled(false);}}
   const FVector Focus(Mid,-200,130);PC->BattleCamera->SetActorLocation(Focus+FVector(0,8500,2950));PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());PC->BattleCamera->GetCameraComponent()->OrthoWidth=6000;PC->SetViewTarget(PC->BattleCamera);if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),Focus,6000,3840);
   R->Stage=2;R->Next=Now+1.;return true;}
  if(R->Stage==2){FrontlineLightingV42::ObserveReviewWorld(B->GetWorld());UGameplayStatics::SetGamePaused(PC,true);R->Prepared=Now;R->Stage=3;R->Next=Now+9.;return true;}
  if(R->Stage==3)
  {
   FrontlineSoldierSurfaceV62::WriteAudit(B->GetWorld(),TEXT("v84-final"));
   auto Materials=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Vehicles;
   for(TActorIterator<AFrontlineTank> It(B->GetWorld());It;++It)
   {
    auto* C=Cast<UStaticMeshComponent>(It->GetRootComponent());if(!C)continue;
    auto V=MakeShared<FJsonObject>();V->SetStringField(TEXT("actor"),It->GetPathName());
    V->SetNumberField(TEXT("model"),int32(It->Type));V->SetNumberField(TEXT("team"),It->Team);
    V->SetStringField(TEXT("model_name"),UFrontlineVehicleSubsystem::Definition(It->Type).Name);
    V->SetStringField(TEXT("body_kind"),It->IsRailConvoyV32()?TEXT("rail"):It->IsWheeledV8218()?TEXT("wheeled"):TEXT("tracked"));
    V->SetStringField(TEXT("body_mesh"),GetPathNameSafe(C->GetStaticMesh()));V->SetStringField(TEXT("body_component"),C->GetClass()->GetPathName());
    V->SetBoolField(TEXT("actual_assets_ready"),It->AreAssetsReady());TArray<TSharedPtr<FJsonValue>> Slots;
    for(int32 I=0;I<C->GetNumMaterials();++I)
    {auto M=MakeShared<FJsonObject>();M->SetStringField(TEXT("material"),GetPathNameSafe(C->GetMaterial(I)));M->SetStringField(TEXT("base"),C->GetMaterial(I)?GetPathNameSafe(C->GetMaterial(I)->GetBaseMaterial()):TEXT("None"));Slots.Add(MakeShared<FJsonValueObject>(M));}
    V->SetArrayField(TEXT("slots"),Slots);Vehicles.Add(MakeShared<FJsonValueObject>(V));
   }
   Materials->SetArrayField(TEXT("actual_armour"),Vehicles);Write(TEXT("materials.json"),Materials);Write(TEXT("body-kinds.json"),Materials);
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV842Review")))FrontlineLightingV42::RunV842Checks(B,Check);
   else FrontlineLightingV42::RunV84Checks(B,Check);
   Write(TEXT("lighting.json"),FrontlineLightingV42::DescribeV84(B));
   auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),R->Samples);J->SetNumberField(TEXT("gpu_mean_ms"),R->GPU/FMath::Max(1,R->Samples));J->SetNumberField(TEXT("game_mean_ms"),R->Game/FMath::Max(1,R->Samples));J->SetStringField(TEXT("scope"),TEXT("Warm disposable fixed gameplay camera 4K Quality, High, FG off; not full battle performance."));Write(TEXT("cost.json"),J);
   Shot(TEXT("battlefield"));R->Stage=4;R->Next=Now+.1;return true;
  }
  if(R->Stage==4)return Finish();
  if(R->Stage==10){Clear();B->Phase=EFrontlinePhase::Briefing;MedalWallV8215::RunExhibitChecksV8311(Check);MedalWallV8215::RunDeedGeometryChecksV83(Check);MedalWallV8215::RunScrollChecksV842(Check);R->Stage=11;R->Next=Now+.1;return true;}
  if(R->Stage==11)
  {
   Clear();const int32 Nation=R->Page<8?R->Page/2:R->Page<12?(R->Page-8)/2:(R->Page-12)/2;
   const bool ZH=(R->Page%2)!=0;FrontlineLocalizationV35::SetLanguage(ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
   if(Nation<2)Check(TEXT("isolated_award_fixture"),FrontlineAchievementsV76::SeedHonoursPresentationV837(B,EFrontlineFaction(Nation)));
   S.AchievementsV76=true;auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Nation=Nation;M.Row=Nation<2?2:1;
   M.ExhibitRowV8311=Nation<2&&R->Page<8?MedalWallV8215::ExhibitLayoutV8311::Maximum(MedalWallV8215::Exhibits(EFRRUNationV81(Nation)).Num()):0;
   M.Gallery.Reset(M.ExhibitRowV8311*MedalWallV8215::ExhibitLayoutV8311::SlotH);M.Catalogue.Reset(M.Row*162.f);M.ScrollNation=Nation;
   if(R->Page>=12)
   {
    const float GalleryMax=MedalWallV8215::ExhibitLayoutV8311::Maximum(MedalWallV8215::Exhibits(EFRRUNationV81(Nation)).Num())*MedalWallV8215::ExhibitLayoutV8311::SlotH;
    const float CatalogueMax=FMath::Max(0,(MedalWallV8215::Items(EFRRUNationV81(Nation)).Num()+2)/3-2)*162.f;
    M.Gallery.Reset(FMath::Min(GalleryMax,GalleryMax*.5f+37.f));M.Catalogue.Reset(FMath::Min(CatalogueMax,CatalogueMax*.5f+37.f));
   }
   M.Focus=MedalWallV8215::DeedBaseV8222+Nation;M.PreviewGrade=MedalWallV8215::EntryTier(M.Focus);R->Stage=12;R->Next=Now+.8;return true;
  }
  if(R->Stage==12)
  {
   const int32 Nation=R->Page<8?R->Page/2:R->Page<12?(R->Page-8)/2:(R->Page-12)/2;
   Shot(FString::Printf(TEXT("medal-wall-%d-%s-%s"),Nation,R->Page%2?TEXT("zh"):TEXT("en"),R->Page<8?TEXT("last"):R->Page<12?TEXT("full"):TEXT("middle")));
   ++R->Page;R->Stage=R->Page<16?11:4;R->Next=Now+.1;return true;
  }
  return Finish();
 }),0.f);
}
}
