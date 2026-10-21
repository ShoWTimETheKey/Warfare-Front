#include "WarfareFrontlineReviewV828.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineCombatV64.h"
#include "WarfareFrontlineFXV47.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineWarRoomV52.h"
#include "WarfareFrontlineWoundFX.h"
#include "WarfareFRRUCampaignV82.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "RHI.h"
#include "RenderTimer.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#include "TextureCompiler.h"
#endif

void StartFrontlineReviewV828(AWarfareFrontlineGameMode* Battle)
{
 FString UserDir;if(!Battle||!Battle->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
  ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
 struct FRun
 {
  TWeakObjectPtr<AWarfareFrontlineGameMode> B;FString Dir;int32 Stage=0;bool Good=true,Motion=false,Sample=false;
  double Start=FPlatformTime::Seconds(),Next=Start+2,Last=Start;float MotionTime=0;
  TArray<TSharedPtr<FJsonValue>> Checks,Shots;TArray<double> Wall,GPU;
  FVector BloodOrigin=FVector(560,320,0);
  TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
 };
 auto R=MakeShared<FRun>();R->B=Battle;R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V828/Review"));
 IFileManager::Get().MakeDirectory(*R->Dir,true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
 {
  auto* B=R->B.Get();if(!B)return false;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V828_CHECK %s %d"),*Name,Pass);};
  auto Finish=[&]()
  {
   R->Report->SetBoolField(TEXT("success"),R->Good);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetArrayField(TEXT("captures"),R->Shots);
   R->Report->SetNumberField(TEXT("wall_seconds"),Now-R->Start);
   auto Dist=[](TArray<double> V){auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),V.Num());if(!V.IsEmpty()){V.Sort();double Sum=0;for(double X:V)Sum+=X;J->SetNumberField(TEXT("mean_ms"),Sum/V.Num());J->SetNumberField(TEXT("p95_ms"),V[FMath::Min(V.Num()-1,FMath::FloorToInt(V.Num()*.95))]);}return J;};
   R->Report->SetObjectField(TEXT("fx_wall"),Dist(R->Wall));R->Report->SetObjectField(TEXT("fx_gpu"),Dist(R->GPU));
   R->Report->SetStringField(TEXT("scope"),TEXT("Disposable focused V828 production combat/outcomes/material checks, two fixed effect ages and live ten-second FX sample. Not a full campaign or compatibility certification. Visual acceptance is separate."));
   FString Text;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Dir/TEXT("review.json")));FPlatformMisc::RequestExit(false);
  };
  auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());
  if(Now-R->Start>240){Check(TEXT("review_completed_within_bound"),false);Finish();return false;}
  if(!PC||!PC->GetHUD())return true;
#if WITH_EDITOR
  if(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling())return true;
