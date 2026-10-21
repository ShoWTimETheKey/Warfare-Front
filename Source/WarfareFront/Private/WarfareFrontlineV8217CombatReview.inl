#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineMotionV40.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"

struct FFrontlineCombatV8217Review
{
    static void ClearArtFixture(AWarfareFrontlineGameMode* B,const TArray<TWeakObjectPtr<AFrontlineTank>>& Vehicles)
    {
        if(!FrontlineEffectsReviewSafetyV8216::Allowed(B))return;
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        for(const auto& W:Vehicles)if(auto* V=W.Get()){if(Fleet)Fleet->Vehicles.Remove(V);V->Destroy();}
    }
    static TArray<AFrontlineTank*> ArtFixture(AWarfareFrontlineGameMode* B)
    {
        TArray<AFrontlineTank*> Result;
        auto* Fleet=B?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;
        if(!Fleet||!FrontlineEffectsReviewSafetyV8216::Allowed(B))return Result;
        const int32 Team=B->GetFactionForTeam(0)==EFrontlineFaction::German?0:1;
        // Invoke on the prepared field, before UI fixtures change the campaign
        // profile. HeightAt and the rendered ground then share the same basis.
        // Do not position by B->Trenches: a UI-only seed can leave those stale.
        const float Centre=FrontlineBattlefield::TrenchCount>0
            ?(FrontlineBattlefield::TrenchX(0)+FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1))*.5f:0.f;
        const float Home=FrontlineBattlefield::TrenchCount>0
            ?FMath::Max(FMath::Abs(FrontlineBattlefield::TrenchX(0)),FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1))):1900.f;
        auto Diagnostic=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Searches;
        int32 Profile;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Profile,Style);
        Diagnostic->SetNumberField(TEXT("ground_profile"),Profile);Diagnostic->SetNumberField(TEXT("trench_count"),FrontlineBattlefield::TrenchCount);
        Diagnostic->SetNumberField(TEXT("home_cm"),Home);
        ON_SCOPE_EXIT
        {
            Diagnostic->SetArrayField(TEXT("searches"),Searches);Diagnostic->SetNumberField(TEXT("actual_pair_count"),Result.Num());
            FString JSON;FJsonSerializer::Serialize(Diagnostic,TJsonWriterFactory<>::Create(&JSON));
            FFileHelper::SaveStringToFile(JSON,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V8217/beute-art-fixture.json")));
        };
        TArray<FVector> Positions;
        for(int32 I=0;I<2;++I)
        {
            // Use the genuine rear apron, away from Menin's cratered forward
            // ridge. Both vehicles stay in one readable tactical view.
            const FVector Desired=Positions.IsEmpty()?FVector(Centre-Home-2500.f,-450.f,0):Positions[0]+FVector(1300.f,0,-Positions[0].Z);
            float BestScore=MAX_flt;FVector Best=FVector::ZeroVector;bool Found=false;
            int32 Candidates=0,NearOther=0,TrenchRejected=0,WaterRejected=0,ShapeRejected=0,Nonfinite=0;
            float LeastResidual=MAX_flt,LeastSpread=MAX_flt,BestResidual=0,BestPitch=0,BestRoll=0;
            for(int32 IX=-14;IX<=14;++IX)for(int32 IY=-7;IY<=7;++IY)
            {
                const FVector Candidate(Desired.X+IX*180.f,Desired.Y+IY*150.f,0);
                if(FMath::Abs(Candidate.X)+400.f>=FrontlineBattlefield::GroundHalfLength)continue;
                ++Candidates;
                if(!Positions.IsEmpty()&&(FVector::DistSquared2D(Candidate,Positions[0])<FMath::Square(1200.f)
                    ||FVector::DistSquared2D(Candidate,Positions[0])>FMath::Square(1600.f)||FMath::Abs(Candidate.Y-Positions[0].Y)>300.f)){++NearOther;continue;}
                bool Clear=true;float Low=MAX_flt,High=-MAX_flt,Mean=0,SlopeX=0,SlopeY=0;
                float Heights[5][3]{};int32 Reason=0;
                // Complete vehicle footprint, including tracks and sponsons.
                for(int32 SX=-2;SX<=2&&Clear;++SX)for(int32 SY=-1;SY<=1&&Clear;++SY)
                {
                    const float X=Candidate.X+SX*200.f,Y=Candidate.Y+SY*230.f;
                    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
                        if(FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(T,Y))<420.f){Clear=false;Reason=1;break;}
                    float Level=0,Depth=0;
                    if(!Clear)break;
                    if(AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth)&&Depth>3.f){Clear=false;Reason=2;break;}
                    const float H=AWarfareFrontlineGround::HeightAt(X,Y);
                    if(!FMath::IsFinite(H)){Clear=false;Reason=3;break;}
                    Heights[SX+2][SY+1]=H;Mean+=H/15.f;SlopeX+=SX*200.f*H/1200000.f;SlopeY+=SY*230.f*H/529000.f;
                    Low=FMath::Min(Low,H);High=FMath::Max(High,H);
                }
                if(!Clear){if(Reason==1)++TrenchRejected;else if(Reason==2)++WaterRejected;else ++Nonfinite;continue;}
                float Residual=0;for(int32 SX=-2;SX<=2;++SX)for(int32 SY=-1;SY<=1;++SY)
                    Residual=FMath::Max(Residual,FMath::Abs(Heights[SX+2][SY+1]-(Mean+SlopeX*SX*200.f+SlopeY*SY*230.f)));
                const float Pitch=FMath::Abs(FMath::RadiansToDegrees(FMath::Atan(SlopeX))),Roll=FMath::Abs(FMath::RadiansToDegrees(FMath::Atan(SlopeY)));
                LeastResidual=FMath::Min(LeastResidual,Residual);LeastSpread=FMath::Min(LeastSpread,High-Low);
                // A smooth sloping road is a flat plane: raw min/max spread
                // wrongly rejected it. Reject hollows/bumps and steep slopes.
                if(Residual>18.f||Pitch>8.f||Roll>6.f){++ShapeRejected;continue;}
                const float Score=Residual*15.f+Pitch*3.f+Roll*3.f+FVector::Dist2D(Candidate,Desired)*.04f;
                if(Score<BestScore){BestScore=Score;Best=Candidate;Found=true;BestResidual=Residual;BestPitch=Pitch;BestRoll=Roll;}
            }
            auto Search=MakeShared<FJsonObject>();Search->SetNumberField(TEXT("vehicle"),I);Search->SetBoolField(TEXT("found"),Found);
            Search->SetNumberField(TEXT("candidates"),Candidates);Search->SetNumberField(TEXT("other_vehicle_rejected"),NearOther);
            Search->SetNumberField(TEXT("trench_rejected"),TrenchRejected);Search->SetNumberField(TEXT("water_rejected"),WaterRejected);
            Search->SetNumberField(TEXT("shape_rejected"),ShapeRejected);Search->SetNumberField(TEXT("nonfinite_rejected"),Nonfinite);
            Search->SetNumberField(TEXT("minimum_plane_residual_cm"),LeastResidual==MAX_flt?-1.f:LeastResidual);
            Search->SetNumberField(TEXT("minimum_raw_height_spread_cm"),LeastSpread==MAX_flt?-1.f:LeastSpread);
            Search->SetNumberField(TEXT("chosen_residual_cm"),BestResidual);Search->SetNumberField(TEXT("chosen_pitch_deg"),BestPitch);Search->SetNumberField(TEXT("chosen_roll_deg"),BestRoll);
            Search->SetNumberField(TEXT("x"),Best.X);Search->SetNumberField(TEXT("y"),Best.Y);Searches.Add(MakeShared<FJsonValueObject>(Search));
            // A failed real footprint search is observable as a missing pair;
            // never hide a placement failure by lifting or shrinking the hull.
            if(!Found)return Result;
            Best.Z=AWarfareFrontlineGround::HeightAt(float(Best.X),float(Best.Y));Positions.Add(Best);
        }
        const EFrontlineVehicle Types[]={EFrontlineVehicle::BeuteMarkIV,EFrontlineVehicle::BeuteMarkIVFemale};
        for(int32 I=0;I<2;++I)
        {
            auto* V=Fleet->Spawn(Types[I],Team,true);
            auto Spawn=MakeShared<FJsonObject>();Spawn->SetNumberField(TEXT("vehicle"),I);Spawn->SetBoolField(TEXT("spawned"),V!=nullptr);
            Spawn->SetStringField(TEXT("asset_failure"),B->AssetFailure);Diagnostic->SetObjectField(I?TEXT("female"):TEXT("male"),Spawn);
            if(!V)continue;
            const FVector P=Positions[I];
            V->HeadingYawV37=I?180.f:0.f;V->PreferredLaneV37=V->DesiredLaneV37=float(P.Y);V->bEntered=true;
            V->SetActorLocation(P);V->FitGroundV35(P,0);V->SetActorTickEnabled(false);
            const FString Expected=I?TEXT("SM_BeuteMarkIVFemale_V8217"):TEXT("SM_BeuteMarkIV_V8217");
            Spawn->SetStringField(TEXT("actual_mesh"),V->Body&&V->Body->GetStaticMesh()?V->Body->GetStaticMesh()->GetName():TEXT("missing"));
            Spawn->SetNumberField(TEXT("fitted_z"),V->GetActorLocation().Z);
            if(!V->Body||!V->Body->GetStaticMesh()||V->Body->GetStaticMesh()->GetName()!=Expected)
            {V->Destroy();Fleet->Vehicles.Remove(V);continue;}
            Result.Add(V);
        }
        return Result;
    }
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        auto* Fleet=B?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;
        const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B)&&Fleet;
        Check(TEXT("v8217_combat_disposable_actual_world"),Safe);if(!Safe)return;
        const auto Units=B->Units;const auto Squads=B->Squads;const auto OldVehicles=Fleet->Vehicles;
        const auto Runs=B->CampaignRuns;const auto Faction=B->PlayerFaction;
        const auto Active=B->ActiveCampaignId,Bound=B->BoundCampaignId,Binding=B->BoundCampaignEncounterId;
        const int32 Mission=B->BoundCampaignMission,Attachment=B->BoundAttachmentStageV53,Serial=Fleet->NextSerial;
        const auto Phase=B->Phase;const float Elapsed=B->Elapsed,Outro=B->BattleOutroElapsed,Enemy=B->EnemyThinkAt,Extended=B->ExtendedSupportThinkAt;
        const int32 Winner=B->BattleOutroWinner,Shots=B->ShotsFired,NextSquad=B->NextSquadId,ResearchBefore=B->ResearchBeforeOutcomeV828;
        const auto Random=B->Random;const auto OldStrikes=B->Strikes;const auto Smoke=B->SmokeZones;const auto Tracers=B->Tracers;
        const FString Message=B->LastMessage,EndReason=B->EndReason;
        const int32 Casualty[2]={B->Casualties[0],B->Casualties[1]};const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];
        const float Morale[2]={B->Morale[0],B->Morale[1]},Damage[2]={B->DamageDealt[0],B->DamageDealt[1]};
        const float Friendly=B->FriendlyFireDamage,Peak=B->PeakSuppression,EnemyAt=Fleet->NextEnemyAt;
        const float ArtilleryReady=B->ArtilleryReadyAt[0];const int32 ArtilleryCommands=B->ArtilleryCommands;
        const float BattleSpeed=B->BattleSpeed;const bool Paused=UGameplayStatics::IsGamePaused(B);
        const auto Encounter=Fleet->LastEncounter;const int32 Deployments[2]={Fleet->DeploymentCountV37[0],Fleet->DeploymentCountV37[1]};
        float Ready[2][int32(EFrontlineVehicle::Count)];FMemory::Memcpy(Ready,Fleet->ReadyAt,sizeof(Ready));
        const float Cycle[2]={Fleet->ArmourCycleSecondsV40[0],Fleet->ArmourCycleSecondsV40[1]};
        TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
        struct FVisibility{AActor* Actor;bool Hidden,Collision;};TArray<FVisibility> Visibility;
        const auto Hide=[&](AActor* A){if(IsValid(A)){Visibility.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
        for(AWarfareFrontlineSoldier* U:Units)Hide(U);for(AFrontlineTank* V:OldVehicles)Hide(V);
        ON_SCOPE_EXIT
        {
            for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
            for(const auto& S:Visibility)if(IsValid(S.Actor)){S.Actor->SetActorHiddenInGame(S.Hidden);S.Actor->SetActorEnableCollision(S.Collision);}
            B->Units=Units;B->Squads=Squads;Fleet->Vehicles=OldVehicles;B->CampaignRuns=Runs;B->PlayerFaction=Faction;
            B->ActiveCampaignId=Active;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=Binding;B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;
            B->Phase=Phase;B->Elapsed=Elapsed;B->BattleOutroElapsed=Outro;B->BattleOutroWinner=Winner;B->EnemyThinkAt=Enemy;B->ExtendedSupportThinkAt=Extended;
            B->NextSquadId=NextSquad;B->ShotsFired=Shots;B->ResearchBeforeOutcomeV828=ResearchBefore;B->Random=Random;B->Strikes=OldStrikes;B->SmokeZones=Smoke;B->Tracers=Tracers;
            B->LastMessage=Message;B->EndReason=EndReason;B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;
            B->FriendlyFireDamage=Friendly;B->PeakSuppression=Peak;Fleet->LastEncounter=Encounter;Fleet->NextEnemyAt=EnemyAt;Fleet->NextSerial=Serial;
            B->ArtilleryReadyAt[0]=ArtilleryReady;B->ArtilleryCommands=ArtilleryCommands;
            B->SetBattleSpeed(BattleSpeed);UGameplayStatics::SetGamePaused(B,Paused);
            FMemory::Memcpy(Fleet->ReadyAt,Ready,sizeof(Ready));
            for(int32 T=0;T<2;++T){B->Casualties[T]=Casualty[T];B->Morale[T]=Morale[T];B->DamageDealt[T]=Damage[T];Fleet->ArmourCycleSecondsV40[T]=Cycle[T];Fleet->DeploymentCountV37[T]=Deployments[T];}
        };
        B->Units.Empty();B->Squads.Empty();Fleet->Vehicles.Empty();B->Strikes.Empty();B->SmokeZones.Empty();
        B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;B->Elapsed=1000;
        B->EnemyThinkAt=B->ExtendedSupportThinkAt=MAX_flt;Fleet->NextEnemyAt=MAX_flt;Fleet->LastEncounter=B->GetEncounterId();Fleet->NextSerial=821700;
        const auto Clear=[&]()
        {
            for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&!Existing.Contains(U))U->Destroy();B->Units.Empty();B->Squads.Empty();
            for(AFrontlineTank* V:Fleet->Vehicles)if(IsValid(V)&&!Existing.Contains(V))V->Destroy();Fleet->Vehicles.Empty();
            for(auto& S:B->Strikes)if(S.Projectile.IsValid())S.Projectile->Destroy();B->Strikes.Empty();B->SmokeZones.Empty();
        };
        const auto Select=[&](int32 F)
        {
            FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=EFrontlineFaction(F);R.SelectedMission=R.CompletedMissions=19;R.ArmourResearchMaskV63=0x1ff;
            B->CampaignRuns={R};B->PlayerFaction=R.Faction;B->ActiveCampaignId=B->BoundCampaignId=R.Id;
            B->BoundCampaignEncounterId=B->GetEncounterId();B->BoundCampaignMission=B->BoundAttachmentStageV53=19;
        };
        auto Diagnostic=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> RecruitRows,DuelRows;
        for(int32 F=0;F<2;++F)
        {
            Select(F);const auto Roster=Fleet->GetRoster();float Minimum=MAX_flt,Maximum=0;
            for(const auto& D:Roster)
            {
                Clear();B->CampaignRuns[0].ArmourSelectionMaskV63=1u<<int32(D.Type);
                Fleet->ResetReadinessForDebug(0);const float Start=B->Elapsed;
                const float Expected=FrontlineGameplayV43::PreparationSeconds(D.Cooldown*FrontlinePacingV24::TempoIntervalScale*FrontlineArmourV63::ReadinessScale(B,D.Type,0),0,B->GetCampaignDifficulty());
                const bool Recruited=Fleet->Recruit(D.Type,0);const float Actual=Fleet->ReadyAt[0][0]-Start;
                bool Shared=Recruited;for(float At:Fleet->ReadyAt[0])Shared&=FMath::IsNearlyEqual(At,Start+Actual,.001f);
                B->Elapsed=Start+Actual*.5f;const bool Half=FMath::IsNearlyEqual(Fleet->CooldownFraction(D.Type,0),.5f,.002f);
                const bool Blocked=!Fleet->Recruit(D.Type,0);Clear();B->Elapsed=Start+Actual+.001f;
                const bool Cleared=Fleet->CooldownFraction(D.Type,0)<.001f&&Fleet->RecruitBlockReason(D.Type,0).IsEmpty();
                Check(FString::Printf(TEXT("v8217_player_type%d_real_recruit_shared_queue_model_duration"),int32(D.Type)),Recruited&&Shared&&Half&&Blocked&&Cleared&&FMath::IsNearlyEqual(Actual,Expected,.003f));
                auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("type"),int32(D.Type));J->SetNumberField(TEXT("base_seconds"),D.Cooldown);J->SetNumberField(TEXT("actual_seconds"),Actual);J->SetNumberField(TEXT("expected_seconds"),Expected);RecruitRows.Add(MakeShared<FJsonValueObject>(J));
                Minimum=FMath::Min(Minimum,Actual);Maximum=FMath::Max(Maximum,Actual);B->Elapsed=Start;
            }
            Check(FString::Printf(TEXT("v8217_army%d_player_model_cycles_are_not_identical"),F),Roster.Num()==5&&Maximum>Minimum*1.4f);
        }
        Clear();Select(1);
        const float Dir=B->GetTeamDirection(0),Home=B->Trenches[B->GetHomeTrenchIndex(0)].X;
        const float Next=B->Trenches[B->GetHomeTrenchIndex(0)+(Dir>0?1:-1)].X;
        const float Middle=(Home+Next)*.5f;
        const auto Ground=[](float X,float Y){return FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y));};
        const auto MakeMan=[&](int32 Team,const FVector& P)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            U->Initialize(B,++B->NextSquadId,Team,EFrontlineRole::Rifle,0);U->SetActorLocation(P+FVector(0,0,88));U->bFieldEntered=true;U->SetActorTickEnabled(false);B->Units.Add(U);return U;
        };
        const FVector Target=Ground(Home+Dir*1200.f,300);
        auto* Gun=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
        const bool Configured=Gun&&Gun->Configure(B,EFrontlineSupport::AntiTank,Target,0);
        Check(TEXT("v8217_antitank_real_ground_mission_configures_without_armour"),Configured&&Fleet->Vehicles.IsEmpty());
        if(Configured)
        {
            B->Elapsed=Gun->RequestedAt+Gun->Arrival+.01f;Gun->Tick(.01f);
            const bool Launched=Gun->RoundsFired==1&&B->Strikes.Num()==1;
            Check(TEXT("v8217_antitank_ground_call_really_launches_future_physical_shell"),Launched&&B->Strikes[0].Projectile.IsValid()&&B->Strikes[0].ImpactAt>B->Elapsed);
            if(Launched)
            {
                const auto Strike=B->Strikes[0];auto* Victim=MakeMan(1,Ground(Strike.Target.X,Strike.Target.Y));
                const float Chance=Victim?B->ExplosionHitChance(Strike,Victim):0;
                bool Seeded=false;for(int32 Seed=1;Seed<5000;++Seed){FRandomStream R(Seed);if(R.FRand()<Chance){B->Random.Initialize(Seed);Seeded=true;break;}}
                const int32 Lost=B->CombatRecordV65[1].InfantryLost;
                B->Elapsed=Strike.ImpactAt+.001f;B->TickCommittedSupportV8217();
                Check(TEXT("v8217_actual_ground_at_impact_can_kill_infantry_and_record_it"),Victim&&Seeded&&Chance>0&&!Victim->IsAlive()&&B->CombatRecordV65[1].InfantryLost==Lost+1);
            }
            Gun->Destroy();
        }
        Clear();B->Elapsed=1000;
        auto* OutcomeGun=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
        const bool OutcomeReady=OutcomeGun&&OutcomeGun->Configure(B,EFrontlineSupport::AntiTank,Target,0);
        B->ArtilleryReadyAt[0]=0;
        const bool Ordered=B->RequestSupport(EFrontlineSupport::Artillery,Ground(Middle,0),0);
        const int32 Committed=B->Strikes.Num();B->FinishBattle(0,TEXT("V82.17 isolated committed-fire fixture"));
        const float EndClock=B->Elapsed,EndMorale0=B->Morale[0],EndMorale1=B->Morale[1];
        if(OutcomeReady)
        {
            B->Tick(OutcomeGun->Arrival+.01f);OutcomeGun->Tick(.01f);
            Check(TEXT("v8217_at_mission_waiting_at_victory_still_fires_selected_ground"),OutcomeGun->RoundsFired==1&&B->Elapsed==EndClock&&B->Phase==EFrontlinePhase::Victory);
        }
        Check(TEXT("v8217_finish_preserves_all_called_artillery_rounds"),Ordered&&Committed>0&&B->Strikes.Num()>=Committed);
        float LastImpact=EndClock;for(const auto& S:B->Strikes)LastImpact=FMath::Max(LastImpact,S.ImpactAt);
        AFrontlineOrdnanceV36* Arriving=nullptr;
        const auto CommittedCopy=B->Strikes;
        for(const auto& S:CommittedCopy)if(S.SourceV65.Support==int32(EFrontlineSupport::Artillery))
        {
            B->BattleOutroElapsed=FMath::Max(0.f,S.ImpactAt-EndClock-.15f);
            B->TickCommittedSupportV8217();
            for(const auto& Live:B->Strikes)if(Live.ImpactAt==S.ImpactAt&&Live.RoundIndex==S.RoundIndex&&Live.FireMissionId==S.FireMissionId)
                Arriving=Cast<AFrontlineOrdnanceV36>(Live.Projectile.Get());
            if(Arriving){Arriving->Tick(.01f);break;}
        }
        Check(TEXT("v8217_called_artillery_terminal_actor_remains_alive_in_outcome"),IsValid(Arriving)&&!Arriving->IsActorBeingDestroyed());
        B->Tick(FMath::Max(0.f,LastImpact-B->CommittedSupportTimeV8217())+.01f);
        bool Resolved=true;for(const auto& S:B->Strikes)Resolved&=S.bResolved;
        Check(TEXT("v8217_called_salvo_resolves_after_victory_without_clock_winner_morale_change"),Resolved&&B->Elapsed==EndClock&&B->BattleOutroWinner==0&&B->Morale[0]==EndMorale0&&B->Morale[1]==EndMorale1);
        if(IsValid(OutcomeGun))OutcomeGun->Destroy();
        // A genuine support source, ordinary damage path and active entered man.
        auto* Late=MakeMan(1,Target);const int32 BeforeLate=B->CombatRecordV65[0].Total.Infantry;
        if(Late)Late->ReceiveHit(1,0,0,FVector(1,0,0),EFrontlineDamageKind::Artillery,.5f,FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::Artillery)));
        Check(TEXT("v8217_outcome_support_loss_records_actual_kill_without_morale_change"),Late&&!Late->IsAlive()&&B->CombatRecordV65[0].Total.Infantry==BeforeLate+1&&B->Morale[1]==EndMorale1);
        auto* Ceasefire=MakeMan(1,Target+FVector(0,500,0));if(Ceasefire)Ceasefire->ReceiveHit(1,0,0,FVector(1,0,0));
        Check(TEXT("v8217_ordinary_rifle_still_cannot_restart_outcome_combat"),Ceasefire&&Ceasefire->IsAlive());
        auto* Gas=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
        const bool GasReady=Gas&&Gas->Configure(B,EFrontlineSupport::Gas,Target,0);
        if(GasReady)
        {
            B->BattleOutroElapsed=Gas->Arrival+.02f;Gas->Tick(.02f);
            const bool Delivered=Gas->RoundsFired>0&&B->SmokeZones.ContainsByPredicate([](const auto& Z){return Z.bGas;});
            FrontlineMotionV40::TickExposure(Ceasefire,.1f);
            Check(TEXT("v8217_called_gas_delivers_real_exposure_zone_during_outcome"),Delivered&&FrontlineMotionV40::GasExposureV8215(Ceasefire).Concentration>0);
            B->Phase=EFrontlinePhase::Briefing;const int32 Zones=B->SmokeZones.Num();Gas->Tick(.1f);
            Check(TEXT("v8217_support_cannot_follow_into_war_room"),Gas->IsActorBeingDestroyed()&&B->SmokeZones.Num()==Zones);
        }
        else Check(TEXT("v8217_called_gas_delivers_real_exposure_zone_during_outcome"),false);
        Clear();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;B->Elapsed=1000;
        for(int32 Reversed=0;Reversed<2;++Reversed)
        {
            Clear();auto* A=Fleet->Spawn(EFrontlineVehicle::A7V,0,true);auto* M=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,1,true);
            if(!A||!M){Check(FString::Printf(TEXT("v8217_duel%d_assets"),Reversed),false);continue;}
            const float Heading=B->GetTeamDirection(0)>0?0.f:180.f;
            const auto Place=[&](AFrontlineTank* V,float X,float Y){V->HeadingYawV37=Heading+(V->Team?180.f:0.f)+(Reversed?65.f:0.f);V->PreferredLaneV37=V->DesiredLaneV37=Y;V->bEntered=true;V->SetActorLocation(Ground(X,Y));V->FitGroundV35(V->GetActorLocation(),0);V->SetActorTickEnabled(false);};
            Place(A,Middle-Dir*1600,-420);Place(M,Middle+Dir*1600,420);
            bool ClearHulls=true,Crossed=false;const double StartSide=(M->GetActorLocation().X-A->GetActorLocation().X)*Dir;
            for(int32 I=0;I<100;++I)
            {
                A->Simulate(.1f);M->Simulate(.1f);
                if(A->IsOperational()&&M->IsOperational())ClearHulls&=A->CanOccupyV48(A->GetActorLocation(),A->HeadingYawV37)&&M->CanOccupyV48(M->GetActorLocation(),M->HeadingYawV37);
                if(A->CannonShots==0&&M->CannonShots==0&&(M->GetActorLocation().X-A->GetActorLocation().X)*Dir*StartSide<0)Crossed=true;
            }
            Check(FString::Printf(TEXT("v8217_duel%d_actual_mounts_engage_without_passing_or_overlap"),Reversed),A->CannonShots>0&&M->CannonShots>0&&!Crossed&&ClearHulls);
            auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("case"),Reversed);J->SetNumberField(TEXT("a7v_cannon_shots"),A->CannonShots);J->SetNumberField(TEXT("mark_iv_cannon_shots"),M->CannonShots);J->SetBoolField(TEXT("crossed_before_fire"),Crossed);J->SetBoolField(TEXT("clear_hulls"),ClearHulls);DuelRows.Add(MakeShared<FJsonValueObject>(J));
        }
        Diagnostic->SetArrayField(TEXT("actual_recruit_model_seconds"),RecruitRows);Diagnostic->SetArrayField(TEXT("actual_duels"),DuelRows);
        FString Json;FJsonSerializer::Serialize(Diagnostic,TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V8217/combat-diagnostics.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
};
static void RunFrontlineCombatChecksV8217(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineCombatV8217Review::Run(B,Check);}
static TArray<AFrontlineTank*> SetupFrontlineBeuteArtFixtureV8217(AWarfareFrontlineGameMode* B)
{return FFrontlineCombatV8217Review::ArtFixture(B);}
