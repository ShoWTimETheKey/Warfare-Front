#include "WarfareFrontlineCombatV64.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineBalanceV50.h"
#include "WarfareFrontlineSupportV32.h"
#include "WarfareFrontlineSupportArrivalV30.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineGameplayV49.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMovementV28.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "Dom/JsonObject.h"
#include "Misc/Paths.h"

struct FFrontlineCombatV64Review
{
 #include "WarfareFrontlineAntiArmourReviewV833.inl"
 #include "WarfareFrontlineNo44RotationReviewV8310.inl"
 static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
 {
  // Dedicated newly opened review encounter only; caller abandons immediately.
  // Support clocks/aircraft are reset, not restored as a playable saved battle.
  FString UserDir;
  const bool Safe=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
   &&(FParse::Param(FCommandLine::Get(),TEXT("WFV64Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV828Review")))
   &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
  Check(TEXT("v64_combat_disposable_profile"),Safe);if(!Safe)return;
  auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Fleet){Check(TEXT("v64_vehicle_service"),false);return;}
  const auto OldTrenches=B->Trenches;const int32 OldShots=B->ShotsFired;
  const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldStrikes=B->Strikes;const auto OldSmoke=B->SmokeZones;
  const auto OldVehicles=Fleet->Vehicles;const auto OldPhase=B->Phase;const auto OldRandom=B->Random;
  const auto OldRuns=B->CampaignRuns;const auto OldFaction=B->PlayerFaction;const auto OldId=B->ActiveCampaignId;
  const auto OldBoundId=B->BoundCampaignId,OldBoundEncounter=B->BoundCampaignEncounterId;
  const float OldClock=B->Elapsed;const int32 OldOutro=B->BattleOutroWinner,OldSerial=Fleet->NextSerial;
  const FString OldMessage=B->LastMessage;const float OldPressure=B->PeakSuppression,OldFriendly=B->FriendlyFireDamage;
  const float OldMorale[]={B->Morale[0],B->Morale[1]},OldDamage[]={B->DamageDealt[0],B->DamageDealt[1]};
  const int32 OldDead[]={B->Casualties[0],B->Casualties[1]};
  struct FSavedActor{AActor* A;bool Hidden,Collision;};TArray<FSavedActor> SavedActors;
  auto Hide=[&](AActor* A){if(IsValid(A)){SavedActors.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
  for(AWarfareFrontlineSoldier* U:OldUnits)Hide(U);for(AFrontlineTank* T:OldVehicles)Hide(T);
  TSet<AActor*> Existing;for(TActorIterator<AActor> I(B->GetWorld());I;++I)Existing.Add(*I);
  B->Units.Empty();B->Squads.Empty();B->Strikes.Empty();B->SmokeZones.Empty();Fleet->Vehicles.Empty();Fleet->NextSerial=64000;
  B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=1000;
  FFrontlineCampaignProgress Run;Run.Id=FGuid::NewGuid();Run.SelectedMission=Run.CompletedMissions=19;Run.Faction=EFrontlineFaction::British;
  Run.Research.TechnologyMask=(1u<<12);Run.SupportSelectionMask=(1u<<int32(EFrontlineSupport::AirRecon));
  B->CampaignRuns={Run};B->ActiveCampaignId=Run.Id;B->PlayerFaction=Run.Faction;
  B->BoundCampaignId=Run.Id;B->BoundCampaignEncounterId=B->EncounterId;
  ON_SCOPE_EXIT
  {
   for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(!Existing.Contains(*I))I->Destroy();
   for(const auto& S:SavedActors)if(IsValid(S.A)){S.A->SetActorHiddenInGame(S.Hidden);S.A->SetActorEnableCollision(S.Collision);}
   B->Trenches=OldTrenches;B->ShotsFired=OldShots;B->Units=OldUnits;B->Squads=OldSquads;B->Strikes=OldStrikes;B->SmokeZones=OldSmoke;Fleet->Vehicles=OldVehicles;Fleet->NextSerial=OldSerial;
   B->Phase=OldPhase;B->Random=OldRandom;B->Elapsed=OldClock;B->BattleOutroWinner=OldOutro;B->LastMessage=OldMessage;
   B->CampaignRuns=OldRuns;B->PlayerFaction=OldFaction;B->ActiveCampaignId=OldId;
   B->BoundCampaignId=OldBoundId;B->BoundCampaignEncounterId=OldBoundEncounter;
   B->PeakSuppression=OldPressure;B->FriendlyFireDamage=OldFriendly;
   for(int32 T=0;T<2;++T){B->Morale[T]=OldMorale[T];B->DamageDealt[T]=OldDamage[T];B->Casualties[T]=OldDead[T];}
   FrontlineSupportV32::Reset(B);
  };
  auto Make=[&](int32 Team,EFrontlineRole Role,int32 Id,int32 Member,FVector P)
  {
   auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
   U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->Suppression=0;
   U->SetActorLocation(P+FVector(0,0,88));B->Units.Add(U);return U;
  };
  if(FParse::Param(FCommandLine::Get(),TEXT("WFV828Review")))
  {
   const auto SavedRecord0=B->CombatRecordV65[0],SavedRecord1=B->CombatRecordV65[1];
   ON_SCOPE_EXIT{B->CombatRecordV65[0]=SavedRecord0;B->CombatRecordV65[1]=SavedRecord1;};
   const int32 Dir=B->GetTeamDirection(0);
   auto* Gun=Make(0,EFrontlineRole::Lewis,648280,0,FVector(-1000*Dir,0,10000));
   auto* Aim=Make(1,EFrontlineRole::Rifle,648281,0,FVector(1000*Dir,0,10000));
   auto* Rank=Make(1,EFrontlineRole::Rifle,648282,0,FVector(1000*Dir,480,10000));
   Check(TEXT("v828_combat_actual_actor_fixtures"),Gun&&Aim&&Rank);if(!Gun||!Aim||!Rank)return;
   Aim->Suppression=.95f;
   Check(TEXT("v828_mg_allocates_fire_to_unsuppressed_rank"),B->AcquireTarget(Gun)==Rank
    &&FrontlineMachinegunsV30::ShouldRetargetV828(Gun,Rank,Aim));
   Rank->Suppression=.92f;
   Check(TEXT("v828_similar_pressure_does_not_reset_aim_between_targets"),!FrontlineMachinegunsV30::ShouldRetargetV828(Gun,Rank,Aim));
   Rank->Suppression=Aim->Suppression=0;
   int32 MissSeed=1;for(;MissSeed<10000;++MissSeed){FRandomStream R(MissSeed);if(R.FRand()>.999f)break;}
   const float RankHealth=Rank->Health;
   for(int32 I=0;I<8;++I){B->Random.Initialize(MissSeed);B->Fire(Gun,Aim);}
   Check(TEXT("v828_real_mg_burst_suppresses_wider_rank_without_splash_damage"),Rank->Suppression>.85f&&Rank->Health==RankHealth);
   const float FreeSpeed=FrontlineMovementV28::TravelSpeed(2000,0,true);
   const float PinnedSpeed=FrontlineMovementV28::TravelSpeed(2000,Rank->Suppression,true);
   Check(TEXT("v8215_sustained_pressure_retains_ordinary_translation_speed"),FreeSpeed>380.f&&FMath::IsNearlyEqual(PinnedSpeed,FreeSpeed,.001f));
   auto* Wall=B->GetWorld()->SpawnActor<AActor>();auto* Block=NewObject<UBoxComponent>(Wall);
   Wall->SetRootComponent(Block);Wall->AddInstanceComponent(Block);Block->SetBoxExtent(FVector(30,1100,500));
   Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Block->SetCollisionResponseToAllChannels(ECR_Ignore);
   Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Block->RegisterComponent();Wall->SetActorLocation(FVector(0,0,10100));
    Rank->Suppression=0;const int32 WallShots=B->ShotsFired;const float AimPressure=Aim->Suppression,AimHealth=Aim->Health;
    B->Random.Initialize(MissSeed);B->Fire(Gun,Aim);
    Check(TEXT("v828_area_pressure_does_not_cross_a_physical_wall"),Rank->Suppression==0);
    Check(TEXT("v8213_direct_fire_rechecks_occluded_contact_before_combat_events"),!B->HasSight(Gun,Aim)
     &&B->ShotsFired==WallShots&&Aim->Suppression==AimPressure&&Aim->Health==AimHealth);
    auto* CachedRifle=Make(0,EFrontlineRole::Rifle,648283,1,Gun->Feet());
    if(CachedRifle)
    {
     CachedRifle->FocusTarget=Aim;CachedRifle->TargetReviewClock=1.f;CachedRifle->WeaponPhase=EFrontlineWeaponPhase::Ready;
     const int32 Rounds=CachedRifle->RoundsRemaining,PersonalShots=CachedRifle->ShotsDischarged;
     CachedRifle->DischargeWeapon(Aim);
     Check(TEXT("v8213_cached_rifle_does_not_spend_round_or_start_recoil_through_wall"),!B->HasSight(CachedRifle,Aim)
      &&CachedRifle->FocusTarget.Get()==Aim&&CachedRifle->TargetReviewClock>0&&CachedRifle->RoundsRemaining==Rounds
      &&CachedRifle->ShotsDischarged==PersonalShots&&CachedRifle->WeaponPhase==EFrontlineWeaponPhase::Ready&&B->ShotsFired==WallShots);
     Wall->SetActorEnableCollision(false);B->Random.Initialize(MissSeed);CachedRifle->DischargeWeapon(Aim);
     Check(TEXT("v8213_cached_rifle_resumes_normal_round_after_sight_recovers"),B->HasSight(CachedRifle,Aim)
      &&CachedRifle->RoundsRemaining==Rounds-1&&CachedRifle->ShotsDischarged==PersonalShots+1
      &&CachedRifle->WeaponPhase==EFrontlineWeaponPhase::Recoil&&B->ShotsFired==WallShots+1&&Aim->Health==AimHealth);
     CachedRifle->SetActorHiddenInGame(true);CachedRifle->SetActorEnableCollision(false);
    }
    else Check(TEXT("v8213_cached_rifle_fixture"),false);
    Wall->SetActorEnableCollision(false);Wall->Destroy();

   // One real mounted gun crew, controlled misses, full production Simulate.
   // Compare time spent firing, not a constant copied out of the implementation.
   B->Units={Aim};Aim->Suppression=0;Aim->SetActorLocation(FVector(1500*Dir,0,10088));
   auto* Tank=Fleet->Spawn(EFrontlineVehicle::Whippet,0,true);Check(TEXT("v828_mounted_gun_fixture"),Tank!=nullptr);
   if(Tank)
   {
    Tank->HeadingYawV37=Dir>0?0.f:180.f;Tank->SetActorRotation(FRotator(0,Tank->HeadingYawV37,0));
    Tank->SetActorLocation(FVector(-500*Dir,0,10000));Tank->bEntered=true;
    for(auto& Clock:Tank->GunClock)Clock=0;
    FVector Point;bool Solution=false;
    for(int32 I=0;I<Tank->GunMounts.Num();++I)Solution|=Tank->InfantryFiringSolutionV828(Aim,I,false,Point);
    const int32 Shots=Tank->MachinegunShots;
    for(int32 I=0;I<80;++I){B->Elapsed+=.05f;Tank->Random.Initialize(MissSeed);Tank->Simulate(.05f);}
    Check(TEXT("v828_mounted_gun_fires_sustained_bursts_with_one_whippet_crew"),Solution&&Tank->MachinegunShots-Shots>=14&&Aim->IsAlive());
    UE_LOG(LogTemp,Display,TEXT("WF_V828_MOUNTED_CADENCE shots_4s=%d suppression=%.3f"),Tank->MachinegunShots-Shots,Aim->Suppression);
    // A very close/high target is observable but above every real mount's
    // elevation. The hull must continue a legal approach instead of idling.
    const FVector Start(-1200*Dir,-650,FrontlineInfantryV45::SurfaceHeight(B,FVector(-1200*Dir,-650,0)));
    Tank->SetActorLocation(Start);Tank->SetActorRotation(FRotator(0,Tank->HeadingYawV37,0));
    Tank->PreferredLaneV37=Tank->DesiredLaneV37=-650;Tank->RouteDecisionAtV37=B->Elapsed+10;
    Aim->SetActorLocation(Start+FVector(1200*Dir,0,1900));
    bool ElevationBlocked=true;for(int32 I=0;I<Tank->GunMounts.Num();++I)ElevationBlocked&=!Tank->InfantryFiringSolutionV828(Aim,I,false,Point);
    const int32 Before=Tank->MachinegunShots;const float Travel=Tank->DistanceTravelled;
    for(int32 I=0;I<10;++I){B->Elapsed+=.05f;Tank->Simulate(.05f);}
    Check(TEXT("v828_observation_without_mount_solution_does_not_deadlock_armour"),ElevationBlocked&&Tank->MachinegunShots==Before&&Tank->DistanceTravelled>Travel+.1f);
    Tank->SetActorEnableCollision(false);Tank->Destroy();Fleet->Vehicles.Empty();
   }
   B->Units={Gun};B->Squads.Empty();
   const FVector GunFeet(-1200*Dir,400,FrontlineInfantryV45::SurfaceHeight(B,FVector(-1200*Dir,400,0)));
   Gun->SetActorLocation(GunFeet+FVector(0,0,88));Gun->SetActorRotation(FRotator(0,Dir>0?0:180,0));Gun->bMoving=false;
   auto* Escort=Make(0,EFrontlineRole::Lewis,648280,1,GunFeet+FVector(-150*Dir,170,0));
   FFrontlineSquad Crew;Crew.Id=648280;Crew.Team=0;Crew.Role=EFrontlineRole::Lewis;Crew.bAdvancing=true;
   Crew.Trench=INDEX_NONE;Crew.Destination=B->GetGoalTrenchIndex(0);Crew.Members={Gun,Escort};B->Squads.Add(Crew);
   auto* Support=FrontlineMachinegunsV30::Get(Gun);
   if(Support&&Escort)
   {
    Support->State=EFrontlineGunStanceV30::Supported;Support->Update(.01f,false);
    bool Reform=false;const FVector Station=FrontlineMachinegunsV30::Destination(Escort,B->SoldierDestination(Escort),Reform);
    Gun->bMoving=true;bool Again=false;const FVector Stable=FrontlineMachinegunsV30::Destination(Escort,B->SoldierDestination(Escort),Again);Gun->bMoving=false;
    Check(TEXT("v828_assistant_station_survives_residual_primary_velocity"),FrontlineMachinegunsV30::HasGroundStationV828(Escort)&&Station.Equals(Stable,.01f));
    Escort->SetActorLocation(Station+FVector(20,0,88));FrontlineMachinegunsV30::Destination(Escort,B->SoldierDestination(Escort),Reform);
    Escort->SetActorLocation(Station+FVector(45,0,88));FrontlineMachinegunsV30::Destination(Escort,B->SoldierDestination(Escort),Again);
    Check(TEXT("v828_assistant_arrival_has_stable_local_hysteresis"),!Reform&&!Again);
    Support->Update(.01f,true);bool DepartReform=false;const FVector March=B->SoldierDestination(Escort);
    Check(TEXT("v828_real_primary_departure_releases_assistant_station"),FrontlineMachinegunsV30::Destination(Escort,March,DepartReform).Equals(March,.01f)&&!DepartReform);
   }
   else Check(TEXT("v828_assistant_supported_fixture"),false);

   // Both opponents run their normal decision/weapon/movement code. The
   // fixed-step fixture advances the gameplay clock and evaluated skeleton;
   // none of these decisions require the world's wall-clock to advance.
   for(AWarfareFrontlineSoldier* U:B->Units)if(U){U->SetActorEnableCollision(false);U->SetActorHiddenInGame(true);}
   B->Units.Empty();B->Squads.Empty();Fleet->Vehicles.Empty();
   FVector RifleFeet(1000*Dir,-650,0);RifleFeet.Z=FrontlineInfantryV45::SurfaceHeight(B,RifleFeet);
   auto* DuelRifle=Make(1,EFrontlineRole::Rifle,648290,1,RifleFeet);
   auto* DuelTank=Fleet->Spawn(EFrontlineVehicle::Whippet,0,true);
   Check(TEXT("v828_two_sided_contact_actual_ground_fixtures"),DuelRifle&&DuelTank);
   if(DuelRifle&&DuelTank)
   {
    FFrontlineSquad Section;Section.Id=DuelRifle->SquadId;Section.Team=1;Section.Role=EFrontlineRole::Rifle;
    Section.bAdvancing=true;Section.Trench=INDEX_NONE;Section.Destination=B->GetGoalTrenchIndex(1);Section.Members={DuelRifle};B->Squads.Add(Section);
    DuelRifle->ReturnToIdle();DuelRifle->ShotClock=DuelRifle->AimSettleClock=DuelRifle->DepartureClock=0;
    DuelRifle->SetActorRotation(FRotator(0,Dir>0?180.f:0.f,0));DuelRifle->SetMotion(DuelRifle->GetAimMotion());
    auto EvaluateRifle=[&](float DT)
    {DuelRifle->Soldier->HandleExistingParallelEvaluationTask(true,true);DuelRifle->Soldier->TickAnimation(DT,false);DuelRifle->Soldier->RefreshBoneTransforms();};
    // Complete the ordinary low-ready -> aim blend before measuring either
    // ray. Sampling the first 50ms measured an intermediate, transient pose.
    DuelRifle->bEngaging=true;EvaluateRifle(.6f);
    DuelTank->HeadingYawV37=Dir>0?0.f:180.f;DuelTank->SetActorRotation(FRotator(0,DuelTank->HeadingYawV37,0));
    DuelTank->FitGroundV35(FVector(-1000*Dir,-650,0),0.f);DuelTank->bEntered=true;
    DuelTank->PreferredLaneV37=DuelTank->DesiredLaneV37=-650;DuelTank->RouteDecisionAtV37=B->Elapsed+20;
    for(float& Clock:DuelTank->GunClock)Clock=0;
    FVector MountPoint;bool MountClear=false;
    for(int32 I=0;I<DuelTank->GunMounts.Num();++I)MountClear|=DuelTank->InfantryFiringSolutionV828(DuelRifle,I,false,MountPoint);
    const bool InitiallyClear=Fleet->AcquireInfantryTarget(DuelRifle)==DuelTank&&Fleet->HasInfantryFiringLineV828(DuelRifle,DuelTank)&&MountClear;
    Check(TEXT("v828_duel_starts_with_real_two_sided_observation_and_mount_solution"),InitiallyClear);

    // Measure the full eye/muzzle separation, including the lateral offset.
    // Bound the actual animated muzzle ray at several fixed planes
    // after evaluating the normal aim pose at both pressure endpoints. Place
    // one fixed sight bar around that envelope with a physical eye clearance;
    // it never follows the actor, changes the pose or overrides a sight query.
    TArray<FVector> AimMuzzles;
    for(float Pressure:{0.f,1.f})
    {
     DuelRifle->Suppression=Pressure;
     for(int32 Frame=0;Frame<160;++Frame){EvaluateRifle(.025f);AimMuzzles.Add(DuelRifle->GetMuzzleLocation());}
    }
    DuelRifle->Suppression=0;EvaluateRifle(.025f);AimMuzzles.Add(DuelRifle->GetMuzzleLocation());
    const FVector Eye=DuelRifle->Feet()+FVector(0,0,145),AimPoint=DuelTank->AimPoint();
    FVector BarAt=FVector::ZeroVector,BarExtent=FVector::ZeroVector;FRotator BarRotation=FRotator::ZeroRotator;float RayGap=0;
    const FVector PlaneAxis(FMath::Sign(AimPoint.X-Eye.X),0,0);
    for(float Distance:{140.f,220.f,300.f})
    {
     const float PlaneX=Eye.X+PlaneAxis.X*Distance;
     const FVector EyeAtPlane=FMath::Lerp(Eye,AimPoint,(PlaneX-Eye.X)/(AimPoint.X-Eye.X));
     TArray<FVector> Crossings;FVector Mean=FVector::ZeroVector;bool Ahead=true;
     for(const FVector& Muzzle:AimMuzzles)
     {
      const float T=(PlaneX-Muzzle.X)/(AimPoint.X-Muzzle.X);Ahead&=T>0&&T<1;
      const FVector P=FMath::Lerp(Muzzle,AimPoint,T);Crossings.Add(P);Mean+=P;
     }
     if(!Ahead||Crossings.IsEmpty())continue;
     Mean/=Crossings.Num();const FVector Across=(Mean-EyeAtPlane).GetSafeNormal();
     if(Across.IsNearlyZero())continue;
     float Near=MAX_flt,Far=-MAX_flt;
     for(const FVector& P:Crossings){const float Projection=FVector::DotProduct(P-EyeAtPlane,Across);Near=FMath::Min(Near,Projection);Far=FMath::Max(Far,Projection);}
     if(Near<=RayGap)continue;
     const float Margin=FMath::Min(4.f,Near*.25f);RayGap=Near;
     BarAt=EyeAtPlane+Across*((Near+Far)*.5f);BarExtent=FVector(8,160,(Far-Near)*.5f+Margin);
     BarRotation=FRotationMatrix::MakeFromXZ(PlaneAxis,Across).Rotator();
    }
    auto* SightBar=B->GetWorld()->SpawnActor<AActor>();
    if(SightBar)
    {
     auto* Shape=NewObject<UBoxComponent>(SightBar);SightBar->SetRootComponent(Shape);SightBar->AddInstanceComponent(Shape);
     Shape->SetBoxExtent(BarExtent.ComponentMax(FVector(1)));
     Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Shape->SetCollisionResponseToAllChannels(ECR_Ignore);
     Shape->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Shape->RegisterComponent();SightBar->SetActorLocationAndRotation(BarAt,BarRotation);
    }
    FCollisionQueryParams BarQuery(SCENE_QUERY_STAT(V828FixedMuzzleEnvelope),true);BarQuery.AddIgnoredActor(DuelRifle);BarQuery.AddIgnoredActor(DuelTank);
    bool WholeAimBlocked=SightBar!=nullptr;
    for(const FVector& Muzzle:AimMuzzles)
    {FHitResult Hit;WholeAimBlocked&=B->GetWorld()->LineTraceSingleByChannel(Hit,Muzzle,AimPoint,ECC_Visibility,BarQuery)&&Hit.GetActor()==SightBar;}
    const bool Split=WholeAimBlocked&&RayGap>6.f&&Fleet->AcquireInfantryTarget(DuelRifle)==DuelTank&&!Fleet->HasInfantryFiringLineV828(DuelRifle,DuelTank);
    Check(TEXT("v828_duel_fixed_obstruction_keeps_eye_contact_but_blocks_actual_muzzle"),Split);
    FrontlineMobilityV41::Reset(B);
    constexpr float Step=.025f;float ReadyBlocked=0,LongestReadyBlocked=0,FirstAdvance=-1,Travel=0;
    float FirstSightClear=-1,FirstContactLost=-1,BlockedBeforeAdvance=0;
    bool NoBlockedDischarge=true,Finite=true;int32 RifleAtClear=0,TankAtClear=0;
    const FVector Start=DuelRifle->Feet();
    for(int32 Frame=0;Frame<380;++Frame)
    {
     if(Frame==140)
     {if(SightBar){SightBar->SetActorEnableCollision(false);SightBar->Destroy();}RifleAtClear=DuelRifle->ShotsDischarged;TankAtClear=DuelTank->MachinegunShots;}
     B->Elapsed+=Step;DuelTank->Random.Initialize(MissSeed);DuelTank->Simulate(Step);
     const bool HasContact=Fleet->AcquireInfantryTarget(DuelRifle)==DuelTank;
     const bool ClearMuzzle=Fleet->HasInfantryFiringLineV828(DuelRifle,DuelTank);
     const bool Blocked=HasContact&&!ClearMuzzle;
     if(Frame<140)
     {
      if(ClearMuzzle&&FirstSightClear<0)FirstSightClear=(Frame+1)*Step;
      if(!HasContact&&FirstContactLost<0)FirstContactLost=(Frame+1)*Step;
     }
     ReadyBlocked=Blocked&&DuelRifle->WeaponPhase==EFrontlineWeaponPhase::Ready?ReadyBlocked+Step:0;
     LongestReadyBlocked=FMath::Max(LongestReadyBlocked,ReadyBlocked);
     if(Frame<140&&FirstAdvance<0)BlockedBeforeAdvance=FMath::Max(BlockedBeforeAdvance,ReadyBlocked);
     const FVector Before=DuelRifle->Feet();const int32 BeforeShots=DuelRifle->ShotsDischarged;
     B->Random.Initialize(MissSeed);DuelRifle->Tick(Step);EvaluateRifle(Step);
     if(Blocked)NoBlockedDischarge&=DuelRifle->ShotsDischarged==BeforeShots;
     Finite&=!DuelRifle->Feet().ContainsNaN()&&!DuelTank->GetActorTransform().ContainsNaN();
     if(Frame<140)
     {
      Travel+=FVector::Dist2D(Before,DuelRifle->Feet());
      if(FirstAdvance<0&&(DuelRifle->Feet().X-Start.X)*B->GetTeamDirection(1)>5.f)FirstAdvance=(Frame+1)*Step;
     }
    }
    // This opponent really drives and turns, so a muzzle ray or awareness
    // contact may clear before the obstruction timeout. Require an observed
    // release before the first advance; without one, retain the sustained
    // blocked-ready wait. The fixed AT wall below covers persistent recovery.
    const bool SightReleasedBeforeAdvance=(FirstSightClear>=0&&FirstSightClear<=FirstAdvance)
     ||(FirstContactLost>=0&&FirstContactLost<=FirstAdvance);
    const bool WaitedForBlockedRecovery=BlockedBeforeAdvance>=1.9f&&FirstAdvance>=1.9f;
    Check(FString::Printf(TEXT("v828_soldier_releases_dynamic_armour_contact_without_firing_through_bar split=%d ready=%.3f ready_before_move=%.3f first=%.3f clear=%.3f lost=%.3f travel=%.2f no_blocked_shot=%d ray_gap=%.2f"),
      Split,LongestReadyBlocked,BlockedBeforeAdvance,FirstAdvance,FirstSightClear,FirstContactLost,Travel,NoBlockedDischarge,RayGap),
     Split&&(SightReleasedBeforeAdvance||WaitedForBlockedRecovery)&&FirstAdvance>=0&&FirstAdvance<3.5f&&Travel>5.f&&NoBlockedDischarge);
    Check(TEXT("v828_both_real_opponents_resume_discharge_after_obstruction_removed"),InitiallyClear&&Finite&&DuelRifle->IsAlive()&&DuelTank->IsOperational()
     &&DuelRifle->ShotsDischarged>RifleAtClear&&DuelTank->MachinegunShots>TankAtClear);
    UE_LOG(LogTemp,Display,TEXT("WF_V828_TWO_SIDED_CONTACT seconds=9.5 step=%.3f split=%d ray_gap=%.2f blocked_ready=%.3f first_advance=%.3f blocked_travel=%.2f rifle_before_clear=%d rifle_final=%d armour_before_clear=%d armour_final=%d alive=%d"),
     Step,Split,RayGap,LongestReadyBlocked,FirstAdvance,Travel,RifleAtClear,DuelRifle->ShotsDischarged,TankAtClear,DuelTank->MachinegunShots,DuelRifle->IsAlive());
   }
   if(DuelTank){DuelTank->SetActorEnableCollision(false);DuelTank->Destroy();}Fleet->Vehicles.Empty();
   B->Units.Empty();B->Squads.Empty();


   // A separate real supported AT gun keeps one fixed low wall throughout
   // recovery. The target hull is stationary and does not retaliate: the
   // preceding two-sided fixture already covers firing both opponents. Here
   // neither removing the obstacle nor a transient standing muzzle may fake
   // the departure from a persistently blocked prone firing point.
   if(DuelRifle){DuelRifle->SetActorEnableCollision(false);DuelRifle->SetActorHiddenInGame(true);}
   FVector ATFeet(1000*Dir,-650,0);ATFeet.Z=FrontlineInfantryV45::SurfaceHeight(B,ATFeet);
   auto* RecoveryGun=Make(1,EFrontlineRole::AntiTankRifle,648291,0,ATFeet);
   auto* RecoveryTank=Fleet->Spawn(EFrontlineVehicle::Whippet,0,true);
   auto* RecoverySupport=FrontlineMachinegunsV30::Get(RecoveryGun);
   Check(TEXT("v828_supported_recovery_has_real_at_and_hull_assets"),RecoveryGun&&RecoverySupport&&RecoverySupport->HasAssets()&&RecoveryTank);
   if(RecoveryGun&&RecoverySupport&&RecoveryTank)
   {
    FFrontlineSquad ATSection;ATSection.Id=RecoveryGun->SquadId;ATSection.Team=1;ATSection.Role=EFrontlineRole::AntiTankRifle;
    ATSection.bAdvancing=true;ATSection.Trench=INDEX_NONE;ATSection.Destination=B->GetGoalTrenchIndex(1);ATSection.Members={RecoveryGun};B->Squads.Add(ATSection);
    RecoveryGun->ReturnToIdle();RecoveryGun->ShotClock=10000;RecoveryGun->DepartureClock=0;
    RecoveryGun->SetActorRotation(FRotator(0,Dir>0?180.f:0.f,0));
    RecoveryTank->HeadingYawV37=Dir>0?0.f:180.f;RecoveryTank->SetActorRotation(FRotator(0,RecoveryTank->HeadingYawV37,0));
    RecoveryTank->FitGroundV35(FVector(-1000*Dir,-650,0),0.f);RecoveryTank->bEntered=true;
    const auto TickRecovery=[&](float DT)
    {
     B->Elapsed+=DT;RecoveryGun->Tick(DT);
     RecoveryGun->Soldier->HandleExistingParallelEvaluationTask(true,true);
     RecoveryGun->Soldier->TickAnimation(DT,false);RecoveryGun->Soldier->RefreshBoneTransforms();
     RecoverySupport->TickComponent(DT,LEVELTICK_All,nullptr);
    };
    constexpr float RecoveryStep=.025f;float SupportedPoseSeconds=0;
    for(int32 Frame=0;Frame<160&&SupportedPoseSeconds<.6f;++Frame)
    {
     TickRecovery(RecoveryStep);
     SupportedPoseSeconds=RecoverySupport->IsSupported()&&RecoveryGun->Playing==RecoveryGun->GetAimMotion()
       &&RecoveryGun->WeaponPhase==EFrontlineWeaponPhase::Ready?SupportedPoseSeconds+RecoveryStep:0;
    }
    Check(TEXT("v828_recovery_starts_after_actual_personal_deployment_and_prone_aim"),SupportedPoseSeconds>=.6f
     &&RecoverySupport->IsProne()&&RecoverySupport->CanFire()&&RecoverySupport->ObserveArmourV48()==RecoveryTank);
    const FVector OriginalFeet=RecoveryGun->Feet(),OriginalMuzzle=RecoveryGun->GetMuzzleLocation(),ArmourAim=RecoveryTank->AimPoint();
    const FVector WallPoint=FMath::Lerp(OriginalMuzzle,ArmourAim,.07f),Eye=OriginalFeet+FVector(0,0,145);
    const float EyeT=(WallPoint.X-Eye.X)/(ArmourAim.X-Eye.X),EyeZ=FMath::Lerp(Eye.Z,ArmourAim.Z,EyeT);
    const float Top=(EyeZ+WallPoint.Z)*.5f,Bottom=FrontlineInfantryV45::SurfaceHeight(B,WallPoint)-30.f;
    auto* LowWall=B->GetWorld()->SpawnActor<AActor>();
    if(LowWall)
    {
     auto* Shape=NewObject<UBoxComponent>(LowWall);LowWall->SetRootComponent(Shape);LowWall->AddInstanceComponent(Shape);
     Shape->SetBoxExtent(FVector(15,400,FMath::Max(1.f,(Top-Bottom)*.5f)));
     Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Shape->SetCollisionResponseToAllChannels(ECR_Ignore);
     Shape->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Shape->RegisterComponent();
     LowWall->SetActorLocation(FVector(WallPoint.X,WallPoint.Y,(Top+Bottom)*.5f));
    }
    const bool PersistentSplit=LowWall&&EyeZ-WallPoint.Z>25.f&&Top>Bottom
     &&RecoverySupport->ObserveArmourV48()==RecoveryTank&&!RecoverySupport->HasArmourFiringLineV48(RecoveryTank);
    Check(TEXT("v828_supported_recovery_fixed_wall_separates_eye_and_prone_muzzle"),PersistentSplit);
    RecoveryGun->ShotClock=0;FrontlineMobilityV41::Reset(B);
    bool Packed=false,NoOldPointRedeploy=true,NoBlockedShot=true,Finite=true;float FirstMovement=-1,Displacement=0,MaxStep=0;
    for(int32 Frame=0;Frame<240;++Frame)
    {
     const FVector Before=RecoveryGun->Feet();const int32 Shots=RecoveryGun->ShotsDischarged;
     const bool Blocked=!RecoverySupport->HasArmourFiringLineV48(RecoveryTank);
     TickRecovery(RecoveryStep);
     Packed|=RecoverySupport->State==EFrontlineGunStanceV30::Pack;
     Displacement=FMath::Max(Displacement,float(FVector::Dist2D(OriginalFeet,RecoveryGun->Feet())));
     MaxStep=FMath::Max(MaxStep,float(FVector::Dist2D(Before,RecoveryGun->Feet())));
     if(FirstMovement<0&&(RecoveryGun->Feet().X-OriginalFeet.X)*B->GetTeamDirection(1)>5.f)FirstMovement=(Frame+1)*RecoveryStep;
     if(Packed&&RecoverySupport->State==EFrontlineGunStanceV30::Deploy)
      NoOldPointRedeploy&=FVector::Dist2D(OriginalFeet,RecoveryGun->Feet())>80.f;
     if(Blocked)NoBlockedShot&=RecoveryGun->ShotsDischarged==Shots;
     Finite&=!RecoveryGun->Feet().ContainsNaN();
    }
    FCollisionQueryParams KeptWallQuery(SCENE_QUERY_STAT(V828PersistentATWall),true);KeptWallQuery.AddIgnoredActor(RecoveryGun);KeptWallQuery.AddIgnoredActor(RecoveryTank);
    FHitResult StillBlocked;const bool WallStillThere=LowWall&&B->GetWorld()->LineTraceSingleByChannel(StillBlocked,OriginalMuzzle,ArmourAim,ECC_Visibility,KeptWallQuery)&&StillBlocked.GetActor()==LowWall;
    Check(FString::Printf(TEXT("v828_supported_gun_packs_and_relocates_before_redeploy_under_persistent_wall split=%d pack=%d first=%.3f travel=%.2f max_step=%.2f new_point=%d wall=%d no_blocked_shot=%d"),
     PersistentSplit,Packed,FirstMovement,Displacement,MaxStep,NoOldPointRedeploy,WallStillThere,NoBlockedShot),PersistentSplit&&WallStillThere
     &&Packed&&NoOldPointRedeploy&&FirstMovement>=1.9f&&FirstMovement<6.f&&Displacement>100.f&&Finite&&MaxStep<15.f&&NoBlockedShot);
    UE_LOG(LogTemp,Display,TEXT("WF_V828_SUPPORTED_ARMOUR_RECOVERY split=%d eye_gap=%.2f pack=%d first_movement=%.3f displacement=%.2f max_step=%.2f no_old_point_deploy=%d shots=%d wall_retained=%d"),
     PersistentSplit,EyeZ-WallPoint.Z,Packed,FirstMovement,Displacement,MaxStep,NoOldPointRedeploy,RecoveryGun->ShotsDischarged,WallStillThere);
    if(LowWall){LowWall->SetActorEnableCollision(false);LowWall->Destroy();}
   }
   if(RecoveryGun){RecoveryGun->SetActorEnableCollision(false);RecoveryGun->SetActorHiddenInGame(true);}
   if(RecoveryTank){RecoveryTank->SetActorEnableCollision(false);RecoveryTank->Destroy();}Fleet->Vehicles.Empty();
   B->Units.Empty();B->Squads.Empty();

   // Each real death needs its own actor: RegisterCasualty deliberately
   // deduplicates casualties, and reusing one would mask a missing notice.
   for(int32 VictimTeam=0;VictimTeam<2;++VictimTeam)for(int32 Attacker=0;Attacker<2;++Attacker)
   {
    auto* Victim=Make(VictimTeam,EFrontlineRole::Rifle,648300+VictimTeam*2+Attacker,1,FVector(0,0,10000));
    const FString Sentinel=TEXT("v828_notice_unchanged");B->LastMessage=Sentinel;
    const int32 Deaths=B->Casualties[VictimTeam];
    if(Victim)Victim->ReceiveHit(9999,0,Attacker,FVector::ForwardVector,EFrontlineDamageKind::Artillery);
    const bool PlayerFriendly=VictimTeam==0&&Attacker==0;
    Check(FString::Printf(TEXT("v828_actual_artillery_notice_victim%d_attacker%d"),VictimTeam,Attacker),Victim&&!Victim->IsAlive()
     &&B->Casualties[VictimTeam]==Deaths+1&&(PlayerFriendly?B->LastMessage==TEXT("友军遭到爆炸误伤。"):B->LastMessage==Sentinel));
   }
   auto* BulletVictim=Make(0,EFrontlineRole::Rifle,648304,1,FVector(0,0,10000));B->LastMessage=TEXT("v828_notice_unchanged");
   if(BulletVictim)BulletVictim->ReceiveHit(9999,0,0,FVector::ForwardVector,EFrontlineDamageKind::Bullet);
   Check(TEXT("v828_player_friendly_bullet_does_not_emit_explosion_notice"),BulletVictim&&!BulletVictim->IsAlive()&&B->LastMessage==TEXT("v828_notice_unchanged"));
   return;
  }
  auto* Rifle=Make(0,EFrontlineRole::Rifle,640001,0,FVector(-1000,0,10000));
  auto* Sniper=Make(0,EFrontlineRole::Marksman,640002,0,FVector(-1000,350,10000));
  auto* Spotter=Make(0,EFrontlineRole::Marksman,640002,1,FVector(-1080,450,10000));
  auto* Enemy=Make(1,EFrontlineRole::Rifle,640003,0,FVector(1000,0,10000));
  Check(TEXT("v64_actual_rifle_sniper_spotter_assets"),Rifle&&Sniper&&Spotter&&Enemy);if(!Rifle||!Sniper||!Spotter||!Enemy)return;
  FFrontlineSquad Pair;Pair.Id=640002;Pair.Team=0;Pair.Role=EFrontlineRole::Marksman;Pair.Members={Sniper,Spotter};Pair.bAdvancing=true;B->Squads.Add(Pair);
  const auto& Primary=FrontlineWeapons::ProfileFor(Sniper);const auto& Secondary=FrontlineWeapons::ProfileFor(Spotter);
  Check(TEXT("v64_sniper_pair_has_one_scope_and_one_service_rifle"),Sniper->IsScopedMarksman()&&!Spotter->IsScopedMarksman()
   &&Primary.FireRangeCm>Secondary.FireRangeCm&&Secondary.HitProbabilityScale==FrontlineWeapons::ProfileFor(Rifle).HitProbabilityScale);
  Check(TEXT("v64_spotter_retains_pair_contact_but_not_sniper_fire_range"),FrontlineWeapons::AdvanceContactRangeFor(Spotter)==Primary.AdvanceContactCm&&Secondary.FireRangeCm<Primary.AdvanceContactCm);
  Enemy->SetActorLocation(Rifle->GetActorLocation()+FVector(1500,0,0));const float Near=B->BulletHitChance(Rifle,Enemy);
  Enemy->SetActorLocation(Rifle->GetActorLocation()+FVector(3600,0,0));const float Far=B->BulletHitChance(Rifle,Enemy);
  Rifle->Suppression=.8f;const float Pinned=B->BulletHitChance(Rifle,Enemy);Rifle->Suppression=0;
  Check(TEXT("v64_service_rifle_range_and_pressure_reduce_actual_hit_chance"),Near>0&&Far>0&&Far<Near*.52f&&Pinned<Far*.55f);
  Enemy->Role=EFrontlineRole::Lewis;Enemy->MemberIndex=0;
  const float MG=FrontlineBalanceV50::TargetExposure(Sniper,Enemy,.75f);Enemy->Role=EFrontlineRole::Rifle;
  const float Ordinary=FrontlineBalanceV50::TargetExposure(Sniper,Enemy,.75f);
  Check(TEXT("v64_precision_advantage_targets_exposed_gunner_not_every_trench_rifle"),MG>Ordinary*1.5f&&Ordinary==FrontlineBalanceV50::TargetExposure(Rifle,Enemy,.75f));
  auto D=[](EFrontlineVehicle T)->const FFrontlineVehicleEntry&{return UFrontlineVehicleSubsystem::Definition(T);};
  const float Prior[]={130,245,145,120,138,125,165,138,145,125};bool Speeds=true;
  for(int32 I=0;I<UE_ARRAY_COUNT(Prior);++I)Speeds&=FMath::IsNearlyEqual(D(EFrontlineVehicle(I)).SpeedCm,Prior[I]*(I==int32(EFrontlineVehicle::Whippet)?1.10f:1.25f),.001f);
  Check(TEXT("v64_whippet_ten_percent_all_other_armour_twenty_five"),Speeds);
  Check(TEXT("v64_pioneer_shared_queue_cost_is_materially_lower_than_fighting_tanks"),D(EFrontlineVehicle::MarkIVPioneer).Cooldown<D(EFrontlineVehicle::MarkIVFemale).Cooldown*.82f
   &&D(EFrontlineVehicle::BeutePioneer).Cooldown<D(EFrontlineVehicle::BeuteMarkIVFemale).Cooldown*.82f
   &&D(EFrontlineVehicle::Whippet).Cooldown<D(EFrontlineVehicle::MarkIVMale).Cooldown&&D(EFrontlineVehicle::MarkVMale).Cooldown>D(EFrontlineVehicle::MarkIVMale).Cooldown);
  Check(TEXT("v64_specialists_have_lower_actual_queue_cost"),B->EffectiveRecruitSeconds(EFrontlineRole::Officer)<B->EffectiveRecruitSeconds(EFrontlineRole::Rifle)
   &&B->RoleRecruitSeconds(EFrontlineRole::RifleGrenadier)==12.f*FrontlinePacingV24::BaseRecruitScale&&B->RoleRecruitSeconds(EFrontlineRole::Flamethrower)==13.f*FrontlinePacingV24::BaseRecruitScale);
  FFrontlineSquad Rear;Rear.Id=640001;Rear.Team=0;Rear.Role=EFrontlineRole::Rifle;Rear.Members={Rifle};Rear.bAdvancing=true;Rear.Trench=INDEX_NONE;
  Rear.LastPassedTrench=B->GetHomeTrenchIndex(1);Rear.Destination=B->GetGoalTrenchIndex(0);B->Squads.Add(Rear);
  const int32 Dir=B->GetTeamDirection(0);Rifle->SetActorLocation(FVector(Dir*(FrontlineGameplayV49::BreakthroughLine()+60),0,10088));
  const FVector Destination=B->SoldierDestination(Rifle);
  Check(TEXT("v64_first_arrival_continues_beyond_objective_without_staging_at_quorum"),Destination.X*Dir>Rifle->Feet().X*Dir+600&&Destination.X*Dir<FrontlineBattlefield::GroundHalfLength);
  Enemy->SetActorLocation(Rifle->GetActorLocation()-FVector(Dir*300,0,0));
  Check(TEXT("v64_leading_soldier_still_engages_rear_defender"),B->AcquireTarget(Rifle,true)==Enemy&&!FrontlineGameplayV43::HasCrossedEnemyEnd(B,Rear));
  B->Units={Enemy};Enemy->Role=EFrontlineRole::Rifle;Enemy->Suppression=0;Enemy->SetActorLocation(FVector(0,0,10088));
  FFrontlineStrike Airburst;Airburst.Team=0;Airburst.bShrapnelV64=true;Airburst.Radius=920;Airburst.Target=Enemy->Feet()+FVector(0,0,430);
  const float Open=B->ExplosionHitChance(Airburst,Enemy);
  const int32 Home=B->GetHomeTrenchIndex(1);const int32 Owner=B->Trenches[Home].Owner;B->Trenches[Home].Owner=1;
  FFrontlineSquad Sheltered;Sheltered.Id=Enemy->SquadId;Sheltered.Team=1;Sheltered.Trench=Home;Sheltered.Members={Enemy};B->Squads.Add(Sheltered);
  const float X=FrontlineFieldV32::TrenchCentreX(Home,0);Enemy->SetActorLocation(FVector(X,0,10088));Airburst.Target=Enemy->Feet()+FVector(0,0,430);
  const float Covered=B->ExplosionHitChance(Airburst,Enemy);B->Trenches[Home].Owner=Owner;
  Check(TEXT("v64_airburst_is_lethal_open_but_trench_cover_is_a_real_counter"),Open>.70f&&Covered<Open*.20f);
  B->Squads.Empty();Enemy->SetActorLocation(FVector(0,0,10088));Airburst.Target=Enemy->Feet()+FVector(0,0,430);
  auto* Tank=Fleet->Spawn(EFrontlineVehicle::A7V,1,true);Check(TEXT("v64_shrapnel_tank_fixture"),Tank!=nullptr);
  if(Tank)
  {
   Tank->SetActorLocation(FVector(0,0,10000));Tank->SetActorEnableCollision(false);const float Hull=Tank->HullIntegrity,Track=Tank->TrackIntegrity[0];
   for(int32 Seed=1;Seed<1000;++Seed){FRandomStream R(Seed);if(R.FRand()<.1f){B->Random.Initialize(Seed);break;}}
   B->ResolveStrike(Airburst);
   Check(TEXT("v64_actual_shrapnel_resolution_kills_infantry_without_armour_damage"),!Enemy->IsAlive()&&Tank->HullIntegrity==Hull&&Tank->TrackIntegrity[0]==Track);
  }
  B->Units.Empty();B->Strikes.Empty();FrontlineSupportV32::Reset(B);FrontlineSupportV32::ResetReadinessForDebug(B);
  B->CampaignRuns[0].Research.TechnologyMask&=~(1u<<20);
  const FVector Target(0,0,AWarfareFrontlineGround::HeightAt(0,0));const bool Called=FrontlineSupportV32::Request(B,EFrontlineSupport::AirRecon,Target,0);
  bool Ballistic=Called&&B->Strikes.Num()==6;for(const auto& S:B->Strikes)Ballistic&=S.bShrapnelV64&&S.ImpactAt>S.LaunchedAt&&S.LaunchedAt>B->Elapsed&&S.Target.Z>AWarfareFrontlineGround::HeightAt(S.Target.X,S.Target.Y)+350.f;
  Check(TEXT("v64_retired_recon_calls_six_real_airbursts_with_delay"),Ballistic);
  if(Called&&!B->Strikes.IsEmpty())
  {
   auto& S=B->Strikes[0];B->Elapsed=S.LaunchedAt+.01f;EnsureFrontlineSupportProjectileV30(B,S);
   Check(TEXT("v64_shrapnel_has_a_real_incoming_projectile"),S.Projectile.IsValid());
  }
  B->Strikes.Empty();FrontlineSupportV32::ResetReadinessForDebug(B);B->CampaignRuns[0].Research.TechnologyMask|=1u<<20;
  const bool Trained=FrontlineSupportV32::Request(B,EFrontlineSupport::AirRecon,Target,0);
  Check(TEXT("v64_fuze_research_adds_three_real_shells_not_observation_buff"),Trained&&B->Strikes.Num()==9&&!FrontlineSupportV32::HasRegistrationV37(B,0,Target));
  // Actual placement, bank geometry, deployment, evaluated skeleton and firing
  // in both national directions, all three bays, shallow and rear trenches.
  B->Units.Empty();B->Squads.Empty();B->Strikes.Empty();B->Elapsed=1000;
  int32 MGCase=0;
  for(int32 Team=0;Team<2;++Team)for(int32 Depth=0;Depth<2;++Depth)for(int32 Slot=0;Slot<3;++Slot)
  {
   const int32 Trench=Depth?B->GetHomeTrenchIndex(Team):FrontlineBattlefield::CentreTrench;
   B->Trenches[Trench].Owner=Team;B->Units.Empty();B->Squads.Empty();
   auto* Gunner=Make(Team,EFrontlineRole::Lewis,641000+MGCase,0,FVector::ZeroVector);
   auto* Bearer=Make(Team,EFrontlineRole::Lewis,641000+MGCase,1,FVector::ZeroVector);
   FFrontlineSquad Crew;Crew.Id=641000+MGCase;Crew.Team=Team;Crew.Role=EFrontlineRole::Lewis;Crew.Trench=Trench;Crew.Destination=Trench;Crew.Slot=Slot;Crew.bAdvancing=false;Crew.Members={Gunner,Bearer};B->Squads.Add(Crew);
   const int32 Forward=B->GetTeamDirection(Team);
    bool Fired=false,Ready=false,Clear=false,Traverse=false,WallBlocks=false,NoPressureBonus=false,LiveSight=false,FriendlySight=false;
   if(Gunner&&Bearer)
   {
    // Start in the ordinary assigned trench slot, then reach the selected gun
    // bay through production movement. Do not teleport past a curved bank.
    FVector Station=B->SoldierDestination(Gunner);
    Gunner->SetActorLocation(Station+FVector(0,0,88));Gunner->SetActorRotation(FRotator(0,Forward>0?0:180,0));
    Bearer->SetActorLocation(B->SoldierDestination(Bearer)+FVector(0,0,88));Bearer->SetActorRotation(Gunner->GetActorRotation());
    const FVector Ahead=Station+FVector(Forward*1150.f,(Slot-1)*95.f,0);
    auto* Victim=Make(1-Team,EFrontlineRole::Rifle,642000+MGCase,0,FVector(Ahead.X,Ahead.Y,FrontlineInfantryV45::SurfaceHeight(B,Ahead)));
    if(Victim)
    {
     Victim->SetMotion(Victim->GetAimMotion());Victim->Soldier->TickAnimation(.2f,false);Victim->Soldier->RefreshBoneTransforms();
     auto* C=FrontlineMachinegunsV30::Get(Gunner);
     const int32 Shots=Gunner->ShotsDischarged;
     for(int32 Frame=0;C&&Frame<200&&Gunner->ShotsDischarged==Shots;++Frame)
     {
      if(Team==0&&Depth==0&&Slot==0&&Frame>=194)
      {
       FrontlineMobilityV41::TraceNextStepV64(Gunner);
       UE_LOG(LogTemp,Display,TEXT("V64_MG_STALL_FRAME frame=%d feet=%s wanted=%d arrival=%d requested_speed=%.3f weapon=%d departure=%.3f fire_pose=%.3f alive=%d engaging=%d"),Frame,*Gunner->Feet().ToString(),Gunner->bWantedMovement,Gunner->bAtDestinationV43,Gunner->RequestedTravelSpeedV43,int32(Gunner->WeaponPhase),Gunner->DepartureClock,Gunner->FirePoseClock,Gunner->IsAlive(),Gunner->bEngaging);
      }
      B->Elapsed+=.04f;Gunner->Tick(.04f);Gunner->Soldier->TickAnimation(.04f,false);Gunner->Soldier->RefreshBoneTransforms();C->TickComponent(.04f,LEVELTICK_All,nullptr);
     }
     Ready=C&&C->IsSupported()&&C->IsTrench()&&C->CanFire();Fired=Gunner->ShotsDischarged>Shots;
     Clear=C&&C->HasInfantryFiringLine(Victim);
     if(C&&Ready)
     {
      // A shot may already have killed the first victim. The wall test needs
      // a fresh living target, or an inactive-target rejection would mask it.
      if(!Victim->IsAlive())
      {
       Victim=Make(1-Team,EFrontlineRole::Rifle,643000+MGCase,0,FVector(Ahead.X,Ahead.Y,FrontlineInfantryV45::SurfaceHeight(B,Ahead)));
       if(Victim){Victim->SetMotion(Victim->GetAimMotion());Victim->Soldier->TickAnimation(.2f,false);Victim->Soldier->RefreshBoneTransforms();}
      }
       if(Victim)
       {
        LiveSight=B->HasSight(Gunner,Victim)&&C->HasInfantryFiringLine(Victim);
       // Keep the real curved bay, lateral target and evaluated supported pose.
       // Own riflemen on the viewing line must not become physical sight blockers.
       const FVector Screen=FMath::Lerp(Gunner->Feet(),Victim->Feet(),.45f);
       auto* Friendly0=Make(Team,EFrontlineRole::Rifle,643100+MGCase,0,
        FVector(Screen.X,Screen.Y,FrontlineInfantryV45::SurfaceHeight(B,Screen)));
       auto* Friendly1=Make(Team,EFrontlineRole::Rifle,643200+MGCase,0,
        FVector(Screen.X+Forward*100.f,Screen.Y,FrontlineInfantryV45::SurfaceHeight(B,Screen+FVector(Forward*100.f,0,0))));
       FriendlySight=Friendly0&&Friendly1&&B->HasSight(Gunner,Victim)&&C->HasInfantryFiringLine(Victim);
        const auto Facing=Gunner->GetActorRotation();const int32 Packs=C->Packs;
        Gunner->WeaponPhase=EFrontlineWeaponPhase::Ready;Gunner->FirePoseClock=0;Gunner->bMoving=false;
        Gunner->SetActorRotation(Facing+FRotator(0,50,0));C->Update(.4f,false);
        Traverse=C->IsSupported()&&C->Packs==Packs&&C->CanFire();Gunner->SetActorRotation(Facing);
        auto* Wall=B->GetWorld()->SpawnActor<AActor>();
        auto* Block=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Block);Wall->AddInstanceComponent(Block);Block->SetBoxExtent(FVector(25,800,1800));
        Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Block->SetCollisionResponseToAllChannels(ECR_Ignore);Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Block->RegisterComponent();
        Wall->SetActorLocation((Gunner->GetMuzzleLocation()+Victim->Feet()+FVector(0,0,135))*.5f);
       const int32 Count=Gunner->ShotsDischarged,CombatShots=B->ShotsFired,Rounds=Gunner->RoundsRemaining;
       const float Pressure=Victim->Suppression,Health=Victim->Health;
       Gunner->DischargeWeapon(Victim);B->Fire(Gunner,Victim);
       WallBlocks=Gunner->ShotsDischarged==Count&&Gunner->RoundsRemaining==Rounds&&B->ShotsFired==CombatShots
        &&Victim->Suppression==Pressure&&Victim->Health==Health&&!B->HasSight(Gunner,Victim)&&!C->HasInfantryFiringLine(Victim);Wall->Destroy();
       }
     }
     const float Expected=(Team==0?.072f:.084f)*1.15f*1.25f;
     NoPressureBonus=FMath::IsNearlyEqual(FrontlineWeapons::ProfileFor(Gunner).HitProbabilityScale,Expected,.00001f)
      &&FMath::IsNearlyEqual(FrontlineWeapons::ProfileFor(Bearer).HitProbabilityScale,.25f,.00001f);
     const FVector Bay=FrontlineMachinegunsV30::TrenchFiringStepV36(Gunner,B->SoldierDestination(Gunner));
     UE_LOG(LogTemp,Display,TEXT("V64_MG_CASE team=%d depth=%d slot=%d stance=%d cover=%.3f fire=%d clear=%d shots=%d packs=%d low=%d feet=%s muzzle=%s bay=%s distance=%.2f moving=%d bank=%.2f"),Team,Depth,Slot,C?int32(C->State):-1,B->CoverFor(Gunner),Fired,Clear,Gunner->ShotsDischarged,C?C->Packs:-1,FrontlineMachinegunsV30::UsesLowTrenchPoseV64(Gunner),*Gunner->Feet().ToString(),*Gunner->GetMuzzleLocation().ToString(),*Bay.ToString(),FVector::Dist2D(Bay,Gunner->Feet()),Gunner->bMoving,FrontlineInfantryV45::BankIntrusion(Gunner,Gunner->Feet()));
    }
   }
   const FString Key=FString::Printf(TEXT("v64_trench_mg_actual_fire_team%d_depth%d_slot%d"),Team,Depth,Slot);
   Check(Key,Ready&&Fired);Check(Key+TEXT("_traverse_wall_and_primary_only_uplift"),Traverse&&WallBlocks&&NoPressureBonus);
    Check(Key+TEXT("_live_supported_sight_and_dense_friendly_screen"),LiveSight&&FriendlySight);
   for(AWarfareFrontlineSoldier* U:B->Units)if(U){U->SetActorHiddenInGame(true);U->SetActorEnableCollision(false);}
   ++MGCase;
  }
  auto* Puff=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Target+FVector(0,0,450),FRotator::ZeroRotator);
  if(Puff){Puff->ConfigureAirburstV64();Puff->Tick(.1f);Puff->Tick(.1f);}
  Check(TEXT("v8211_airburst_visual_has_airborne_core_and_bounded_fragment_fan"),Puff&&Puff->IsVisualReady()&&Puff->GetBlastProfileIndex()==4&&Puff->GetBallisticFragmentCount()>0&&Puff->GetBallisticFragmentCount()<=13&&Puff->GetVisibleSmokeCount()>0);
 }
};
void FrontlineCombatV64::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check){FFrontlineCombatV64Review::Run(B,Check);}
TSharedRef<FJsonObject> FrontlineCombatV64::RunAntiArmourChecksV833(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineCombatV64Review::RunAntiArmourV833(B,Check);}
TSharedRef<FJsonObject> FrontlineCombatV64::RunNo44RotationChecksV8310(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineCombatV64Review::RunNo44RotationV8310(B,Check);}