#endif
  auto Shot=[&](const TCHAR* Name){const FString P=R->Dir/(FString(Name)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(P,true,false);R->Shots.Add(MakeShared<FJsonValueString>(P));};
  if(R->Motion){const float Step=FMath::Min(DT,.05f);FrontlineMotionV41::TickActionReview(B,Step);R->MotionTime+=Step;}
  if(R->Sample){R->Wall.Add((Now-R->Last)*1000);const double G=FPlatformTime::ToMilliseconds64(RHIGetGPUFrameCycles());if(G>0)R->GPU.Add(G);}
  R->Last=Now;if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  if(R->Motion&&((R->Stage==1&&R->MotionTime<2.5f)||(R->Stage==2&&R->MotionTime<5.5f)))return true;
  double Delay=1;
  switch(R->Stage++)
  {
   case 0:
#if WITH_EDITOR
    if(auto* Atlas=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/Field/V47/Textures/T_FlowDensity64_V47.T_FlowDensity64_V47")))
    {TArray<UTexture*> Pending;Pending.Add(Atlas);FTextureCompilingManager::Get().FinishCompilation(Pending);}
#endif
    Check(TEXT("french_russian_campaigns_remain_closed"),!WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::French)
        &&!WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::Russian));
    Check(TEXT("national_campaign_gate_rejects_enemy_and_unknown_nations"),!WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::AustroHungarian)
        &&!WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::Invalid)&&!WarfareFRRUCampaignV82::CampaignsAvailable(static_cast<EFRRUNationV81>(254)));
    FrontlineWarRoomV52::RunEntryChecksV828(PC,Check);
    Check(TEXT("real_british_operation_prepared"),FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,0));
    B->BeginBattle();B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);PC->GetHUD()->bShowHUD=false;
    FrontlineReviewUIV41::Clear(PC);
    FrontlineCombatV64::RunChecks(B,Check);
    FrontlineMotionV41::RunFocusedChecks(B,Check);FrontlineMotionV41::RunOutcomeChecksV45(B,Check);
    AWarfareFrontlineWoundFX::RunChecksV828(B,Check);
    if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->RunV828AudioChecks(Check);
    FrontlineFXV47::RunFocusedChecks(B,[&](const TCHAR* N,bool P){Check(N,P);});
    FrontlineMotionV41::PrepareActionReview(B,2);FrontlineReviewUIV41::Camera(PC,0,0,3200,350,true);
    R->Motion=true;Delay=2.5;break;
   case 1:Shot(TEXT("01-outcome-arcs"));R->Report->SetObjectField(TEXT("outcomes_first"),FrontlineMotionV41::ActionReviewState(B));Delay=3;break;
   case 2:Shot(TEXT("02-outcome-repeat"));R->Report->SetObjectField(TEXT("outcomes_second"),FrontlineMotionV41::ActionReviewState(B));R->Motion=false;break;
   case 3:
    B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)){U->SetActorHiddenInGame(true);U->SetActorTickEnabled(false);}
    FrontlineReviewUIV41::Camera(PC,0,300,5400,200,true);
    FrontlineFXV47::SetupTemporalFixture(B,.12f);
    for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I){I->SetCasualtyDustV8211(4);I->Tick(0);}
    if(auto* FX=AWarfareFrontlineWoundFX::Get(B->GetWorld()))
    {Check(TEXT("blood_capture_has_actual_surface_runoff"),FX->ReviewDepositsV828(R->BloodOrigin,FVector(1150,420,AWarfareFrontlineGround::HeightAt(1150,420)+85)));FX->Tick(.14f);FX->SetActorTickEnabled(false);}
    Delay=2;break;
   case 4:Shot(TEXT("03-blast-early-blood-volume"));R->Report->SetObjectField(TEXT("early_fx"),FrontlineFXV47::Describe(B));break;
   case 5:
    FrontlineFXV47::SetupTemporalFixture(B,.55f);
    for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I){I->SetCasualtyDustV8211(4);I->Tick(0);}
    if(auto* FX=AWarfareFrontlineWoundFX::Get(B->GetWorld()))FX->Tick(.4f);Delay=2;break;
   case 6:Shot(TEXT("04-blast-dirt-volume"));R->Report->SetObjectField(TEXT("late_fx"),FrontlineFXV47::Describe(B));break;
   case 7:
    for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)I->Destroy();
    if(auto* FX=AWarfareFrontlineWoundFX::Get(B->GetWorld())){FX->Tick(3.8f);FX->SetActorTickEnabled(true);}
    FrontlineReviewUIV41::Camera(PC,R->BloodOrigin.X,R->BloodOrigin.Y,1000,R->BloodOrigin.Z+20,true);Delay=2;break;
   case 8:Shot(TEXT("05-blood-surface-flow"));break;
   case 9:
    FrontlineReviewUIV41::Camera(PC,0,300,5400,200,true);
    {
     FVector At(0,320,AWarfareFrontlineGround::HeightAt(0,320)+430.f);
     auto* Airburst=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);
     Airburst->ConfigureAirburstV64(1200,FVector(1,0,-1));Airburst->SetCasualtyDustV8211(4);
     Airburst->SetActorTickEnabled(false);Airburst->Tick(0);Airburst->Tick(.22f);
     Check(TEXT("v8211_airburst_has_bounded_fragment_fan_and_ground_strikes"),Airburst->GetBallisticFragmentCount()==13&&Airburst->GetVisibleSmokeCount()>3);
    }
    Delay=2;break;
   case 10:
    Shot(TEXT("07-shrapnel-ground-fan"));
    Delay=.5f;break;
   case 11:
    for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)I->Destroy();
    for(int32 I=0;I<3;++I){FVector P(-1450+I*1450,420,0);P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y);auto* A=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(P,FRotator::ZeroRotator);A->ConfigureBlast(false,I==1);}
    if(auto* FX=AWarfareFrontlineWoundFX::Get(B->GetWorld()))for(int32 I=0;I<8;++I)
    {FVector P=R->BloodOrigin;FX->ReviewDepositsV828(P,FVector(-1500+I*420,420,160));}
    R->Sample=true;R->Last=Now;Delay=10;break;
   case 12:
   {
    R->Sample=false;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    const int32 Before=B->GetResearchPoints();B->Morale[0]=100;B->Morale[1]=0;B->Tick(.016f);
    Check(TEXT("completed_action_persists_reward_without_early_display"),B->Phase==EFrontlinePhase::Victory
     &&B->GetResearchPoints()==FMath::Min(99999,Before+3)&&B->GetDisplayedResearchPointsV828()==Before);
    B->BattleOutroElapsed=8;FrontlineReviewUIV41::Clear(PC);PC->ToggleTechnology();PC->GetHUD()->bShowHUD=true;
    FrontlineReviewUIV41::BeginAudit();Delay=2;break;
   }
   case 13:
   {
    Shot(TEXT("06-field-report"));const auto Layout=FrontlineReviewUIV41::EndAudit();
    R->Report->SetObjectField(TEXT("field_report_layout"),Layout);
    Check(TEXT("completed_action_report_layout_fits"),Layout->GetBoolField(TEXT("success")));break;
   }
   case 14:
   {
    const int32 Awarded=B->GetResearchPoints();Check(TEXT("research_balance_becomes_visible_in_operations_room"),
     B->ContinueCampaign()&&B->Phase==EFrontlinePhase::Briefing&&B->GetDisplayedResearchPointsV828()==Awarded);
    Finish();return false;
   }
   default:R->Sample=false;Finish();return false;
  }
  R->Next=FPlatformTime::Seconds()+Delay;return true;
 }));
}
