#include "WarfareFrontlineReviewV49.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGameplayV49.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "RHI.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

namespace
{
bool Allowed49(const AWarfareFrontlineGameMode* B)
{
    FString D;return B&&B->GetWorld()&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV49Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),D)&&!D.IsEmpty();
}
bool Pending49()
{
#if WITH_EDITOR
    return !GUsingNullRHI&&GShaderCompilingManager&&GShaderCompilingManager->IsCompiling();
#else
    return false;
#endif
}
}

struct FFrontlineReviewV49
{
    static bool Ready(AWarfareFrontlineGameMode* B)
    {return B&&B->HasActorBegunPlay()&&B->LoadedSoldier&&B->IdleAnimation&&B->AssetFailure.IsEmpty();}
    static bool Field(AWarfareFrontlineGameMode* B,EFrontlineFaction F)
    {
        if(!Allowed49(B)||!FrontlineCampaignV41::PrepareReviewOperation(B,F,0))return false;
        B->BeginBattle();B->SetActorTickEnabled(false);UGameplayStatics::SetGamePaused(B,false);
        return B->Phase==EFrontlinePhase::Fighting&&B->AssetFailure.IsEmpty();
    }
    static void Clear(AWarfareFrontlineGameMode* B)
    {
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Empty();B->Squads.Empty();
        if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))V->ResetBattle();FrontlineMobilityV41::Reset(B);
    }
    static AWarfareFrontlineSoldier* Man(AWarfareFrontlineGameMode* B,int32 Id,int32 Team,int32 Member,FVector P)
    {
        P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y)+88;
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(P,FRotator(0,B->GetTeamDirection(Team)>0?0:180,0),Params);
        if(!U)return nullptr;U->Initialize(B,Id,Team,EFrontlineRole::Rifle,Member);U->bFieldEntered=true;
        U->SetActorTickEnabled(false);U->ShotClock=10000;B->Units.Add(U);return U;
    }
    static int32 Section(AWarfareFrontlineGameMode* B,int32 Team,float Progress,int32 Count=6)
    {
        FFrontlineSquad S;S.Id=B->NextSquadId++;S.Team=Team;S.Role=EFrontlineRole::Rifle;S.Trench=INDEX_NONE;
        S.Destination=B->GetGoalTrenchIndex(Team);S.LastPassedTrench=B->GetHomeTrenchIndex(1-Team);S.bAdvancing=true;S.bReinforcement=false;S.Slot=1;
        const int32 Id=S.Id,Dir=B->GetTeamDirection(Team);
        for(int32 I=0;I<Count;++I)if(auto* U=Man(B,Id,Team,I,FVector(Dir*Progress,(I-(Count-1)*.5f)*100.f,0)))S.Members.Add(U);
        B->Squads.Add(S);return Id;
    }
    static void Place(AWarfareFrontlineSoldier* U,float Progress,int32 Dir,float Y)
    {const float X=Dir*Progress;U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));}
    static AFrontlineTank* Tank(AWarfareFrontlineGameMode* B,int32 Team,float Progress,int32 Dir,float Y=650.f)
    {
        auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!V)return nullptr;
        auto* T=V->Spawn(B->GetFactionForTeam(Team)==EFrontlineFaction::British?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V,Team,false);
        if(T){T->bEntered=true;T->HeadingYawV37=B->GetTeamDirection(Team)>0?0.f:180.f;
            T->FitGroundV35(FVector(Dir*Progress,Y,0),0.f);T->SetActorTickEnabled(false);}
        return T;
    }
    static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        auto J=MakeShared<FJsonObject>();if(!Allowed49(B)){J->SetBoolField(TEXT("success"),false);return J;}
        const auto SavedRuns=B->CampaignRuns;const auto SavedId=B->ActiveCampaignId;const auto SavedFaction=B->PlayerFaction;
        ON_SCOPE_EXIT{Clear(B);B->AbandonBattleToOperations();B->CampaignRuns=SavedRuns;B->ActiveCampaignId=SavedId;B->PlayerFaction=SavedFaction;};
        bool All=true;TArray<TSharedPtr<FJsonValue>> Rows;
        for(EFrontlineFaction F:{EFrontlineFaction::British,EFrontlineFaction::German})
        {
            const FString Prefix=F==EFrontlineFaction::British?TEXT("v49_british_"):TEXT("v49_german_");
            const auto Test=[&](const TCHAR* Name,bool Good){Check(Prefix+Name,Good);All&=Good;};
            if(!Field(B,F)){Test(TEXT("real_first_operation_loaded"),false);continue;}Clear(B);
            const int32 Team=0,Dir=B->GetTeamDirection(Team);const float Line=FrontlineGameplayV49::BreakthroughLine();
            const int32 Id=Section(B,Team,Line+60);auto* S=B->FindSquad(Id);
            if(!S||S->Members.Num()!=6){Test(TEXT("six_real_attackers"),false);continue;}
            auto* Tail=S->Members.Last().Get();Place(Tail,Line-80,Dir,250);
            Test(TEXT("mean_crossing_cannot_leave_living_tail_behind"),S->Centre().X*Dir>Line&&!FrontlineGameplayV43::HasCrossedEnemyEnd(B,*S));
            Tail->Health=0;Test(TEXT("dead_tail_does_not_block_surviving_members"),FrontlineGameplayV43::HasCrossedEnemyEnd(B,*S));Tail->Health=1;
            for(const auto& W:S->Members)Place(W.Get(),Line-1,Dir,W->Feet().Y);
            Test(TEXT("old_end_and_new_line_not_yet_cleared"),!FrontlineGameplayV43::HasCrossedEnemyEnd(B,*S));
            for(const auto& W:S->Members)Place(W.Get(),Line+35,Dir,W->Feet().Y);
            Test(TEXT("all_six_clear_new_line_in_empty_sector"),FrontlineGameplayV43::HasCrossedEnemyEnd(B,*S));
            const int32 Home=B->GetHomeTrenchIndex(1-Team);const float HomeProgress=B->Trenches[Home].X*Dir;
            auto* Guard=Man(B,949900+int32(F),1-Team,0,FVector(B->Trenches[Home].X,0,0));
            if(!Guard){Test(TEXT("real_enemy_guard_created"),false);continue;}
            Test(TEXT("living_enemy_home_trench_still_defended"),FrontlineGameplayV49::InEndSector(B,Team,Guard->Feet())&&FrontlineGameplayV49::EnemyEndDefended(B,Team)&&!FrontlineGameplayV43::HasCrossedEnemyEnd(B,*S));
            Guard->Health=0;Test(TEXT("corpse_does_not_hold_end_sector"),!FrontlineGameplayV49::EnemyEndDefended(B,Team)&&FrontlineGameplayV43::HasCrossedEnemyEnd(B,*S));
            Guard->Health=1;Guard->bFieldEntered=false;Test(TEXT("unentered_reserve_does_not_hold_end_sector"),!FrontlineGameplayV49::EnemyEndDefended(B,Team));
            Guard->bFieldEntered=true;Place(Guard,-1000,Dir,0);Test(TEXT("distant_enemy_elsewhere_does_not_hold_end_sector"),!FrontlineGameplayV49::EnemyEndDefended(B,Team));
            Place(Guard,HomeProgress,Dir,FrontlineBattlefield::HalfFrontage+250);Test(TEXT("outside_frontage_does_not_hold_end_sector"),!FrontlineGameplayV49::EnemyEndDefended(B,Team));
            Guard->Health=0;
            auto* EnemyTank=Tank(B,1-Team,HomeProgress,Dir);
            if(!EnemyTank){Test(TEXT("actual_defending_tank_created"),false);continue;}
            Test(TEXT("operational_entered_enemy_tank_blocks_infantry"),FrontlineGameplayV49::EnemyEndDefended(B,Team)&&!FrontlineGameplayV43::HasCrossedEnemyEnd(B,*S));
            EnemyTank->bEntered=false;Test(TEXT("reserve_tank_does_not_hold_end_sector"),!FrontlineGameplayV49::EnemyEndDefended(B,Team));
            EnemyTank->bEntered=true;EnemyTank->HullIntegrity=0;Test(TEXT("wreck_does_not_hold_end_sector"),!FrontlineGameplayV49::EnemyEndDefended(B,Team));
            auto* OwnTank=Tank(B,Team,Line-80,Dir,-650);
            if(!OwnTank){Test(TEXT("actual_attacking_tank_created"),false);continue;}
            const float RearBefore=OwnTank->RearProgressV49();
            Test(TEXT("tank_nose_crossing_cannot_leave_rear_behind"),OwnTank->GetActorLocation().X*Dir+OwnTank->FootprintHalfLengthV37()>Line&&RearBefore<Line&&!OwnTank->HasClearedEnemyEndV49());
            OwnTank->AddActorWorldOffset(FVector(Dir*(Line-RearBefore+60.f),0,0));
            Test(TEXT("whole_tank_can_clear_empty_end_sector"),OwnTank->RearProgressV49()>Line&&OwnTank->HasClearedEnemyEndV49());
            Guard->Health=1;Place(Guard,Line-250,Dir,-650);
            Test(TEXT("stationary_cleared_tank_waits_for_remaining_guard"),!OwnTank->HasClearedEnemyEndV49());
            Guard->Health=0;Test(TEXT("stationary_cleared_tank_can_win_after_guard_removed"),OwnTank->HasClearedEnemyEndV49());
            // A last guard behind the advancing men remains a contact. Elevate
            // both equally for this LOS-specific branch, outside terrain noise;
            // positions, direction, entered state, range and production query
            // remain authoritative. The subsequent screenshot uses real ground.
            auto* Scout=S->Members[0].Get();for(const auto& W:S->Members)if(W.Get()!=Scout)W->Health=0;
            Scout->SetActorLocation(FVector(Dir*(Line+25),0,2488));Guard->SetActorLocation(FVector(Dir*(Line-275),0,2488));Guard->Health=1;
            const bool RearContact=B->AcquireTarget(Scout,true)==Guard;
            Test(TEXT("end_sector_rear_guard_is_real_advance_contact"),RearContact);
            Guard->Health=0;Scout->SetActorLocation(FVector(Dir*(-1000),0,2488));
            Guard->SetActorLocation(FVector(Dir*(-1300),0,2488));Guard->Health=1;
            Test(TEXT("ordinary_frontage_still_does_not_chase_rear_contact"),B->AcquireTarget(Scout,true)==nullptr);
            Guard->Health=0;OwnTank->TrackIntegrity[0]=OwnTank->TrackIntegrity[1]=0;
            const FVector Parked=OwnTank->GetActorLocation();OwnTank->Simulate(.05f);
            Test(TEXT("actual_immobilized_tank_tick_finishes_cleared_end"),B->BattleOutroWinner==Team&&FVector::Dist2D(Parked,OwnTank->GetActorLocation())<.01f);
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("faction"),Prefix);Row->SetNumberField(TEXT("direction"),Dir);
            Row->SetNumberField(TEXT("new_line_cm"),Line);Row->SetNumberField(TEXT("enemy_home_progress_cm"),HomeProgress);
            Row->SetNumberField(TEXT("tank_rear_before_cm"),RearBefore);Row->SetBoolField(TEXT("rear_contact_found"),RearContact);Rows.Add(MakeShared<FJsonValueObject>(Row));
        }
        J->SetArrayField(TEXT("mirrored_cases"),Rows);J->SetBoolField(TEXT("success"),All&&Rows.Num()==2);
        J->SetStringField(TEXT("scope"),TEXT("Two actual first operations with real six-member squads and both faction tank hulls. Production breakthrough/sector/contact predicates; no alternate victory calculation, persistent campaign changes or historical suites. The rear-contact unit pair is elevated solely to isolate the LOS branch; the capture uses real ground."));return J;
    }
    static bool Scene(AWarfareFrontlineGameMode* B,FVector& Focus)
    {
        if(!Field(B,EFrontlineFaction::German))return false;Clear(B);
        const int32 Dir=B->GetTeamDirection(0);const float Line=FrontlineGameplayV49::BreakthroughLine();
        const int32 AId=Section(B,0,Line+30,4);const int32 DId=Section(B,1,-(Line-350),4);
        auto* A=B->FindSquad(AId);auto* D=B->FindSquad(DId);if(!A||!D||A->Members.Num()!=4||D->Members.Num()!=4)return false;
        D->bAdvancing=false;D->bHoldPosition=true;
        // Explicit held ground prevents the fixture defenders walking toward a
        // nonexistent assignment. Real enemies are visible and can be acquired;
        // only attackers tick so the screenshot cannot consume its guards early.
        for(const auto& W:D->Members){W->SetActorRotation(FRotator(0,Dir>0?0:180,0));W->SetMotion(W->GetAimMotion());}
        for(const auto& W:A->Members){W->ShotClock=10;W->AimSettleClock=0;W->SetActorTickEnabled(true);}
        Focus=FVector(Dir*(Line-140),0,130);
        return FrontlineGameplayV49::EnemyEndDefended(B,0)&&!FrontlineGameplayV43::HasCrossedEnemyEnd(B,*A);
    }
    static TSharedRef<FJsonObject> SceneState(AWarfareFrontlineGameMode* B)
    {
        auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Men;
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))
        {auto M=MakeShared<FJsonObject>();M->SetNumberField(TEXT("team"),U->Team);M->SetNumberField(TEXT("squad"),U->SquadId);
            M->SetStringField(TEXT("feet"),U->Feet().ToString());M->SetStringField(TEXT("rotation"),U->GetActorRotation().ToString());
            M->SetBoolField(TEXT("alive"),U->IsAlive());M->SetBoolField(TEXT("entered"),U->bFieldEntered);M->SetBoolField(TEXT("hidden"),U->IsHidden());
            M->SetNumberField(TEXT("speed"),U->GetMotionSpeed());Men.Add(MakeShared<FJsonValueObject>(M));}
        J->SetArrayField(TEXT("soldiers"),Men);J->SetBoolField(TEXT("player_end_defended"),FrontlineGameplayV49::EnemyEndDefended(B,0));return J;
    }
};

