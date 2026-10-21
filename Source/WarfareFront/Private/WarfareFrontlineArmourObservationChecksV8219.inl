#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineMobilityV41.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include <cfloat>

// Runs on actual prepared UK/DE terrain and actual production vehicle actors.
// The only fabricated object is a visibility blocker with a measured top edge.
struct FFrontlineArmourObservationV8219Review
{
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review")))
        {Check(TEXT("v8219_armour_observation_requires_exact_disposable_profile"),false);return;}
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        if(!Fleet||B->Trenches.IsEmpty())
        {Check(TEXT("v8219_armour_observation_requires_prepared_actual_field"),false);return;}
        const auto Units=B->Units;const auto Squads=B->Squads;const auto Vehicles=Fleet->Vehicles;
        const auto Runs=B->CampaignRuns;const auto Active=B->ActiveCampaignId,Encounter=B->EncounterId;
        const auto Bound=B->BoundCampaignId,BoundEncounter=B->BoundCampaignEncounterId;
        const auto Faction=B->PlayerFaction;const auto Phase=B->Phase;
        const auto Strikes=B->Strikes;const auto Tracers=B->Tracers;const auto Smoke=B->SmokeZones;
        const float Clock=B->Elapsed,OutroClock=B->BattleOutroElapsed;const int32 Outro=B->BattleOutroWinner;
        const int32 Mission=B->BoundCampaignMission,Attachment=B->BoundAttachmentStageV53,Shots=B->ShotsFired;
        const auto Random=B->Random;const auto LastEncounter=Fleet->LastEncounter;
        const int32 Serial=Fleet->NextSerial,Deploy0=Fleet->DeploymentCountV37[0],Deploy1=Fleet->DeploymentCountV37[1];
        const FString Message=B->LastMessage,Failure=B->AssetFailure;
        TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
        struct FVisibility {AActor* Actor;bool Hidden,Collision;};TArray<FVisibility> Visibility;
        for(AWarfareFrontlineSoldier* U:Units)if(IsValid(U)){Visibility.Add({U,U->IsHidden(),U->GetActorEnableCollision()});U->SetActorHiddenInGame(true);U->SetActorEnableCollision(false);}
        for(AFrontlineTank* V:Vehicles)if(IsValid(V)){Visibility.Add({V,V->IsHidden(),V->GetActorEnableCollision()});V->SetActorHiddenInGame(true);V->SetActorEnableCollision(false);}
        TArray<AActor*> Owned;
        const auto Clear=[&]()
        {
            for(auto* A:Owned)if(IsValid(A))A->Destroy();Owned.Reset();
            B->Units.Reset();B->Squads.Reset();Fleet->Vehicles.Reset();B->Tracers.Reset();FrontlineMobilityV41::Reset(B);
        };
        ON_SCOPE_EXIT
        {
            Clear();for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
            for(const auto& V:Visibility)if(IsValid(V.Actor)){V.Actor->SetActorHiddenInGame(V.Hidden);V.Actor->SetActorEnableCollision(V.Collision);}
            B->Units=Units;B->Squads=Squads;Fleet->Vehicles=Vehicles;B->CampaignRuns=Runs;
            B->ActiveCampaignId=Active;B->EncounterId=Encounter;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;
            B->PlayerFaction=Faction;B->Phase=Phase;B->Elapsed=Clock;B->BattleOutroElapsed=OutroClock;B->BattleOutroWinner=Outro;
            B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;B->ShotsFired=Shots;
            B->Random=Random;B->Strikes=Strikes;B->Tracers=Tracers;B->SmokeZones=Smoke;B->LastMessage=Message;B->AssetFailure=Failure;
            Fleet->LastEncounter=LastEncounter;Fleet->NextSerial=Serial;Fleet->DeploymentCountV37[0]=Deploy0;Fleet->DeploymentCountV37[1]=Deploy1;
            FrontlineMobilityV41::Reset(B);
        };
        B->EncounterId=FGuid::NewGuid();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
        B->Strikes.Reset();B->SmokeZones.Reset();B->Tracers.Reset();Fleet->LastEncounter=B->EncounterId;
        TArray<TSharedPtr<FJsonValue>> Rows;
        ON_SCOPE_EXIT
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("scope"),TEXT("Real UK/DE vehicle actors, real muzzle visibility traces and production Simulate. A geometric screen blocks the 160 cm centre but exposes the 200 cm hull panel. No player archive or campaign content acceptance."));
            J->SetArrayField(TEXT("cases"),Rows);FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));
            const FString Path=FPaths::ProjectSavedDir()/TEXT("BugAudit/V8219/armour-observation-engine.json");
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);FFileHelper::SaveStringToFile(Json,*Path);
        };
        for(int32 Nation=0;Nation<2;++Nation)
        {
            Clear();B->Elapsed=1000;B->PlayerFaction=EFrontlineFaction(Nation);
            FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=B->PlayerFaction;FrontlineCampaignV52::Initialize(R);
            B->CampaignRuns={R};B->ActiveCampaignId=B->BoundCampaignId=R.Id;
            B->BoundCampaignEncounterId=B->EncounterId;B->BoundCampaignMission=B->BoundAttachmentStageV53=0;
            auto* Shooter=Fleet->Spawn(Nation?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale,0,false);
            auto* Enemy=Fleet->Spawn(Nation?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V,1,false);
            if(Shooter)Owned.Add(Shooter);if(Enemy)Owned.Add(Enemy);
            const FString Prefix=Nation?TEXT("v8219_DE_"):TEXT("v8219_UK_");
            const bool Assets=Shooter&&Enemy&&Shooter->AreAssetsReady()&&Enemy->AreAssetsReady()&&!Shooter->Cannons.IsEmpty();
            Check(Prefix+TEXT("exposed_hull_actual_vehicle_assets"),Assets);if(!Assets)continue;
            Shooter->SetActorTickEnabled(false);Enemy->SetActorTickEnabled(false);
            const int32 Dir=B->GetTeamDirection(0);const float Heading=Dir>0?0.f:180.f;
            Shooter->HeadingYawV37=Heading;Enemy->HeadingYawV37=Heading+180.f;Shooter->bEntered=Enemy->bEntered=true;
            bool Baseline=false;
            const auto Visible=[&](const FVector& From,const FVector& To)
            {
                FCollisionQueryParams Q(SCENE_QUERY_STAT(V8219ExposedHull),true);Q.AddIgnoredActor(Shooter);Q.AddIgnoredActor(Enemy);
                for(const AWarfareFrontlineSoldier* U:Units)if(U)Q.AddIgnoredActor(U);
                for(const AFrontlineTank* V:Vehicles)if(V)Q.AddIgnoredActor(V);
                FHitResult Hit;return !B->GetWorld()->LineTraceSingleByChannel(Hit,From,To,ECC_Visibility,Q);
            };
            for(const float Y:{-600.f,0.f,600.f})
            {
                Shooter->FitGroundV35(FVector(-1500.f*Dir,Y,0),0);Enemy->FitGroundV35(FVector(1500.f*Dir,Y,0),0);
                Baseline=true;
                for(int32 I=0;I<Shooter->Cannons.Num();++I)
                    Baseline&=Visible(Shooter->WeaponMuzzle(I,true),Enemy->AimPoint())
                        &&Visible(Shooter->WeaponMuzzle(I,true),Enemy->GetActorTransform().TransformPosition(FVector(0,0,200)));
                if(Baseline)break;
            }
            Check(Prefix+TEXT("exposed_hull_unobstructed_actual_baseline"),Baseline);if(!Baseline)continue;
            auto* Wall=B->GetWorld()->SpawnActor<AActor>();if(!Wall){Check(Prefix+TEXT("exposed_hull_screen_spawned"),false);continue;}
            Owned.Add(Wall);auto* Block=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Block);Wall->AddInstanceComponent(Block);
            Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Block->SetCollisionResponseToAllChannels(ECR_Ignore);
            Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Block->RegisterComponent();
            const FVector High=Enemy->GetActorTransform().TransformPosition(FVector(0,0,200));
            const float WallX=Enemy->GetActorLocation().X-Dir*600.f;
            double LowTop=-DBL_MAX,HighBottom=DBL_MAX;
            for(int32 I=0;I<Shooter->Cannons.Num();++I)
            {
                const FVector Muzzle=Shooter->WeaponMuzzle(I,true);
                const double T=(WallX-Muzzle.X)/(Enemy->GetActorLocation().X-Muzzle.X);
                for(const FVector& Low:{Enemy->AimPoint(),Enemy->GetActorTransform().TransformPosition(FVector(0,0,160)),Enemy->GetActorTransform().TransformPosition(FVector(0,0,120))})
                    LowTop=FMath::Max(LowTop,FMath::Lerp(Muzzle,Low,T).Z);
                HighBottom=FMath::Min(HighBottom,FMath::Lerp(Muzzle,High,T).Z);
            }
            const float Top=float((LowTop+HighBottom)*.5),Bottom=Top-1500.f;
            Block->SetBoxExtent(FVector(8,1800,(Top-Bottom)*.5f));Block->SetWorldLocation(FVector(WallX,Enemy->GetActorLocation().Y,(Top+Bottom)*.5f));
            bool CentreBlocked=true,UpperClear=true,EveryMount=true;
            for(int32 I=0;I<Shooter->Cannons.Num();++I)
            {
                CentreBlocked&=!Visible(Shooter->WeaponMuzzle(I,true),Enemy->AimPoint());
                UpperClear&=Visible(Shooter->WeaponMuzzle(I,true),High);
                FVector Solution;EveryMount&=Shooter->ArmourFiringSolutionV8219(Enemy,I,Solution)&&Solution.Equals(High,.1f);
                Shooter->CannonClock[I]=0;
            }
            Check(Prefix+TEXT("screen_really_blocks_160_and_exposes_200_at_every_actual_muzzle"),HighBottom-LowTop>4&&CentreBlocked&&UpperClear);
            Check(Prefix+TEXT("every_actual_cannon_keeps_the_exposed_hull_solution"),EveryMount);
            // Keep the physical scatter, but select a deterministic upward
            // sample so a legitimate random low miss cannot hide the aim-point
            // regression. Each cannon consumes lateral, vertical and cycle draws.
            for(int32 Seed=1;Seed<10000;++Seed)
            {
                FRandomStream Probe(Seed);bool Upward=true;
                for(int32 I=0;I<Shooter->Cannons.Num();++I)
                {Probe.FRand();Upward&=Probe.FRand()>.75f;Probe.FRand();}
                if(Upward){Shooter->Random.Initialize(Seed);break;}
            }
            const FVector Start=Shooter->GetActorLocation();const int32 BeforeShots=Shooter->CannonShots;
            for(int32 Frame=1;Frame<=100&&Shooter->CannonShots==BeforeShots;++Frame)
            {B->Elapsed=1000+Frame*.05f;Shooter->Simulate(.05f);}
            const bool Fired=Shooter->CannonShots>BeforeShots&&!Shooter->Shells.IsEmpty();
            Check(Prefix+TEXT("production_simulate_holds_and_commits_a_real_shell_at_exposed_hull"),CentreBlocked&&UpperClear&&Fired&&FVector::Dist2D(Start,Shooter->GetActorLocation())<1.f);
            bool CrossesExposedEdge=Fired;
            for(const auto& Shell:Shooter->Shells)
            {
                const double DX=Shell.Impact.X-Shell.From.X;
                const double T=FMath::Abs(DX)>1.e-6?(WallX-Shell.From.X)/DX:-1.;
                CrossesExposedEdge&=T>0&&T<1&&FMath::Lerp(Shell.From,Shell.Impact,T).Z>Top;
            }
            Check(Prefix+TEXT("actual_committed_shells_cross_the_exposed_screen_edge"),CrossesExposedEdge);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("army"),Nation?TEXT("DE"):TEXT("UK"));
            J->SetBoolField(TEXT("160_blocked"),CentreBlocked);J->SetBoolField(TEXT("200_visible"),UpperClear);J->SetBoolField(TEXT("every_mount_has_200_solution"),EveryMount);
            J->SetNumberField(TEXT("production_cannon_shots"),Shooter->CannonShots-BeforeShots);J->SetNumberField(TEXT("hull_travel_cm"),FVector::Dist2D(Start,Shooter->GetActorLocation()));
            J->SetStringField(TEXT("screen_location"),Block->GetComponentLocation().ToString());J->SetStringField(TEXT("upper_hull_point"),High.ToString());Rows.Add(MakeShared<FJsonValueObject>(J));
            // Keep the committed first shell from damaging the fixture enemy while
            // testing exclusions. Production ResolveShell is covered separately.
            for(auto& Shell:Shooter->Shells)if(auto* Visual=Shell.Visual.Get())Visual->DestroyComponent();Shooter->Shells.Reset();
            Block->SetBoxExtent(FVector(8,1800,1500));
            const int32 BlockedShots=Shooter->CannonShots;Shooter->ArmourContactV48.Reset();Shooter->ArmourContactAgeV48=2;
            for(int32 Frame=1;Frame<=8;++Frame){B->Elapsed+=.05f;for(int32 I=0;I<Shooter->Cannons.Num();++I)Shooter->CannonClock[I]=0;Shooter->Simulate(.05f);}
            FVector Point;bool NoBlockedMount=true;
            for(int32 I=0;I<Shooter->Cannons.Num();++I)NoBlockedMount&=!Shooter->ArmourFiringSolutionV8219(Enemy,I,Point);
            Check(Prefix+TEXT("fully_obstructed_actual_hull_cannot_authorize_a_shell"),NoBlockedMount&&Shooter->CannonShots==BlockedShots);
            Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            const bool HostileClear=Shooter->ArmourFiringSolutionV8219(Enemy,0,Point);Enemy->Team=0;
            Check(Prefix+TEXT("exposed_panel_never_authorizes_a_friendly_hull"),HostileClear&&!Shooter->ArmourFiringSolutionV8219(Enemy,0,Point));Enemy->Team=1;
            Shooter->TrackIntegrity[0]=Shooter->TrackIntegrity[1]=0;
            Shooter->HeadingYawV37=Heading+180.f;Shooter->FitGroundV35(Shooter->GetActorLocation(),0);
            bool NoRearMount=true;
            for(int32 I=0;I<Shooter->Cannons.Num();++I)NoRearMount&=!Shooter->ArmourFiringSolutionV8219(Enemy,I,Point);
            const int32 RearShots=Shooter->CannonShots;
            for(int32 Frame=1;Frame<=8;++Frame){B->Elapsed+=.05f;for(int32 I=0;I<Shooter->Cannons.Num();++I)Shooter->CannonClock[I]=0;Shooter->Simulate(.05f);}
            Check(Prefix+TEXT("immobilized_rear_contact_keeps_actual_cannon_arc_gate"),NoRearMount&&Shooter->CannonShots==RearShots);
        }
    }
};
static void RunFrontlineArmourObservationChecksV8219(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineArmourObservationV8219Review::Run(B,Check);}
