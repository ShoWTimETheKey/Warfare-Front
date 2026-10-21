#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMotionV40.h"
#include "WarfareFrontlineCampaignV52.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"

// Production actors and production tick paths. The fixture only owns a strict
// V8219 disposable profile; the serial integrator calls this after preparing
// an actual UK/DE battlefield. No parallel world, save or alternate simulator.
struct FFrontlineTimeBudgetV8219Review
{
    struct FSample
    {
        FVector End=FVector::ZeroVector;
        float Travel=0,CannonClock=0,GunClock=0,SoldierClock=0;
        bool Ready=false,Finite=true,ShellPending=false,ShellArrived=false,EscapeFinished=false,ContactStride=false;
    };
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review")))
        {Check(TEXT("v8219_time_budget_requires_exact_disposable_profile"),false);return;}
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        if(!Fleet||B->Trenches.IsEmpty())
        {Check(TEXT("v8219_time_budget_requires_prepared_actual_field"),false);return;}
        const auto Units=B->Units;const auto Squads=B->Squads;const auto Vehicles=Fleet->Vehicles;
        const auto Runs=B->CampaignRuns;const auto Active=B->ActiveCampaignId,Encounter=B->EncounterId;
        const auto Bound=B->BoundCampaignId,BoundEncounter=B->BoundCampaignEncounterId;
        const auto Faction=B->PlayerFaction;const auto Phase=B->Phase;
        const auto Strikes=B->Strikes;const auto Tracers=B->Tracers;
        const auto Smoke=B->SmokeZones;const float Clock=B->Elapsed,OutroClock=B->BattleOutroElapsed;
        const int32 Outro=B->BattleOutroWinner,Mission=B->BoundCampaignMission,Attachment=B->BoundAttachmentStageV53;
        const int32 Shots=B->ShotsFired,NextSquad=B->NextSquadId;const auto Random=B->Random;
        const auto LastEncounter=Fleet->LastEncounter;const float EnemyAt=Fleet->NextEnemyAt;
        const int32 Serial=Fleet->NextSerial,Deploy0=Fleet->DeploymentCountV37[0],Deploy1=Fleet->DeploymentCountV37[1];
        const float Rail0=Fleet->RailReadyAtV75[0],Rail1=Fleet->RailReadyAtV75[1];
        const FString Message=B->LastMessage,Failure=B->AssetFailure;
        TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
        struct FVisibility {AActor* Actor;bool Hidden;};TArray<FVisibility> Visibility;
        for(AWarfareFrontlineSoldier* U:Units)if(IsValid(U)){Visibility.Add({U,U->IsHidden()});U->SetActorHiddenInGame(true);}
        for(AFrontlineTank* V:Vehicles)if(IsValid(V)){Visibility.Add({V,V->IsHidden()});V->SetActorHiddenInGame(true);}
        TArray<AActor*> Owned;
        const auto Clear=[&]()
        {
            for(auto* A:Owned)if(IsValid(A))A->Destroy();Owned.Reset();
            B->Units.Reset();B->Squads.Reset();Fleet->Vehicles.Reset();B->Tracers.Reset();
            FrontlineMobilityV41::Reset(B);
        };
        ON_SCOPE_EXIT
        {
            Clear();for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
            for(const auto& S:Visibility)if(IsValid(S.Actor))S.Actor->SetActorHiddenInGame(S.Hidden);
            B->Units=Units;B->Squads=Squads;Fleet->Vehicles=Vehicles;B->CampaignRuns=Runs;
            B->ActiveCampaignId=Active;B->EncounterId=Encounter;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;
            B->PlayerFaction=Faction;B->Phase=Phase;B->Elapsed=Clock;B->BattleOutroElapsed=OutroClock;B->BattleOutroWinner=Outro;
            B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;B->ShotsFired=Shots;B->NextSquadId=NextSquad;
            B->Random=Random;B->Strikes=Strikes;B->Tracers=Tracers;B->SmokeZones=Smoke;B->LastMessage=Message;B->AssetFailure=Failure;
            Fleet->LastEncounter=LastEncounter;Fleet->NextEnemyAt=EnemyAt;Fleet->NextSerial=Serial;
            Fleet->DeploymentCountV37[0]=Deploy0;Fleet->DeploymentCountV37[1]=Deploy1;
            Fleet->RailReadyAtV75[0]=Rail0;Fleet->RailReadyAtV75[1]=Rail1;FrontlineMobilityV41::Reset(B);
        };
        B->EncounterId=FGuid::NewGuid();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
        B->Strikes.Reset();B->SmokeZones.Reset();B->Tracers.Reset();Fleet->LastEncounter=B->EncounterId;
        Fleet->NextEnemyAt=Fleet->RailReadyAtV75[0]=Fleet->RailReadyAtV75[1]=MAX_flt;
        auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
        Report->SetStringField(TEXT("scope"),TEXT("Actual UK/DE soldier Tick, fleet Tick and gas escape movement; 10 game seconds with advancing host clock, fine/.2/.4 s frames. Synthetic disposable actors on the prepared actual ground. No natural campaign or performance claim."));
        const auto Record=[&](int32 Nation,const TCHAR* Kind,float Frame,const FSample& S)
        {
            auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("faction"),Nation);J->SetStringField(TEXT("kind"),Kind);
            J->SetNumberField(TEXT("frame_game_seconds"),Frame);J->SetNumberField(TEXT("simulated_game_seconds"),10);
            J->SetNumberField(TEXT("travel_cm"),S.Travel);J->SetStringField(TEXT("end_feet_or_hull"),S.End.ToString());
            J->SetBoolField(TEXT("actual_actor_ready"),S.Ready);J->SetBoolField(TEXT("finite"),S.Finite);
            J->SetNumberField(TEXT("cannon_clock"),S.CannonClock);J->SetNumberField(TEXT("machinegun_clock"),S.GunClock);
            J->SetNumberField(TEXT("soldier_shot_clock"),S.SoldierClock);J->SetBoolField(TEXT("actual_shell_committed"),S.ShellPending);
            J->SetBoolField(TEXT("actual_committed_shell_arrived_by_0_4_game_seconds"),S.ShellArrived);
            J->SetBoolField(TEXT("finite_gas_escape_finished"),S.EscapeFinished);J->SetBoolField(TEXT("contact_keeps_body_gap_and_stride_matches_translation"),S.ContactStride);
            Rows.Add(MakeShared<FJsonValueObject>(J));
        };
        ON_SCOPE_EXIT
        {
            Report->SetArrayField(TEXT("samples"),Rows);FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
            const FString Path=FPaths::ProjectSavedDir()/TEXT("BugAudit/V8219/time-budget-engine.json");
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);FFileHelper::SaveStringToFile(Json,*Path);
        };
        const auto Ground=[&](FVector P){P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);return P;};
        const auto Man=[&](const FVector& P,bool Entered)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            Owned.Add(U);U->Initialize(B,B->NextSquadId++,0,EFrontlineRole::Rifle,0);U->SetActorTickEnabled(false);B->Units.Add(U);
            U->SetActorLocation(Ground(P)+FVector(0,0,88));U->bFieldEntered=Entered;U->ShotClock=U->GrenadeClock=100;
            U->ReactionSeconds=U->DepartureClock=U->AimSettleClock=U->FirePoseClock=0;
            U->bWantedMovement=true;U->bAtDestinationV43=false;U->RequestedTravelSpeedV43=390;
            FFrontlineSquad Q;Q.Id=U->SquadId;Q.Team=0;Q.Role=EFrontlineRole::Rifle;Q.Slot=1;
            Q.Trench=INDEX_NONE;Q.Destination=B->GetHomeTrenchIndex(0);Q.bAdvancing=true;Q.bReinforcement=true;
            Q.LastPassedTrench=Q.Destination-B->GetTeamDirection(0);Q.Members.Add(U);B->Squads.Add(Q);return U;
        };
        const float Frames[]={.05f,.2f,.4f};
        for(int32 Nation=0;Nation<2;++Nation)
        {
            B->PlayerFaction=EFrontlineFaction(Nation);FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=B->PlayerFaction;
            FrontlineCampaignV52::Initialize(R);B->CampaignRuns={R};B->ActiveCampaignId=B->BoundCampaignId=R.Id;
            B->BoundCampaignEncounterId=B->EncounterId;B->BoundCampaignMission=B->BoundAttachmentStageV53=0;
            const int32 Dir=B->GetTeamDirection(0);const float IngressX=-Dir*(FrontlinePacingV24::CombatHalfLength()+5000.f);
            FSample Infantry[3],Armour[3],Gas[3];
            for(int32 Case=0;Case<3;++Case)
            {
                const float Frame=Frames[Case];const int32 Count=FMath::RoundToInt(10.f/Frame);
                Clear();B->Elapsed=1000;auto* U=Man(FVector(IngressX,0,0),false);
                auto& I=Infantry[Case];I.Ready=U&&U->Soldier&&U->Soldier->GetSkeletalMeshAsset();
                if(I.Ready)
                {
                    const FVector Start=U->Feet();
                    for(int32 F=1;F<=Count;++F){B->Elapsed=1000+F*Frame;U->Tick(Frame);I.Finite&=!U->Feet().ContainsNaN();}
                    I.End=U->Feet();I.Travel=FVector::Dist2D(Start,I.End);I.SoldierClock=U->ShotClock;
                }
                Record(Nation,TEXT("ordinary_infantry_ingress"),Frame,I);
                Clear();B->Elapsed=1000;
                auto* V=Fleet->Spawn(Nation?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale,0,false);
                auto& A=Armour[Case];A.Ready=V&&V->AreAssetsReady()&&!V->Cannons.IsEmpty()&&!V->GunClock.IsEmpty();
                if(V)Owned.Add(V);
                if(A.Ready)
                {
                    const float X=B->Trenches[B->GetHomeTrenchIndex(0)].X-Dir*2300.f;
                    V->HeadingYawV37=Dir>0?0.f:180.f;V->PreferredLaneV37=V->DesiredLaneV37=0;V->bEntered=true;
                    V->FitGroundV35(Ground(FVector(X,0,0)),0);for(float& C:V->CannonClock)C=100;for(float& C:V->GunClock)C=100;
                    V->FireCannon(0,V->AimPoint()+FVector(Dir*4000.f,0,0),nullptr);A.ShellPending=V->Shells.Num()==1;
                    for(int32 F=1;F<=Count;++F)
                    {
                        B->Elapsed=1000+F*Frame;Fleet->Tick(Frame);A.Finite&=!V->GetActorLocation().ContainsNaN();
                        if(FMath::IsNearlyEqual(F*Frame,.4f,.001f))A.ShellArrived=V->Shells.IsEmpty();
                    }
                    A.End=V->GetActorLocation();A.Travel=V->DistanceTravelled;A.CannonClock=V->CannonClock[0];A.GunClock=V->GunClock[0];
                }
                Record(Nation,TEXT("actual_fleet_movement_and_weapon_clocks"),Frame,A);
                Clear();B->Elapsed=1000;U=Man(FVector(IngressX,0,0),true);auto& G=Gas[Case];
                G.Ready=U&&U->Soldier&&U->Soldier->GetSkeletalMeshAsset()&&FrontlineMotionV40::PrepareGasEscapeTimeBudgetReviewV8219(U);
                if(G.Ready)
                {
                    const FVector Start=U->Feet();
                    for(int32 F=1;F<=Count;++F)
                    {B->Elapsed=1000+F*Frame;FrontlineMotionV40::TickGasEscapeV8216(U,Frame);G.Finite&=!U->Feet().ContainsNaN();}
                    G.End=U->Feet();G.Travel=FVector::Dist2D(Start,G.End);G.EscapeFinished=!FrontlineMotionV40::GasExposureV8215(U).Escaping;
                    auto* Blocker=Man(U->Feet()+FVector(0,56.f,0),true);
                    if(Blocker)
                    {
                        Blocker->SetActorLocation(Ground(U->Feet()+FVector(0,56.f,0))+FVector(0,0,88));
                        FrontlineMobilityV41::Reset(B);FrontlineMotionV40::PrepareGasEscapeTimeBudgetReviewV8219(U);
                        const FVector Before=U->Feet();FrontlineMotionV40::TickGasEscapeV8216(U,Frame);
                        const float Travel=FVector::Dist2D(Before,U->Feet());
                        G.ContactStride=FVector::Dist2D(U->Feet(),Blocker->Feet())>=55.99f
                            &&FMath::IsNearlyEqual(U->MotionSpeed,Travel/Frame,.01f)&&U->bMoving==(Travel/Frame>8.f);
                    }
                }
                Record(Nation,TEXT("actual_finite_gas_escape"),Frame,G);
            }
            for(int32 Case=1;Case<3;++Case)
            {
                const FString P=FString::Printf(TEXT("v8219_%s_frame_%.1f_"),Nation?TEXT("DE"):TEXT("UK"),Frames[Case]);
                Check(P+TEXT("actual_ingress_keeps_ten_second_travel_budget"),Infantry[0].Ready&&Infantry[Case].Ready&&Infantry[Case].Finite
                    &&Infantry[0].Travel>3800&&FMath::Abs(Infantry[Case].Travel-Infantry[0].Travel)<=3.f);
                Check(P+TEXT("movement_substeps_do_not_repeat_soldier_weapon_clock"),Infantry[0].Ready&&Infantry[Case].Ready
                    &&FMath::IsNearlyEqual(Infantry[Case].SoldierClock,Infantry[0].SoldierClock,.02f)
                    &&FMath::IsNearlyEqual(Infantry[Case].SoldierClock,90.f,.02f));
                Check(P+TEXT("actual_armour_keeps_travel_and_both_weapon_clocks"),Armour[0].Ready&&Armour[Case].Ready&&Armour[Case].Finite
                    &&Armour[0].Travel>100&&FMath::Abs(Armour[Case].Travel-Armour[0].Travel)<=15.f
                    &&FMath::IsNearlyEqual(Armour[Case].CannonClock,Armour[0].CannonClock,.02f)
                    &&FMath::IsNearlyEqual(Armour[Case].GunClock,Armour[0].GunClock,.02f)
                    &&FMath::IsNearlyEqual(Armour[Case].CannonClock,90.f,.02f));
                Check(P+TEXT("actual_shell_arrives_on_complete_game_clock"),Armour[0].ShellPending&&Armour[Case].ShellPending
                    &&Armour[0].ShellArrived&&Armour[Case].ShellArrived);
                Check(P+TEXT("actual_gas_escape_keeps_finite_swept_travel_and_stride"),Gas[0].Ready&&Gas[Case].Ready&&Gas[Case].Finite
                    &&Gas[0].Travel>400&&FMath::Abs(Gas[Case].Travel-Gas[0].Travel)<=3.f&&Gas[Case].EscapeFinished&&Gas[Case].ContactStride);
            }
        }
    }
};
static void RunFrontlineTimeBudgetChecksV8219(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineTimeBudgetV8219Review::Run(B,Check);}