void StartFrontlineV49Review(AWarfareFrontlineGameMode* B)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV49Review")))return;
    struct FRun49{TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;double Start=FPlatformTime::Seconds(),Next=0;int32 Stage=0;bool Good=true,Advance=false;FString Dir,Shot;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;};
    auto R=MakeShared<FRun49>();R->Battle=B;R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V49/Review"));IFileManager::Get().MakeDirectory(*R->Dir,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
    {
        auto* B=R->Battle.Get();const double Now=FPlatformTime::Seconds();
        const auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Good;};
        const auto Finish=[&]()
        {
            const bool Image=GUsingNullRHI?R->Shot.IsEmpty():!R->Shot.IsEmpty()&&IFileManager::Get().FileSize(*R->Shot)>1024;
            Check(TEXT("v49_only_requested_capture_written"),Image);R->Report->SetBoolField(TEXT("success"),R->Good);R->Report->SetArrayField(TEXT("checks"),R->Checks);
            int32 Passed=0;for(const auto& V:R->Checks)Passed+=V->AsObject()->GetBoolField(TEXT("passed"));R->Report->SetNumberField(TEXT("passed"),Passed);R->Report->SetNumberField(TEXT("total"),R->Checks.Num());
            R->Report->SetBoolField(TEXT("null_rhi_logic_only"),GUsingNullRHI);R->Report->SetStringField(TEXT("capture"),R->Shot);R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetStringField(TEXT("rhi"),GDynamicRHI?GDynamicRHI->GetName():TEXT("unavailable"));R->Report->SetStringField(TEXT("scope"),TEXT("V49 only: mirrored whole-section/whole-hull breakthrough, defended enemy-end sector and rear contact. At most one actual ground engagement capture; no performance, universal visual-quality or old-suite assertion."));
            FString JSON;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V49_REVIEW_DONE success=%d checks=%d"),R->Good,R->Checks.Num());FPlatformMisc::RequestExit(false);
        };
        if(!B||Now-R->Start>120.){Check(TEXT("v49_bounded_world_and_120_seconds"),false);Finish();return false;}
        auto* PC=B&&B->GetWorld()?Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController()):nullptr;
        if(!FFrontlineReviewV49::Ready(B)||(!GUsingNullRHI&&(!PC||!PC->GetHUD()))||Pending49())return true;
        if(R->Advance)B->Elapsed+=FMath::Clamp(DT,0.f,.05f);if(Now<R->Next)return true;
        if(R->Stage==0)
        {
            Check(TEXT("v49_isolated_volatile_no_save"),Allowed49(B));if(!Allowed49(B)){Finish();return false;}
            UGameplayStatics::SetGamePaused(B,false);auto Logic=FFrontlineReviewV49::Run(B,Check);R->Report->SetObjectField(TEXT("breakthrough"),Logic);Check(TEXT("v49_mirrored_logic_complete"),Logic->GetBoolField(TEXT("success")));
            if(GUsingNullRHI){Finish();return false;}
            FVector Focus;const bool Scene=FFrontlineReviewV49::Scene(B,Focus);Check(TEXT("v49_real_end_guard_scene"),Scene);if(!Scene){Finish();return false;}
            FrontlineReviewUIV41::Clear(PC);FrontlineReviewUIV41::Camera(PC,Focus.X,Focus.Y,3000,Focus.Z);
            if(auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Settings->SetHDREnabled(false,false);Settings->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            R->Advance=true;R->Stage=1;R->Next=FPlatformTime::Seconds()+4.;return true;
        }
        if(R->Stage==1)
        {
            R->Report->SetObjectField(TEXT("captured_scene"),FFrontlineReviewV49::SceneState(B));Check(TEXT("v49_guard_still_prevents_premature_victory"),FrontlineGameplayV49::EnemyEndDefended(B,0)&&B->Phase==EFrontlinePhase::Fighting);
            R->Shot=R->Dir/TEXT("01-enemy-end-guard-engagement.png");FScreenshotRequest::RequestScreenshot(R->Shot,true,false);R->Stage=2;R->Next=Now+1.;return true;
        }
        Finish();return false;
    }));
}
