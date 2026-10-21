#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineSupportV32.h"
#include "WarfareFrontlineSupportV28.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"

struct FFrontlineCombatV8218Review
{
    struct FArtState
    {
        TArray<TWeakObjectPtr<AFrontlineTank>> Vehicles;
        int32 Serial=0,Deployment0=0,Deployment1=0;
    };
    static TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FArtState>& ArtStates()
    {static TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FArtState> States;return States;}
    static void ClearArtFixture(AWarfareFrontlineGameMode* B)
    {
        if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV8218Review")))return;
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());auto* State=ArtStates().Find(B);if(!State)return;
        for(const auto& W:State->Vehicles)if(auto* V=W.Get()){if(Fleet)Fleet->Vehicles.Remove(V);V->Destroy();}
        if(Fleet){Fleet->NextSerial=State->Serial;Fleet->DeploymentCountV37[0]=State->Deployment0;Fleet->DeploymentCountV37[1]=State->Deployment1;}
        ArtStates().Remove(B);
    }
    static TArray<AFrontlineTank*> ArtFixture(AWarfareFrontlineGameMode* B,bool UK)
    {
        TArray<AFrontlineTank*> Result;
        auto* Fleet=B?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;
        if(!Fleet||!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV8218Review")))return Result;
        ClearArtFixture(B);auto& State=ArtStates().FindOrAdd(B);State.Serial=Fleet->NextSerial;
        State.Deployment0=Fleet->DeploymentCountV37[0];State.Deployment1=Fleet->DeploymentCountV37[1];
        const int32 Team=(B->GetFactionForTeam(0)==EFrontlineFaction::British)==UK?0:1;
        const EFrontlineVehicle Types[2]={UK?EFrontlineVehicle::Austin1918:EFrontlineVehicle::BeuteMarkIV,UK?EFrontlineVehicle::MarkVStar:EFrontlineVehicle::A7V};
        const float Centre=FrontlineBattlefield::TrenchCount>0?(FrontlineBattlefield::TrenchX(0)+FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1))*.5f:0.f;
        const float Home=FrontlineBattlefield::TrenchCount>0?FMath::Max(FMath::Abs(FrontlineBattlefield::TrenchX(0)),FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1))):1900.f;
        TArray<FVector> Positions;auto Diagnostic=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
        Diagnostic->SetBoolField(TEXT("uk"),UK);Diagnostic->SetNumberField(TEXT("actual_ground_trenches"),FrontlineBattlefield::TrenchCount);
        ON_SCOPE_EXIT
        {
            Diagnostic->SetArrayField(TEXT("vehicles"),Rows);Diagnostic->SetNumberField(TEXT("actual_pair_count"),Result.Num());
            FString JSON;FJsonSerializer::Serialize(Diagnostic,TJsonWriterFactory<>::Create(&JSON));
            const FString Path=FPaths::ProjectSavedDir()/TEXT("Gameplay/V8218")/(UK?TEXT("armour-art-UK.json"):TEXT("armour-art-DE.json"));
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);FFileHelper::SaveStringToFile(JSON,*Path);
        };
        for(int32 I=0;I<2;++I)
        {
            auto Row=MakeShared<FJsonObject>();Rows.Add(MakeShared<FJsonValueObject>(Row));Row->SetNumberField(TEXT("type"),int32(Types[I]));
            AFrontlineTank* V=Fleet->Spawn(Types[I],Team,true);Row->SetBoolField(TEXT("spawned"),V!=nullptr);
            if(!V)continue;State.Vehicles.Add(V);V->SetActorTickEnabled(false);
            UStaticMesh* Mesh=V->Body?V->Body->GetStaticMesh():nullptr;
            Row->SetStringField(TEXT("actual_mesh"),Mesh?Mesh->GetName():TEXT("missing"));Row->SetBoolField(TEXT("assets_ready"),V->AreAssetsReady());
            if(!Mesh||!V->AreAssetsReady()){Fleet->Vehicles.Remove(V);V->Destroy();continue;}
            // Sample the genuine current ground across this imported vehicle's
            // full footprint, before any campaign/UI seed changes the profile.
            const float HalfX=V->FootprintHalfLengthV37()+30.f;
            const float HalfY=FMath::Max(125.f,float(Mesh->GetBoundingBox().GetExtent().Y)+30.f);
            // The larger second hull seeks its own rear-road plane. A small
            // first car's chosen plane does not guarantee a full tank fits
            // 14 metres farther towards the cratered battle centre.
            const FVector Desired=Positions.IsEmpty()?FVector(Centre-Home-2600.f,-450.f,0):Positions[0]+FVector(-1400.f,0,-Positions[0].Z);
            bool Found=false;FVector Best=FVector::ZeroVector;float BestScore=MAX_flt,BestResidual=0,BestPitch=0,BestRoll=0;
            int32 Candidates=0,RejectedTrench=0,RejectedWater=0,RejectedShape=0,RejectedPair=0;
            for(int32 IX=-20;IX<=20;++IX)for(int32 IY=-7;IY<=7;++IY)
            {
                const FVector P(Desired.X+IX*180.f,Desired.Y+IY*150.f,0);
                if(FMath::Abs(P.X)+HalfX>=FrontlineBattlefield::GroundHalfLength||FMath::Abs(P.Y)>FrontlineBattlefield::HalfFrontage-50.f)continue;++Candidates;
                if(!Positions.IsEmpty()&&(FVector::Dist2D(P,Positions[0])<1250.f||FVector::Dist2D(P,Positions[0])>3600.f||FMath::Abs(P.Y-Positions[0].Y)>900.f)){++RejectedPair;continue;}
                float Heights[5][3]{},Mean=0,SlopeX=0,SlopeY=0;bool Clear=true;int32 Reason=0;
                const float StepX=HalfX*.5f,StepY=HalfY;
                for(int32 SX=-2;SX<=2&&Clear;++SX)for(int32 SY=-1;SY<=1&&Clear;++SY)
                {
                    const float X=P.X+SX*StepX,Y=P.Y+SY*StepY;
                    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)if(FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(T,Y))<420.f){Clear=false;Reason=1;break;}
                    if(!Clear)break;float Level=0,Depth=0;
                    if(AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth)&&Depth>3.f){Clear=false;Reason=2;break;}
                    const float H=AWarfareFrontlineGround::HeightAt(X,Y);if(!FMath::IsFinite(H)){Clear=false;Reason=3;break;}
                    Heights[SX+2][SY+1]=H;Mean+=H/15.f;SlopeX+=SX*StepX*H/(30.f*StepX*StepX);SlopeY+=SY*StepY*H/(10.f*StepY*StepY);
                }
                if(!Clear){if(Reason==1)++RejectedTrench;else if(Reason==2)++RejectedWater;else ++RejectedShape;continue;}
                float Residual=0;for(int32 SX=-2;SX<=2;++SX)for(int32 SY=-1;SY<=1;++SY)
                    Residual=FMath::Max(Residual,FMath::Abs(Heights[SX+2][SY+1]-(Mean+SlopeX*SX*StepX+SlopeY*SY*StepY)));
                const float Pitch=FMath::Abs(FMath::RadiansToDegrees(FMath::Atan(SlopeX))),Roll=FMath::Abs(FMath::RadiansToDegrees(FMath::Atan(SlopeY)));
                if(Residual>18.f||Pitch>8.f||Roll>6.f){++RejectedShape;continue;}
                const float Score=Residual*15.f+Pitch*3.f+Roll*3.f+FVector::Dist2D(P,Desired)*.04f;
                if(Score<BestScore){Found=true;BestScore=Score;Best=P;BestResidual=Residual;BestPitch=Pitch;BestRoll=Roll;}
            }
            Row->SetBoolField(TEXT("found"),Found);Row->SetNumberField(TEXT("candidates"),Candidates);Row->SetNumberField(TEXT("trench_rejected"),RejectedTrench);
            Row->SetNumberField(TEXT("water_rejected"),RejectedWater);Row->SetNumberField(TEXT("shape_rejected"),RejectedShape);Row->SetNumberField(TEXT("pair_rejected"),RejectedPair);
            Row->SetNumberField(TEXT("half_x_cm"),HalfX);Row->SetNumberField(TEXT("half_y_cm"),HalfY);
            Row->SetNumberField(TEXT("plane_residual_cm"),BestResidual);Row->SetNumberField(TEXT("plane_pitch_deg"),BestPitch);Row->SetNumberField(TEXT("plane_roll_deg"),BestRoll);
            if(!Found){Fleet->Vehicles.Remove(V);V->Destroy();continue;}
            Best.Z=AWarfareFrontlineGround::HeightAt(Best.X,Best.Y);Positions.Add(Best);
            V->HeadingYawV37=I?180.f:0.f;V->PreferredLaneV37=V->DesiredLaneV37=Best.Y;V->bEntered=true;V->SetActorLocation(Best);V->FitGroundV35(Best,0);
            Row->SetStringField(TEXT("actual_position"),V->GetActorLocation().ToString());Row->SetStringField(TEXT("actual_rotation"),V->GetActorRotation().ToString());
            Result.Add(V);
        }
        return Result;
    }
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV8218Review")))
        {Check(TEXT("v8218_combat_requires_disposable_profile"),false);return;}
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Fleet){Check(TEXT("v8218_combat_actual_fleet"),false);return;}
        const auto Units=B->Units;const auto SavedVehicles=Fleet->Vehicles;const auto Squads=B->Squads;
        const auto Phase=B->Phase;const auto Runs=B->CampaignRuns;const auto Strikes=B->Strikes;const auto Smoke=B->SmokeZones;const auto Tracers=B->Tracers;
        const FString Message=B->LastMessage;
        const float Clock=B->Elapsed,Enemy=B->EnemyThinkAt,Extended=B->ExtendedSupportThinkAt;
        const int32 Winner=B->BattleOutroWinner,BoundMission=B->BoundCampaignMission,Shots=B->ShotsFired,NextSquad=B->NextSquadId;
        const auto Records0=B->CombatRecordV65[0],Records1=B->CombatRecordV65[1];const auto Random=B->Random;
        const int32 Casualties[2]={B->Casualties[0],B->Casualties[1]};
        const float Morale[2]={B->Morale[0],B->Morale[1]},Damage[2]={B->DamageDealt[0],B->DamageDealt[1]},Friendly=B->FriendlyFireDamage,Peak=B->PeakSuppression;
        const float FleetEnemy=Fleet->NextEnemyAt;const int32 Serial=Fleet->NextSerial;
        TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
        struct FVisibility {AActor* Actor;bool Hidden;};TArray<FVisibility> Visibility;
        for(AWarfareFrontlineSoldier* U:Units)if(IsValid(U)){Visibility.Add({U,U->IsHidden()});U->SetActorHiddenInGame(true);}
        for(AFrontlineTank* V:SavedVehicles)if(IsValid(V)){Visibility.Add({V,V->IsHidden()});V->SetActorHiddenInGame(true);}
        TArray<AActor*> Owned;
        const auto Clear=[&]()
        {
            for(AActor* A:Owned)if(IsValid(A))A->Destroy();Owned.Reset();B->Units.Reset();B->Squads.Reset();Fleet->Vehicles.Reset();
            FrontlineMobilityV41::Reset(B);
        };
        ON_SCOPE_EXIT
        {
            Clear();FrontlineSupportV32::Reset(B);
            for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
            for(const auto& V:Visibility)if(IsValid(V.Actor))V.Actor->SetActorHiddenInGame(V.Hidden);
            B->Units=Units;B->Squads=Squads;Fleet->Vehicles=SavedVehicles;B->Phase=Phase;B->CampaignRuns=Runs;
            B->Elapsed=Clock;B->EnemyThinkAt=Enemy;B->ExtendedSupportThinkAt=Extended;B->BattleOutroWinner=Winner;
            B->BoundCampaignMission=BoundMission;B->ShotsFired=Shots;B->NextSquadId=NextSquad;
            B->Strikes=Strikes;B->SmokeZones=Smoke;B->Tracers=Tracers;B->Random=Random;
            B->CombatRecordV65[0]=Records0;B->CombatRecordV65[1]=Records1;B->FriendlyFireDamage=Friendly;B->PeakSuppression=Peak;
            B->LastMessage=Message;
            Fleet->NextEnemyAt=FleetEnemy;Fleet->NextSerial=Serial;
            for(int32 T=0;T<2;++T){B->Casualties[T]=Casualties[T];B->Morale[T]=Morale[T];B->DamageDealt[T]=Damage[T];}
            FrontlineMobilityV41::Reset(B);
        };
        B->Units.Reset();B->Squads.Reset();Fleet->Vehicles.Reset();B->Strikes.Reset();B->SmokeZones.Reset();
        B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->EnemyThinkAt=B->ExtendedSupportThinkAt=MAX_flt;
        Fleet->NextEnemyAt=MAX_flt;B->Elapsed=1000;
        auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Movement,Trench;
        ON_SCOPE_EXIT
        {
            Report->SetArrayField(TEXT("movement"),Movement);Report->SetArrayField(TEXT("trench"),Trench);
            FString JSON;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&JSON));
            const FString Path=FPaths::ProjectSavedDir()/TEXT("Gameplay/V8218/combat-diagnostics.json");
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);FFileHelper::SaveStringToFile(JSON,*Path);
        };
        const auto Ground=[&](float X,float Y){return FVector(X,Y,FrontlineInfantryV45::SurfaceHeight(B,FVector(X,Y,0)));};
        const auto MakeMan=[&](int32 Team,int32 Id,int32 Member,const FVector& P)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            U->Initialize(B,Id,Team,EFrontlineRole::Rifle,Member);Owned.Add(U);B->Units.Add(U);U->SetActorTickEnabled(false);
            U->SetActorLocation(P+FVector(0,0,88));U->bFieldEntered=true;U->ShotClock=U->GrenadeClock=10000;
            U->ReactionSeconds=U->DepartureClock=U->AimSettleClock=0;return U;
        };
        const auto Refresh=[&](AWarfareFrontlineSoldier* U)
        {if(U&&U->Soldier){U->Soldier->TickAnimation(.05f,false);U->Soldier->RefreshBoneTransforms();}};
        const auto AddSquad=[&](int32 Team,int32 TrenchIndex,bool Advancing)
        {
            FFrontlineSquad Q;Q.Id=++B->NextSquadId;Q.Team=Team;Q.Role=EFrontlineRole::Rifle;
            Q.Trench=Q.Destination=TrenchIndex;Q.Slot=1;Q.bAdvancing=Advancing;B->Squads.Add(Q);return Q.Id;
        };
        const auto PlaceTank=[&](AFrontlineTank* V,const FVector& P,float Yaw)
        {
            V->SetActorTickEnabled(false);V->SetActorLocation(P);V->HeadingYawV37=Yaw;V->PreferredLaneV37=V->DesiredLaneV37=P.Y;
            V->bEntered=true;V->FitGroundV35(P,0);for(float& C:V->GunClock)C=10000;V->CannonClock[0]=V->CannonClock[1]=10000;
        };
        for(int32 Team=0;Team<2;++Team)for(bool Mouth:{false,true})
        {
            Clear();const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team);
            if(!B->Trenches.IsValidIndex(Home)){Check(TEXT("v8218_movement_has_real_home_trench"),false);continue;}
            const float X=B->Trenches[Home].X-Dir*(Mouth?720.f:1900.f);
            auto* Tank=Fleet->Spawn(EFrontlineVehicle::Whippet,Team,false);if(Tank)Owned.Add(Tank);
            const int32 Id=AddSquad(Team,Home,true);auto* U=MakeMan(Team,Id,0,Ground(X-Dir*600.f,0));
            if(!Tank||!U){Check(TEXT("v8218_movement_actual_assets"),false);continue;}
            B->FindSquad(Id)->Members.Add(U);PlaceTank(Tank,Ground(X,0),Dir>0?0.f:180.f);
            const FVector StartU=U->Feet(),StartT=Tank->GetActorLocation();bool ClearBodies=true,RealStride=true;int32 Contacts=0;
            for(int32 I=0;I<140;++I)
            {
                B->Elapsed+=.05f;const FVector Before=U->Feet();U->Tick(.05f);Refresh(U);Tank->Simulate(.05f);
                const bool Overlap=Tank->BlocksInfantryAtV8218(U->Feet(),Tank->GetActorLocation(),Tank->HeadingYawV37,24.f);
                ClearBodies&=!Overlap;
                if(FVector::Dist2D(U->Feet(),Tank->GetActorLocation())<950.f)++Contacts;
                RealStride&=!U->bMoving||FVector::Dist2D(Before,U->Feet())>.01;
            }
            auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("team"),Team);J->SetBoolField(TEXT("trench_mouth"),Mouth);
            J->SetNumberField(TEXT("near_contact_frames"),Contacts);J->SetNumberField(TEXT("soldier_travel_cm"),FVector::Dist2D(StartU,U->Feet()));
            J->SetNumberField(TEXT("tank_travel_cm"),FVector::Dist2D(StartT,Tank->GetActorLocation()));Movement.Add(MakeShared<FJsonValueObject>(J));
            const FString Prefix=FString::Printf(TEXT("v8218_team%d_%s"),Team,Mouth?TEXT("real_trench_approach"):TEXT("rear_field"));
            Check(Prefix+TEXT("_seven_seconds_reciprocal_contact_without_hull_overlap"),ClearBodies&&Contacts>10);
            Check(Prefix+TEXT("_both_progress_and_stride_requires_real_translation"),RealStride&&FVector::Dist2D(StartU,U->Feet())>250.f&&FVector::Dist2D(StartT,Tank->GetActorLocation())>100.f);
            const FVector Inside=Tank->GetActorTransform().TransformPosition(FVector(0,0,50.f));
            const bool PredictionBlocks=Tank->BlocksInfantryAtV8218(Inside,Tank->GetActorLocation(),Tank->HeadingYawV37);
            const float SweepInto=Tank->InfantrySweepClearanceV8218(Inside+FVector(-900,0,0),Inside);
            const float StationaryInside=Tank->InfantrySweepClearanceV8218(Inside,Inside);
            J->SetBoolField(TEXT("current_pose_inside_blocked"),StationaryInside==0.f);
            J->SetBoolField(TEXT("predicted_pose_same_inside_blocked"),PredictionBlocks);
            J->SetNumberField(TEXT("sweep_into_same_inside_fraction"),SweepInto);
            J->SetStringField(TEXT("actual_body_world_pose"),Tank->Body->GetComponentTransform().ToString());
            J->SetStringField(TEXT("predicted_actor_world_pose"),Tank->GroundPoseForContactV8218(Tank->GetActorLocation(),Tank->HeadingYawV37).ToString());
            J->SetStringField(TEXT("body_actor_local_pose"),Tank->Body->GetComponentTransform().GetRelativeTransform(Tank->GetActorTransform()).ToString());
            Check(Prefix+TEXT("_same_envelope_blocks_entering_body_from_both_interfaces"),PredictionBlocks&&StationaryInside==0.f&&SweepInto<1.f);
        }
        for(int32 Team=0;Team<2;++Team)
        {
            Clear();const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team);
            const int32 Id=AddSquad(Team,Home,false);TArray<AWarfareFrontlineSoldier*> Shooters;
            for(int32 I=0;I<3;++I)
            {auto* U=MakeMan(Team,Id,I,Ground(B->Trenches[Home].X,-80+I*80));if(U){B->FindSquad(Id)->Members.Add(U);Shooters.Add(U);}}
            for(auto* U:Shooters){U->SetActorLocation(B->SoldierDestination(U)+FVector(0,0,88));Refresh(U);}
            for(int32 I=0;I<Shooters.Num();++I)
            {
                auto* U=Shooters[I];const FVector P=U->Feet();AWarfareFrontlineSoldier* EnemyU=nullptr;
                // Search only genuine exposed targets. No false HasSight or
                // raised firing origins stand in for the prepared earthwork.
                for(float Gap:{650.f,950.f,1250.f})
                {
                    EnemyU=MakeMan(1-Team,++B->NextSquadId,0,Ground(P.X+Dir*Gap,P.Y));Refresh(EnemyU);
                    if(EnemyU&&B->HasSight(U,EnemyU))break;
                    if(EnemyU){B->Units.Remove(EnemyU);EnemyU->Destroy();EnemyU=nullptr;}
                }
                const int32 Before=U->ShotsDischarged;U->ShotClock=0;
                for(int32 F=0;F<100&&U->ShotsDischarged==Before&&EnemyU&&EnemyU->IsAlive();++F)
                {B->Elapsed+=.05f;U->Tick(.05f);Refresh(U);}
                const bool Fired=EnemyU&&U->ShotsDischarged>Before;
                Check(FString::Printf(TEXT("v8218_team%d_trench_rifle_member%d_actual_tick_fires_at_real_visible_enemy"),Team,I),Fired);
                auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("team"),Team);J->SetNumberField(TEXT("member"),I);
                J->SetStringField(TEXT("feet"),U->Feet().ToString());J->SetBoolField(TEXT("real_sight"),EnemyU&&B->HasSight(U,EnemyU));
                J->SetNumberField(TEXT("shots"),U->ShotsDischarged-Before);J->SetNumberField(TEXT("bank_intrusion_cm"),FrontlineInfantryV45::BankIntrusion(U,U->Feet()));
                Trench.Add(MakeShared<FJsonValueObject>(J));U->ShotClock=10000;
                if(EnemyU){B->Units.Remove(EnemyU);EnemyU->Destroy();}
            }
            if(!Shooters.IsEmpty())
            {
                auto* U=Shooters[0];const float Range=FrontlineWeapons::ProfileFor(U).FireRangeCm*B->EncounterVisibilityScale();
                auto* Far=MakeMan(1-Team,++B->NextSquadId,0,Ground(U->Feet().X+Dir*(Range+50.f),U->Feet().Y));Refresh(Far);
                Check(FString::Printf(TEXT("v8218_team%d_real_distance_still_rejects_out_of_range_target"),Team),Far&&B->AcquireTarget(U,false)==nullptr);
            }
        }
        Clear();auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();
        auto* PC=UGameplayStatics::GetPlayerController(B->GetWorld(),0);
        if(Audio&&PC&&PC->PlayerCameraManager)
        {
            const auto* Camera=PC->PlayerCameraManager.Get();const FVector C=Camera->GetCameraLocation(),F=Camera->GetCameraRotation().Vector(),Right=Camera->GetCameraRotation().RotateVector(FVector::RightVector);
            const FVector Focus=F.Z<-.02?C+F*(-C.Z/F.Z):C;
            const FVector Near=Focus+FVector(0,0,1550);const int32 AudioId=8218991;
            Audio->SetBattleActive(true);Audio->UpdateAircraftAudio(AudioId,Near,1,true,Right*1600);
            const auto NearJ=Audio->DescribeAircraftAudioV8218(AudioId);
            Audio->UpdateAircraftAudio(AudioId,Near-Right*7000,1,true,Right*1600);const auto LeftJ=Audio->DescribeAircraftAudioV8218(AudioId);
            Audio->UpdateAircraftAudio(AudioId,Near+Right*7000,1,true,Right*1600);const auto RightJ=Audio->DescribeAircraftAudioV8218(AudioId);
            Check(TEXT("v8218_actual_plane_loop_near_audible_distant_attenuates"),NearJ->GetBoolField(TEXT("playing"))&&NearJ->GetNumberField(TEXT("gain"))>RightJ->GetNumberField(TEXT("gain"))*2);
            Check(TEXT("v8218_actual_flyby_pans_and_recedes_with_physical_velocity"),LeftJ->GetNumberField(TEXT("pan"))<-.4&&RightJ->GetNumberField(TEXT("pan"))>.4&&LeftJ->GetNumberField(TEXT("pitch"))>RightJ->GetNumberField(TEXT("pitch")));
            Audio->UpdateAircraftAudio(AudioId,Near,0,false);Check(TEXT("v8218_aircraft_stop_releases_actual_decoder"),!Audio->DescribeAircraftAudioV8218(AudioId)->GetBoolField(TEXT("playing")));
            if(auto* Run=B->CampaignRuns.FindByPredicate([&](const FFrontlineCampaignProgress& R){return R.Id==B->BoundCampaignId;}))
            {
                // Select a real authorised operation. The last two resident-
                // area missions correctly forbid bombing, even in a fixture.
                int32 Mission=INDEX_NONE;const auto& Missions=B->GetCampaignMissionsForFaction(Run->Faction);
                for(int32 I=Missions.Num()-1;I>=0;--I)if(!(Missions[I].DisabledSupportMask&(1u<<uint8(EFrontlineSupport::AirBombing))))
                {Mission=I;break;}
                if(Mission!=INDEX_NONE){Run->SelectedMission=Run->CompletedMissions=Mission;Run->SupportSelectionMask=1u<<uint8(EFrontlineSupport::AirBombing);B->BoundCampaignMission=Mission;}
                Report->SetNumberField(TEXT("bombing_authorised_actual_mission"),Mission);
            }
            FrontlineSupportV32::Reset(B);FrontlineSupportV32::ResetReadinessForDebug(B);
            TSet<AActor*> BeforeAir;for(TActorIterator<AActor> It(B->GetWorld());It;++It)BeforeAir.Add(*It);
            const FVector BombTarget=Ground(FMath::Clamp(float(Focus.X),-FrontlineBattlefield::SupportHalfLength+200.f,FrontlineBattlefield::SupportHalfLength-200.f),
                FMath::Clamp(float(Focus.Y),-FrontlineBattlefield::HalfFrontage+200.f,FrontlineBattlefield::HalfFrontage-200.f));
            const FString BombGate=B->SupportBlockReason(EFrontlineSupport::AirBombing);
            Report->SetStringField(TEXT("bombing_actual_selected_ground_target"),BombTarget.ToString());
            Report->SetStringField(TEXT("bombing_gate_before_request"),BombGate);
            const bool Called=FrontlineSupportV32::Request(B,EFrontlineSupport::AirBombing,BombTarget,0);
            AActor* Plane=nullptr;
            if(Called)for(TActorIterator<AActor> It(B->GetWorld());It;++It)
                if(!BeforeAir.Contains(*It)&&!It->IsActorBeingDestroyed()&&Cast<UStaticMeshComponent>(It->GetRootComponent())){Plane=*It;break;}
            bool Heard=false;
            for(int32 I=0;I<90&&Called&&!Heard;++I)
            {B->Elapsed+=.5f;FrontlineSupportV32::Tick(B,.5f);if(Plane)Heard=Audio->DescribeAircraftAudioV8218(int32(Plane->GetUniqueID()))->GetBoolField(TEXT("playing"));}
            Check(TEXT("v8218_real_qualified_bombing_request_tick_has_actual_engine_audio"),Called&&Plane&&Heard);
            Report->SetBoolField(TEXT("bombing_requested"),Called);Report->SetStringField(TEXT("bombing_block_reason"),Called?TEXT(""):B->SupportBlockReason(EFrontlineSupport::AirBombing));
            Report->SetObjectField(TEXT("near_aircraft"),NearJ);Report->SetObjectField(TEXT("far_aircraft"),RightJ);
            FrontlineSupportV32::Reset(B);
            Check(TEXT("v8218_bombing_reset_stops_its_actual_aircraft_audio"),!Plane||!Audio->DescribeAircraftAudioV8218(int32(Plane->GetUniqueID()))->GetBoolField(TEXT("playing")));
        }
        else Check(TEXT("v8218_flyby_requires_prepared_actual_listener"),false);
    }
};

static void RunFrontlineCombatChecksV8218(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineCombatV8218Review::Run(B,Check);}
