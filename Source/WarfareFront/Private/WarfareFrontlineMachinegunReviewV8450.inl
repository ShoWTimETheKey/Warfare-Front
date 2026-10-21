// Opt-in, disposable UK/DE production Tick trajectories. Included once by the
// UI review translation unit; no alternate stance or ballistics model is used.
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

struct FFrontlineMachinegunV8450Review
{
    struct FContext
    {
        AWarfareFrontlineGameMode* B;
        TArray<TObjectPtr<AWarfareFrontlineSoldier>> Units;
        TArray<FFrontlineSquad> Squads;
        TArray<FFrontlineTrench> Trenches;
        TArray<FFrontlineTracer> Tracers;
        TArray<FFrontlineSmokeZone> Smoke;
        EFrontlinePhase Phase;float Elapsed,Peak;int32 Outro,Shots,Advances;
        int32 Casualties[2];float Morale[2],Damage[2];FFrontlineCombatRecordV65 Records[2];
        FString Message;
        struct FOriginal{TWeakObjectPtr<AWarfareFrontlineSoldier> U;bool Hidden,Collision;};
        TArray<FOriginal> Originals;TArray<AWarfareFrontlineSoldier*> Made;TArray<AActor*> Walls;
        explicit FContext(AWarfareFrontlineGameMode* In):B(In),Units(In->Units),Squads(In->Squads),Trenches(In->Trenches),
            Tracers(In->Tracers),Smoke(In->SmokeZones),Phase(In->Phase),Elapsed(In->Elapsed),Peak(In->PeakSuppression),
            Outro(In->BattleOutroWinner),Shots(In->ShotsFired),Advances(In->AdvanceCommands),Message(In->LastMessage)
        {
            for(int32 I=0;I<2;++I){Casualties[I]=B->Casualties[I];Morale[I]=B->Morale[I];Damage[I]=B->DamageDealt[I];Records[I]=B->CombatRecordV65[I];}
            for(AWarfareFrontlineSoldier* U:Units)if(IsValid(U)){Originals.Add({U,U->IsHidden(),U->GetActorEnableCollision()});U->SetActorHiddenInGame(true);U->SetActorEnableCollision(false);}
            Clear();
        }
        void Clear()
        {
            for(auto* W:Walls)if(IsValid(W))W->Destroy();Walls.Reset();
            for(auto* U:Made)if(IsValid(U))U->Destroy();Made.Reset();
            B->Units.Reset();B->Squads.Reset();B->Tracers.Reset();B->SmokeZones.Reset();
            B->Trenches=Trenches;
            B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=100.f;
            FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
        }
        ~FContext()
        {
            Clear();B->Units=Units;B->Squads=Squads;B->Trenches=Trenches;B->Tracers=Tracers;B->SmokeZones=Smoke;
            B->Phase=Phase;B->Elapsed=Elapsed;B->PeakSuppression=Peak;B->BattleOutroWinner=Outro;B->ShotsFired=Shots;B->AdvanceCommands=Advances;B->LastMessage=Message;
            for(int32 I=0;I<2;++I){B->Casualties[I]=Casualties[I];B->Morale[I]=Morale[I];B->DamageDealt[I]=Damage[I];B->CombatRecordV65[I]=Records[I];}
            for(const auto& O:Originals)if(auto* U=O.U.Get()){U->SetActorHiddenInGame(O.Hidden);U->SetActorEnableCollision(O.Collision);}
            FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
        }
    };
    static void Pose(AWarfareFrontlineSoldier* U,float DT)
    {U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();}
    static void Tick(AWarfareFrontlineSoldier* U,float DT)
    {U->Tick(DT);Pose(U,DT);if(auto* G=FrontlineMachinegunsV30::Get(U))G->TickComponent(DT,LEVELTICK_All,nullptr);}
    static FVector Ground(AWarfareFrontlineGameMode* B,FVector P)
    {P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);return P;}
    static AWarfareFrontlineSoldier* Make(FContext& C,int32 Id,int32 Team,int32 Member,FVector Feet,bool Enemy=false,
        EFrontlineRole Role=EFrontlineRole::Lewis)
    {
        auto* U=C.B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return nullptr;
        U->Initialize(C.B,Id,Team,Enemy?EFrontlineRole::Rifle:Role,Member);U->SetActorTickEnabled(false);U->ReturnToIdle();
        U->bFieldEntered=true;U->ShotClock=U->GrenadeClock=10000.f;U->DepartureClock=U->ReactionSeconds=0;
        U->SetActorLocation(Ground(C.B,Feet)+FVector(0,0,88));U->SetActorRotation(FRotator(0,C.B->GetTeamDirection(Team)>0?0.f:180.f,0));
        C.B->Units.Add(U);C.Made.Add(U);Pose(U,0);return U;
    }
    static AActor* Wall(FContext& C,FVector At)
    {
        auto* A=C.B->GetWorld()->SpawnActor<AActor>();if(!A)return nullptr;C.Walls.Add(A);
        auto* Box=NewObject<UBoxComponent>(A);A->SetRootComponent(Box);A->AddInstanceComponent(Box);Box->SetBoxExtent(FVector(180,180,5000));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        Box->RegisterComponent();A->SetActorLocation(At);A->SetActorEnableCollision(false);return A;
    }
    static bool Checkpoint(const FString& User,const FString& Prefix,TSharedRef<FJsonObject> Row)
    {
        FString Text;return FJsonSerializer::Serialize(Row,TJsonWriterFactory<>::Create(&Text))
            &&FFileHelper::SaveStringToFile(Text,*(User/(TEXT("checkpoint_")+Prefix+TEXT(".json"))));
    }
    // These short trajectories deliberately start combat before a quiet-pose
    // prewarm. Positions and visibility collision are fixture inputs; all
    // stance, aiming, recoil, suppression, feed and travel remain production.
    static TSharedRef<FJsonObject> Focused(FContext& C,int32 Team,int32 Context,int32 CaseId,
        TFunctionRef<void(const FString&,bool)> Check)
    {
        auto Row=MakeShared<FJsonObject>();auto* B=C.B;C.Clear();
        const FString Prefix=FString::Printf(TEXT("v8450_t%d_context%d_"),Team,Context);
        Row->SetNumberField(TEXT("team"),Team);Row->SetNumberField(TEXT("context"),Context);Row->SetStringField(TEXT("faction"),B->GetFactionName(Team));
        const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        const bool EmptyFleet=!Fleet||Fleet->GetVehicles().IsEmpty();
        Check(Prefix+TEXT("fleet_empty_no_armour_contact_contamination"),EmptyFleet);
        Row->SetNumberField(TEXT("fleet_count"),Fleet?Fleet->GetVehicles().Num():0);if(!EmptyFleet)return Row;
        const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team);
        const int32 Next=FMath::Clamp(Home+Dir,0,B->Trenches.Num()-1),Id=8450000+CaseId;
        const bool Held=Context==6;
        const bool Geometry=B->Trenches.IsValidIndex(Home)&&B->Trenches.IsValidIndex(Next);
        Check(Prefix+TEXT("actual_operation_geometry_available"),Geometry);if(!Geometry)return Row;
        B->Trenches[Home].Owner=Team;
        FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=EFrontlineRole::Lewis;Q.Slot=1;Q.Trench=Held?Home:INDEX_NONE;
        Q.LastPassedTrench=Home;Q.Destination=Held?Home:Next;Q.bAdvancing=!Held;Q.bHoldPosition=Held;B->Squads.Add(Q);
        const FVector Start((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,0,0);TArray<AWarfareFrontlineSoldier*> Men;
        for(int32 M=0;M<B->GetSquadSize(EFrontlineRole::Lewis,Team);++M)
            if(auto* U=Make(C,Id,Team,M,Start+FVector(-Dir*M*80.f,(M-1)*125.f,0)))
            {B->FindSquad(Id)->Members.Add(U);Men.Add(U);}
        Check(Prefix+TEXT("complete_real_machinegun_section"),Men.Num()==B->GetSquadSize(EFrontlineRole::Lewis,Team)&&Men.Num()>=3);
        if(Men.Num()<3)return Row;
        auto* G=Men[0];auto* Gun=FrontlineMachinegunsV30::Get(G);Check(Prefix+TEXT("actual_supported_component"),Gun&&Gun->HasAssets());if(!Gun)return Row;
        if(Held)for(auto* U:Men)
        {
            FVector P=B->SoldierDestination(U);P.X=FrontlineFieldV32::TrenchCentreX(Home,P.Y)-Dir*100.f;
            U->SetActorLocation(Ground(B,P)+FVector(0,0,88));Pose(U,0);
        }
        const float StopRange=FrontlineWeapons::AdvanceContactRangeFor(G)*B->EncounterVisibilityScale();
        const float FireRange=FrontlineWeapons::ProfileFor(G).FireRangeCm*B->EncounterVisibilityScale();
        TArray<AWarfareFrontlineSoldier*> Enemies;TArray<FVector> TargetPositions;
        const FVector ResidentTarget=Held?B->SoldierDestination(G)+FVector(Dir*850.f,0,0):FVector::ZeroVector;
        for(int32 I=0;I<(Context==5||Context==8?2:1);++I)
        {
            const FVector P=Held?ResidentTarget:G->Feet()+FVector(Dir*650.f,I?230.f:0.f,0);
            Enemies.Add(Make(C,Id+500+I*100,1-Team,0,P,true));TargetPositions.Add(P);
        }
        const bool Spawned=!Enemies.Contains(nullptr);Check(Prefix+TEXT("real_hostile_contacts_spawned"),Spawned);if(!Spawned)return Row;
        // Use full authored magazines. The bounded cases require meaningful
        // combat and target transitions, not two potentially idle feed cycles.
        G->ShotClock=G->TargetReviewClock=0;
        const int32 BeginShots=G->ShotsDischarged,BeginReloads=G->ReloadsCompleted,BeginPacks=Gun->Packs,BeginDeploys=Gun->Deployments;
        AActor* Occluder=Context==4?Wall(C,FMath::Lerp(Gun->TrenchObservationOrigin(),FrontlineMachinegunsV30::SightTarget(Enemies[0]),.5f)):nullptr;
        TArray<TSharedPtr<FJsonValue>> Samples;float Time=0,NextSample=0,StressAt=-1.f,StableAt=-1.f;
        float ClearReadyNoShot=0,MaxClearReadyNoShot=0,ClearSeconds=0;
        int32 Frame=0,Replacements=0,StablePackBase=0,StableDeployBase=0,StablePacks=0,StableDeploys=0;
        int32 InnerFrames=0,OuterFrames=0,HeldOnlyFrames=0,LostStopWithFireFrames=0,OccludedFrames=0,FocusSwitches=0,LastFocus=INDEX_NONE;
        int32 ScoreWinnerSwitches=0,LastScoreWinner=INDEX_NONE,OppositeScoreWinsWithClearSector=0,WrongSectorFocusFrames=0,SupportedSectorShots=0;
        float EffectiveFireSeconds=0;
        bool BurstEdgeExercised=false;int32 BurstEdgeCurrent=INDEX_NONE,BurstEdgeCandidate=INDEX_NONE,BurstEdgeRounds=0,BurstEdgePendingAtRecovery=0,BurstEdgeShots=0,BurstEdgeBlockedShots=0;
        float BurstEdgeAt=-1.f,BurstEdgeRecoveredAt=-1.f,BurstEdgeCurrentScore=0,BurstEdgeCandidateScore=0;
        TArray<FVector> BurstEdgePositions;
        int32 LastShots=G->ShotsDischarged,LastPacks=Gun->Packs,LastDeploys=Gun->Deployments,PostArrivalShotsAt=0;auto LastState=Gun->State;
        bool Finite=true,AllStressSight=true,AllStressRange=true;const float CombatStart=B->Elapsed;
        const auto Place=[&](AWarfareFrontlineSoldier* E,FVector P,bool SeekClear)
        {
            TargetPositions[Enemies.IndexOfByKey(E)]=P;
            for(float Offset:{0.f,35.f,-35.f,80.f,-80.f,150.f,-150.f})
            {
                E->SetActorLocation(Ground(B,P+FVector(0,Offset,0))+FVector(0,0,88));Pose(E,0);
                if(!SeekClear||(B->HasSight(G,E)&&Gun->HasInfantryFiringLine(E)))return true;
            }
            // Failure stays a genuine blocked world ray, never a visibility
            // override or an artificially raised enemy torso.
            return false;
        };
        const float CaseLimit=Context==8?20.f:45.f;
        while(Time<CaseLimit)
        {
            const float Steps[]={1.f/120.f,1.f/60.f,1.f/30.f,.1f};const float DT=Steps[Frame++%4];Time+=DT;B->Elapsed+=DT;
            for(int32 I=0;I<Enemies.Num();++I)if(!IsValid(Enemies[I])||!Enemies[I]->IsAlive())
            {
                if(IsValid(Enemies[I])){B->Units.Remove(Enemies[I]);Enemies[I]->Destroy();}
                ++Replacements;Enemies[I]=Make(C,Id+1000+Replacements,1-Team,0,TargetPositions[I],true);
                if(!Enemies[I]){Finite=false;break;}
            }
            if(!Finite)break;
            if(StressAt<0&&Gun->IsSupported()&&G->ShotsDischarged>BeginShots)StressAt=Time;
            const float Age=StressAt<0?0.f:Time-StressAt;
            if(Occluder)Occluder->SetActorEnableCollision(false);
            bool Positioned=true;
            for(int32 I=0;I<Enemies.Num();++I)
            {
                FVector P=Held?ResidentTarget:G->Feet()+FVector(Dir*650.f,I?230.f:0.f,0);
                if(Context==4&&StressAt>=0)
                {
                    // Most of the time the enemy is inside the old +220 cm
                    // retention band but outside fresh advancing acquisition.
                    float Range=StopRange+110.f+70.f*FMath::Sin(Age*.8f);
                    const float Cycle=FMath::Fmod(Age,12.f);
                    if(Cycle>=9.f&&Cycle<10.5f)Range=StopRange+285.f;
                    if(Cycle>=10.5f&&Cycle<11.5f)Range=StopRange-90.f;
                    Range=FMath::Min(Range,FireRange-45.f);
                    P=G->Feet()+FVector(Dir*Range,0,0);
                }
                if(Context==5||Context==8)
                {
                    // Real positions change score by more than its 280 cm
                    // retarget margin. Suppression comes only from real fire.
                    const float Radius=1900.f+(I?1.f:-1.f)*650.f*FMath::Sin(Time*.65f);
                    const float Degrees=Context==8?10.f:20.f;const float Angle=FMath::DegreesToRadians(I?-Degrees:Degrees);
                    P=G->Feet()+FVector(Dir*Radius*FMath::Cos(Angle),Radius*FMath::Sin(Angle),0);
                }
                if(Context==8&&BurstEdgeExercised)P=BurstEdgePositions[I];
                Positioned&=Place(Enemies[I],P,!Held&&StressAt>=0);
            }
            if(Context==8&&!BurstEdgeExercised&&Time>=12.f&&Gun->IsSupported()&&G->BurstRemaining>0)
            {
                const int32 Current=Enemies.IndexOfByKey(G->FocusTarget.Get()),Candidate=Current==INDEX_NONE?INDEX_NONE:1-Current;
                if(Current!=INDEX_NONE&&Enemies[Current]->IsActiveCombatant()&&Enemies[Candidate]->IsActiveCombatant())
                {
                    const float Yaw=Gun->DescribeTrenchFiringSectorV844()->GetNumberField(TEXT("deployment_yaw"));
                    const float OutYaw=FMath::DegreesToRadians(Yaw+50.f),InYaw=FMath::DegreesToRadians(Yaw);
                    const bool CurrentClear=Place(Enemies[Current],G->Feet()+FVector(FMath::Cos(OutYaw)*1100.f,FMath::Sin(OutYaw)*1100.f,0),true);
                    const bool CandidateClear=Place(Enemies[Candidate],G->Feet()+FVector(FMath::Cos(InYaw)*3000.f,FMath::Sin(InYaw)*3000.f,0),true);
                    const float CurrentScore=FrontlineMachinegunsV30::TargetScoreV828(G,Enemies[Current]),CandidateScore=FrontlineMachinegunsV30::TargetScoreV828(G,Enemies[Candidate]);
                    const bool Outside=!Gun->CoversYaw((Enemies[Current]->Feet()-G->Feet()).Rotation().Yaw);
                    const bool Inside=Gun->CoversYaw((Enemies[Candidate]->Feet()-G->Feet()).Rotation().Yaw);
                    if(CurrentClear&&CandidateClear&&Outside&&Inside&&CurrentScore+280.f<CandidateScore
                        &&FrontlineWeapons::DirectFireRangeDistanceCm(G,Enemies[Candidate]->Feet())<=FireRange)
                    {
                        BurstEdgeExercised=true;BurstEdgeAt=Time;BurstEdgeCurrent=Current;BurstEdgeCandidate=Candidate;BurstEdgeRounds=G->BurstRemaining;
                        BurstEdgeCurrentScore=CurrentScore;BurstEdgeCandidateScore=CandidateScore;
                        for(auto* E:Enemies)BurstEdgePositions.Add(E->Feet());
                    }
                }
            }
            const bool BriefOcclusion=Context==4&&StressAt>=0&&Age>=6.f&&FMath::Fmod(Age,6.f)<.12f;
            if(Occluder)
            {
                Occluder->SetActorLocation(FMath::Lerp(Gun->TrenchObservationOrigin(),FrontlineMachinegunsV30::SightTarget(Enemies[0]),.5f));
                Occluder->SetActorEnableCollision(BriefOcclusion);
            }
            const int32 PreviousBurst=G->BurstRemaining;for(auto* U:Men)Tick(U,DT);
            bool AnyClear=false,MechanicalClear=false;int32 Focus=INDEX_NONE,ScoreWinner=INDEX_NONE;float BestScore=MAX_flt;
            TArray<TSharedPtr<FJsonValue>> Targets;
            for(int32 I=0;I<Enemies.Num();++I)
            {
                auto* E=Enemies[I];const float Direct=FrontlineWeapons::DirectFireRangeDistanceCm(G,E->Feet()),Physical=FVector::Dist2D(G->Feet(),E->Feet());
                const bool Sight=B->HasSight(G,E),Muzzle=Gun->HasInfantryFiringLine(E),Alive=E->IsActiveCombatant();
                const float Yaw=(E->Feet()-G->Feet()).Rotation().Yaw;
                const bool Clear=Alive&&Sight&&Muzzle&&Direct<=FireRange,Covered=Gun->CoversYaw(Yaw);
                AnyClear|=Clear;MechanicalClear|=Clear&&Covered;
                const float Score=FrontlineMachinegunsV30::TargetScoreV828(G,E);
                if(Alive&&Sight&&Direct<=FireRange&&Score<BestScore){BestScore=Score;ScoreWinner=I;}
                if(G->FocusTarget.Get()==E)Focus=I;
                if(Context==4&&StressAt>=0&&Alive)
                {
                    InnerFrames+=Physical<=StopRange;OuterFrames+=Physical>StopRange+220.f;
                    HeldOnlyFrames+=Physical>StopRange&&Physical<=StopRange+220.f;
                    if(!BriefOcclusion){AllStressSight&=Sight;AllStressRange&=Direct<FireRange;}
                    if(BriefOcclusion&&!Sight)++OccludedFrames;
                }
                auto T=MakeShared<FJsonObject>();T->SetNumberField(TEXT("slot"),I);T->SetNumberField(TEXT("instance"),E->SquadId);
                T->SetStringField(TEXT("feet"),E->Feet().ToString());T->SetBoolField(TEXT("alive"),Alive);T->SetBoolField(TEXT("moving"),E->bMoving);
                T->SetNumberField(TEXT("suppression"),E->Suppression);T->SetNumberField(TEXT("selection_score"),Score);
                T->SetNumberField(TEXT("world_yaw"),Yaw);T->SetBoolField(TEXT("current_mechanical_sector"),Covered);
                T->SetNumberField(TEXT("direct_range_cm"),Direct);T->SetNumberField(TEXT("physical_range_cm"),Physical);
                T->SetBoolField(TEXT("observation_clear"),Sight);T->SetBoolField(TEXT("muzzle_clear"),Muzzle);Targets.Add(MakeShared<FJsonValueObject>(T));
            }
            if(Focus!=INDEX_NONE&&LastFocus!=INDEX_NONE&&Focus!=LastFocus)++FocusSwitches;if(Focus!=INDEX_NONE)LastFocus=Focus;
            if(ScoreWinner!=INDEX_NONE&&LastScoreWinner!=INDEX_NONE&&ScoreWinner!=LastScoreWinner)++ScoreWinnerSwitches;
            if(ScoreWinner!=INDEX_NONE)LastScoreWinner=ScoreWinner;
            if(Gun->IsSupported()&&MechanicalClear)
            {
                EffectiveFireSeconds+=DT;
                if(ScoreWinner!=INDEX_NONE&&!Gun->CoversYaw((Enemies[ScoreWinner]->Feet()-G->Feet()).Rotation().Yaw))++OppositeScoreWinsWithClearSector;
                if((Context!=8||!BurstEdgeExercised)&&Focus!=INDEX_NONE&&Enemies[Focus]->IsActiveCombatant()
                    &&!Gun->CoversYaw((Enemies[Focus]->Feet()-G->Feet()).Rotation().Yaw))++WrongSectorFocusFrames;
                if(G->ShotsDischarged>LastShots)SupportedSectorShots+=G->ShotsDischarged-LastShots;
            }
            if(Context==8&&BurstEdgeExercised)
            {
                if(Focus==BurstEdgeCandidate&&Gun->IsSupported()&&Gun->CoversYaw((Enemies[Focus]->Feet()-G->Feet()).Rotation().Yaw))
                {
                    if(BurstEdgeRecoveredAt<0){BurstEdgeRecoveredAt=Time;BurstEdgePendingAtRecovery=PreviousBurst;}
                    BurstEdgeShots+=G->ShotsDischarged-LastShots;
                }
                if(Focus==BurstEdgeCurrent&&!Gun->CoversYaw((Enemies[Focus]->Feet()-G->Feet()).Rotation().Yaw))BurstEdgeBlockedShots+=G->ShotsDischarged-LastShots;
            }
            if(AnyClear)ClearSeconds+=DT;
            if(Context==4&&StressAt>=0&&AnyClear&&!B->AcquireTarget(G,true))++LostStopWithFireFrames;
            if(StableAt<0&&Gun->IsSupported()&&AnyClear&&G->ShotsDischarged>BeginShots&&Time>=2.f)
            {StableAt=Time;StablePackBase=Gun->Packs;StableDeployBase=Gun->Deployments;PostArrivalShotsAt=G->ShotsDischarged;}
            if(StableAt>=0){StablePacks=Gun->Packs-StablePackBase;StableDeploys=Gun->Deployments-StableDeployBase;}
            const bool ClearReady=AnyClear&&Gun->CanFire()&&!G->bMoving&&G->WeaponPhase==EFrontlineWeaponPhase::Ready
                &&G->ShotClock<=0&&G->AimSettleClock<=0&&G->FirePoseClock<=0;
            ClearReadyNoShot=ClearReady&&G->ShotsDischarged==LastShots?ClearReadyNoShot+DT:0;
            MaxClearReadyNoShot=FMath::Max(MaxClearReadyNoShot,ClearReadyNoShot);
            const FVector Feet=G->Feet();bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(G,B->SoldierDestination(G),Reform);
            Finite&=!Feet.ContainsNaN()&&!Goal.ContainsNaN()&&FMath::IsFinite(G->GetMotionSpeed());
            const bool Changed=Gun->State!=LastState||Gun->Packs!=LastPacks||Gun->Deployments!=LastDeploys||G->ShotsDischarged!=LastShots;
            if(Time>=NextSample||Changed||BriefOcclusion)
            {
                auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("seconds"),Time);R->SetNumberField(TEXT("dt"),DT);R->SetNumberField(TEXT("stress_seconds"),Age);
                R->SetStringField(TEXT("feet"),Feet.ToString());R->SetStringField(TEXT("goal"),Goal.ToString());R->SetStringField(TEXT("actual_muzzle"),G->GetMuzzleLocation().ToString());
                R->SetStringField(TEXT("observation_origin"),Gun->TrenchObservationOrigin().ToString());R->SetStringField(TEXT("playing"),GetNameSafe(G->Playing));
                R->SetBoolField(TEXT("wants_movement"),G->bWantedMovement);R->SetBoolField(TEXT("moving"),G->bMoving);R->SetBoolField(TEXT("engaging"),G->bEngaging);
                R->SetBoolField(TEXT("fixture_placed_visible"),Positioned);R->SetBoolField(TEXT("brief_real_occluder"),BriefOcclusion);
                R->SetNumberField(TEXT("focus_slot"),Focus);R->SetNumberField(TEXT("shots"),G->ShotsDischarged-BeginShots);R->SetNumberField(TEXT("rounds"),G->RoundsRemaining);
                if(const auto* S=B->FindSquad(Id)){R->SetNumberField(TEXT("slot"),S->Slot);R->SetNumberField(TEXT("destination"),S->Destination);R->SetNumberField(TEXT("trench"),S->Trench);}
                R->SetStringField(TEXT("pack_cause"),Gun->LastPackReasonV8450);
                R->SetNumberField(TEXT("original_score_winner_slot"),ScoreWinner);R->SetBoolField(TEXT("current_sector_effective_fire_contact"),MechanicalClear);
                R->SetBoolField(TEXT("burst_current_moved_outside_sector"),BurstEdgeExercised);R->SetNumberField(TEXT("burst_remaining"),G->BurstRemaining);
                R->SetNumberField(TEXT("advance_contact_range_cm"),StopRange);R->SetNumberField(TEXT("legacy_retention_boundary_cm"),StopRange+220.f);R->SetNumberField(TEXT("fire_range_cm"),FireRange);
                R->SetArrayField(TEXT("targets"),Targets);R->SetObjectField(TEXT("gun"),Gun->DescribeTrenchFiringSectorV844());Samples.Add(MakeShared<FJsonValueObject>(R));NextSample=Time+.25f;
            }
            LastState=Gun->State;LastShots=G->ShotsDischarged;LastPacks=Gun->Packs;LastDeploys=Gun->Deployments;
            if(Held&&Time>=30.f&&StableAt>=0&&G->ShotsDischarged-PostArrivalShotsAt>=24)break;
        }
        if(Occluder)Occluder->SetActorEnableCollision(false);
        Check(Prefix+TEXT("bounded_real_tick_and_pose_produce_natural_shots"),Finite&&StableAt>=0&&Time<=45.11f&&G->ShotsDischarged-BeginShots>=16&&ClearSeconds>8.f);
        Check(Prefix+TEXT("persistent_fire_contact_has_no_repeated_pack_deploy"),StableAt>=0&&StablePacks<=1&&StableDeploys<=1);
        Check(Prefix+TEXT("clear_ready_weapon_has_no_silent_stall"),MaxClearReadyNoShot<1.f);Check(Prefix+TEXT("no_round_spent_during_reload"),G->ShotsDuringReload==0);
        if(Context==4)
        {
            Check(Prefix+TEXT("legacy_stop_boundaries_and_visible_fire_contact_exercised"),InnerFrames>0&&OuterFrames>0&&HeldOnlyFrames>0&&LostStopWithFireFrames>0);
            Check(Prefix+TEXT("repeated_actual_short_occlusion_exercised"),OccludedFrames>=3);
            Check(Prefix+TEXT("stress_target_stays_in_real_fire_range_and_world_sight"),AllStressSight&&AllStressRange);
        }
        if(Context==8)
        {
            Check(Prefix+TEXT("same_cone_real_priority_changes_keep_one_supported_mount"),ScoreWinnerSwitches>=2&&FocusSwitches>=2&&StablePacks==0&&StableDeploys==0
                &&EffectiveFireSeconds>8.f&&SupportedSectorShots>=16&&WrongSectorFocusFrames==0);
            Check(Prefix+TEXT("actual_burst_current_moved_outside_with_worse_score_clear_inside_candidate"),BurstEdgeExercised&&BurstEdgeRounds>0&&BurstEdgeCurrentScore+280.f<BurstEdgeCandidateScore);
            Check(Prefix+TEXT("unfireable_current_burst_recovers_to_real_inside_candidate"),BurstEdgeRecoveredAt>=BurstEdgeAt&&BurstEdgeRecoveredAt-BurstEdgeAt<2.f
                &&BurstEdgePendingAtRecovery>0&&BurstEdgeShots>=3&&BurstEdgeBlockedShots==0&&StablePacks==0&&StableDeploys==0);
            Row->SetNumberField(TEXT("original_score_winner_switches"),ScoreWinnerSwitches);Row->SetNumberField(TEXT("effective_current_sector_fire_seconds"),EffectiveFireSeconds);
            Row->SetNumberField(TEXT("supported_current_sector_shots"),SupportedSectorShots);Row->SetNumberField(TEXT("wrong_sector_focus_with_clear_current_sector_frames"),WrongSectorFocusFrames);
            Row->SetNumberField(TEXT("burst_current_moved_outside_seconds"),BurstEdgeAt);Row->SetNumberField(TEXT("burst_inside_candidate_recovered_seconds"),BurstEdgeRecoveredAt);
            Row->SetNumberField(TEXT("burst_remaining_at_real_target_move"),BurstEdgeRounds);Row->SetNumberField(TEXT("burst_outside_current_score"),BurstEdgeCurrentScore);
            Row->SetNumberField(TEXT("burst_remaining_at_actual_inside_recovery"),BurstEdgePendingAtRecovery);
            Row->SetNumberField(TEXT("burst_clear_inside_candidate_score"),BurstEdgeCandidateScore);Row->SetNumberField(TEXT("burst_inside_candidate_natural_shots"),BurstEdgeShots);
            Row->SetNumberField(TEXT("burst_outside_current_discharge_attempts"),BurstEdgeBlockedShots);
        }
        if(Context==5)
        {
            Check(Prefix+TEXT("two_real_side_targets_change_original_score_winner"),ScoreWinnerSwitches>=2&&OppositeScoreWinsWithClearSector>0);
            Check(Prefix+TEXT("mounted_clear_sector_retains_effective_fire_without_repacking"),EffectiveFireSeconds>8.f&&SupportedSectorShots>=16
                &&WrongSectorFocusFrames==0&&StablePacks==0&&StableDeploys==0);
            // Preserve the preceding 45-second moving-score stress. Now remove
            // its current cone by a genuine fatal hit, rather than a temporary
            // health/visibility toggle, and prove the other cone is not locked out.
            for(int32 I=0;I<Enemies.Num();++I)if(!Enemies[I]->IsAlive())
            {
                B->Units.Remove(Enemies[I]);Enemies[I]->Destroy();++Replacements;
                Enemies[I]=Make(C,Id+1000+Replacements,1-Team,0,TargetPositions[I],true);
            }
            int32 Mounted=INDEX_NONE;
            for(int32 I=0;I<Enemies.Num();++I)if(IsValid(Enemies[I])&&Enemies[I]->IsActiveCombatant()
                &&Gun->CoversYaw((Enemies[I]->Feet()-G->Feet()).Rotation().Yaw)){Mounted=I;break;}
            const int32 Other=Mounted==INDEX_NONE?INDEX_NONE:1-Mounted;
            const bool Opposite=Other!=INDEX_NONE&&IsValid(Enemies[Other])&&Enemies[Other]->IsActiveCombatant()
                &&!Gun->CoversYaw((Enemies[Other]->Feet()-G->Feet()).Rotation().Yaw)
                &&B->HasSight(G,Enemies[Other])&&Gun->HasInfantryFiringLine(Enemies[Other]);
            Check(Prefix+TEXT("real_current_cone_and_clear_opposite_cone_present_before_fatal_hit"),Mounted!=INDEX_NONE&&Opposite&&Gun->IsSupported());
            const int32 BeforeTurnShots=G->ShotsDischarged,BeforeTurnPacks=Gun->Packs,BeforeTurnDeploys=Gun->Deployments;
            float TurnSeconds=0,FirstOppositeShot=-1.f,TurnReadyNoShot=0,MaxTurnReadyNoShot=0;int32 OppositeShots=0,TurnReplacements=0;
            bool Fatal=false;AWarfareFrontlineSoldier* Removed=Mounted!=INDEX_NONE?Enemies[Mounted]:nullptr;int32 RemovedShots=Removed?Removed->ShotsDischarged:0;
            if(Removed){Removed->ReceiveHit(1.f,0.f,Team);Fatal=!Removed->IsAlive();B->Units.Remove(Removed);}
            Check(Prefix+TEXT("current_cone_removed_only_by_normal_fatal_hit"),Fatal);
            if(Opposite&&Fatal)while(TurnSeconds<30.f)
            {
                const float Steps[]={1.f/120.f,1.f/60.f,1.f/30.f,.1f};const float DT=Steps[Frame++%4];TurnSeconds+=DT;B->Elapsed+=DT;
                auto* E=Enemies[Other];
                if(!E->IsAlive())
                {
                    B->Units.Remove(E);E->Destroy();++Replacements;++TurnReplacements;
                    E=Enemies[Other]=Make(C,Id+1000+Replacements,1-Team,0,TargetPositions[Other],true);
                    if(!E){Finite=false;break;}
                }
                const bool Positioned=Place(E,TargetPositions[Other],true);
                const int32 PreviousShots=G->ShotsDischarged;for(auto* U:Men)Tick(U,DT);
                const bool Covered=Gun->CoversYaw((E->Feet()-G->Feet()).Rotation().Yaw);
                const bool Clear=E->IsActiveCombatant()&&B->HasSight(G,E)&&Gun->HasInfantryFiringLine(E)
                    &&FrontlineWeapons::DirectFireRangeDistanceCm(G,E->Feet())<=FireRange;
                if(G->ShotsDischarged>PreviousShots&&G->FocusTarget.Get()==E&&Covered&&Gun->IsSupported())
                {OppositeShots+=G->ShotsDischarged-PreviousShots;if(FirstOppositeShot<0)FirstOppositeShot=TurnSeconds;}
                const bool Ready=Clear&&Covered&&Gun->CanFire()&&!G->bMoving&&G->WeaponPhase==EFrontlineWeaponPhase::Ready
                    &&G->ShotClock<=0&&G->AimSettleClock<=0&&G->FirePoseClock<=0;
                TurnReadyNoShot=Ready&&G->ShotsDischarged==PreviousShots?TurnReadyNoShot+DT:0;MaxTurnReadyNoShot=FMath::Max(MaxTurnReadyNoShot,TurnReadyNoShot);
                Finite&=!G->Feet().ContainsNaN()&&FMath::IsFinite(G->GetMotionSpeed());
                if(TurnSeconds>=NextSample||G->ShotsDischarged>PreviousShots||Gun->Packs!=LastPacks||Gun->Deployments!=LastDeploys)
                {
                    auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("seconds"),Time+TurnSeconds);R->SetStringField(TEXT("phase"),TEXT("current_cone_fatal_hit_then_opposite_fire"));
                    R->SetNumberField(TEXT("seconds_since_fatal_hit"),TurnSeconds);R->SetNumberField(TEXT("remaining_target_slot"),Other);
                    R->SetStringField(TEXT("feet"),G->Feet().ToString());R->SetStringField(TEXT("hostile_feet"),E->Feet().ToString());
                    R->SetBoolField(TEXT("fixture_placed_visible"),Positioned);R->SetBoolField(TEXT("current_sector_effective_fire_contact"),Clear&&Covered);
                    R->SetNumberField(TEXT("shots"),G->ShotsDischarged-BeginShots);R->SetNumberField(TEXT("packs_since_fatal_hit"),Gun->Packs-BeforeTurnPacks);
                    if(const auto* S=B->FindSquad(Id)){R->SetNumberField(TEXT("slot"),S->Slot);R->SetNumberField(TEXT("destination"),S->Destination);R->SetNumberField(TEXT("trench"),S->Trench);}
                    R->SetStringField(TEXT("pack_cause"),Gun->LastPackReasonV8450);
                    R->SetObjectField(TEXT("gun"),Gun->DescribeTrenchFiringSectorV844());Samples.Add(MakeShared<FJsonValueObject>(R));NextSample=TurnSeconds+.25f;
                }
                LastPacks=Gun->Packs;LastDeploys=Gun->Deployments;
                if(TurnSeconds>=8.f&&OppositeShots>=6&&Gun->IsSupported())break;
            }
            Check(Prefix+TEXT("removed_contact_stays_dead_and_never_fires"),Removed&&!Removed->IsAlive()&&Removed->ShotsDischarged==RemovedShots);
            Check(Prefix+TEXT("empty_current_cone_allows_bounded_natural_opposite_fire"),Finite&&Fatal&&OppositeShots>=6&&FirstOppositeShot>=0&&TurnSeconds<=30.11f);
            Check(Prefix+TEXT("opposite_cone_requires_at_most_one_legitimate_pack_and_deploy"),Fatal&&Gun->Packs-BeforeTurnPacks<=1&&Gun->Deployments-BeforeTurnDeploys<=1);
            Check(Prefix+TEXT("opposite_cone_has_no_permanent_clear_ready_lock"),OppositeShots>=6&&MaxTurnReadyNoShot<1.f&&G->ShotsDuringReload==0);
            Row->SetNumberField(TEXT("pre_fatal_hit_shots"),BeforeTurnShots-BeginShots);Row->SetNumberField(TEXT("original_score_winner_switches"),ScoreWinnerSwitches);
            Row->SetNumberField(TEXT("opposite_score_winner_with_clear_current_sector_frames"),OppositeScoreWinsWithClearSector);
            Row->SetNumberField(TEXT("wrong_sector_focus_with_clear_current_sector_frames"),WrongSectorFocusFrames);Row->SetNumberField(TEXT("supported_current_sector_shots"),SupportedSectorShots);
            Row->SetNumberField(TEXT("effective_current_sector_fire_seconds"),EffectiveFireSeconds);Row->SetNumberField(TEXT("current_cone_fatal_hit_slot"),Mounted);
            Row->SetNumberField(TEXT("opposite_fire_seconds"),TurnSeconds);Row->SetNumberField(TEXT("first_natural_opposite_shot_seconds"),FirstOppositeShot);Row->SetNumberField(TEXT("natural_opposite_shots"),OppositeShots);
            Row->SetNumberField(TEXT("opposite_cone_packs"),Gun->Packs-BeforeTurnPacks);Row->SetNumberField(TEXT("opposite_cone_deployments"),Gun->Deployments-BeforeTurnDeploys);
            Row->SetNumberField(TEXT("opposite_cone_actual_killed_and_replaced_contacts"),TurnReplacements);Row->SetNumberField(TEXT("opposite_cone_max_clear_ready_without_discharge_seconds"),MaxTurnReadyNoShot);
        }
        if(Held)Check(Prefix+TEXT("enemy_present_arrival_settles_and_continues_trench_fire"),Gun->IsTrench()&&StableAt>=0&&G->ShotsDischarged-PostArrivalShotsAt>=24);
        Row->SetNumberField(TEXT("combat_seconds"),B->Elapsed-CombatStart);Row->SetNumberField(TEXT("first_supported_fire_seconds"),StressAt);Row->SetNumberField(TEXT("stable_start_seconds"),StableAt);
        Row->SetNumberField(TEXT("shots"),G->ShotsDischarged-BeginShots);Row->SetNumberField(TEXT("reloads"),G->ReloadsCompleted-BeginReloads);Row->SetNumberField(TEXT("total_packs"),Gun->Packs-BeginPacks);Row->SetNumberField(TEXT("total_deployments"),Gun->Deployments-BeginDeploys);
        Row->SetNumberField(TEXT("stable_packs"),StablePacks);Row->SetNumberField(TEXT("stable_deployments"),StableDeploys);Row->SetNumberField(TEXT("focus_switches"),FocusSwitches);Row->SetNumberField(TEXT("actual_killed_and_replaced_contacts"),Replacements);
        Row->SetNumberField(TEXT("inside_fresh_stop_frames"),InnerFrames);Row->SetNumberField(TEXT("outside_retention_frames"),OuterFrames);Row->SetNumberField(TEXT("retention_only_frames"),HeldOnlyFrames);Row->SetNumberField(TEXT("lost_stop_with_clear_fire_frames"),LostStopWithFireFrames);Row->SetNumberField(TEXT("brief_occluded_frames"),OccludedFrames);
        Row->SetNumberField(TEXT("clear_fire_seconds"),ClearSeconds);Row->SetNumberField(TEXT("max_clear_ready_without_discharge_seconds"),MaxClearReadyNoShot);Row->SetArrayField(TEXT("timeline"),Samples);return Row;
    }
    // Unlike the stance-only fixtures, this case executes the real section
    // scheduler before each unit Tick. The two actual resident sections reserve
    // lanes until one leaves through the public order, while the incoming gun
    // holds real contact midway to the receiving trench.
    static TSharedRef<FJsonObject> Scheduled(FContext& C,int32 Team,int32 CaseId,TFunctionRef<void(const FString&,bool)> Check)
    {
        auto Row=MakeShared<FJsonObject>();auto* B=C.B;C.Clear();const int32 Context=7;
        const FString Prefix=FString::Printf(TEXT("v8450_t%d_context7_"),Team);
        const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team),Next=FMath::Clamp(Home+Dir,0,B->Trenches.Num()-1),Id=8450000+CaseId;
        Row->SetNumberField(TEXT("team"),Team);Row->SetNumberField(TEXT("context"),Context);Row->SetStringField(TEXT("faction"),B->GetFactionName(Team));
        const bool Geometry=B->Trenches.IsValidIndex(Home)&&B->Trenches.IsValidIndex(Next)&&Home!=Next;
        Check(Prefix+TEXT("real_scheduler_receiving_trench_available"),Geometry);if(!Geometry)return Row;
        B->Trenches[Home].Owner=B->Trenches[Next].Owner=Team;B->Trenches[Next].bBypass=false;
        TArray<AWarfareFrontlineSoldier*> Residents,Men;TArray<int32> ResidentIds;
        for(int32 Slot=0;Slot<2;++Slot)
        {
            const int32 ResidentId=Id+100+Slot;ResidentIds.Add(ResidentId);
            FFrontlineSquad S;S.Id=ResidentId;S.Team=Team;S.Role=EFrontlineRole::Rifle;S.Slot=Slot;
            S.Trench=S.Destination=S.LastPassedTrench=Next;S.bHoldPosition=true;B->Squads.Add(S);
            for(int32 M=0;M<B->GetSquadSize(EFrontlineRole::Rifle,Team);++M)
                if(auto* U=Make(C,ResidentId,Team,M,FVector(B->Trenches[Next].X,(Slot-1)*630.f,0),false,EFrontlineRole::Rifle))
                {B->FindSquad(ResidentId)->Members.Add(U);Residents.Add(U);}
        }
        for(auto* U:Residents){U->SetActorLocation(Ground(B,B->SoldierDestination(U))+FVector(0,0,88));Pose(U,0);}
        const bool CompleteResidents=Residents.Num()==2*B->GetSquadSize(EFrontlineRole::Rifle,Team);
        Check(Prefix+TEXT("two_complete_real_resident_sections_reserve_slots_zero_and_one"),CompleteResidents);if(!CompleteResidents)return Row;
        FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=EFrontlineRole::Lewis;Q.Slot=2;Q.Trench=INDEX_NONE;
        Q.Destination=Next;Q.LastPassedTrench=Home;Q.bAdvancing=true;B->Squads.Add(Q);
        const FVector Start((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,630.f,0);
        for(int32 M=0;M<B->GetSquadSize(EFrontlineRole::Lewis,Team);++M)
            if(auto* U=Make(C,Id,Team,M,Start+FVector(-Dir*M*80.f,(M-1)*125.f,0)))
            {B->FindSquad(Id)->Members.Add(U);Men.Add(U);}
        Check(Prefix+TEXT("complete_real_advancing_machinegun_section"),Men.Num()==B->GetSquadSize(EFrontlineRole::Lewis,Team)&&Men.Num()>=3);if(Men.Num()<3)return Row;
        auto* G=Men[0];auto* Gun=FrontlineMachinegunsV30::Get(G);Check(Prefix+TEXT("actual_supported_component"),Gun&&Gun->HasAssets());if(!Gun)return Row;
        const FVector TargetBase=G->Feet()+FVector(Dir*650.f,0,0);auto* E=Make(C,Id+500,1-Team,0,TargetBase,true);
        Check(Prefix+TEXT("real_hostile_contact_spawned"),E!=nullptr);if(!E)return Row;
        G->ShotClock=G->TargetReviewClock=0;AActor* Occluder=Wall(C,FMath::Lerp(Gun->TrenchObservationOrigin(),FrontlineMachinegunsV30::SightTarget(E),.5f));
        const int32 BeginShots=G->ShotsDischarged,BeginReloads=G->ReloadsCompleted,BeginPacks=Gun->Packs,BeginDeploys=Gun->Deployments;
        int32 Frame=0,SchedulerTicks=0,Replacements=0,OccludedFrames=0,StablePackBase=0,StableDeployBase=0,StablePacks=0,StableDeploys=0;
        int32 SlotChanges=0,DestinationChanges=0,TrenchChanges=0,LastSlot=2,LastDestination=Next,LastTrench=INDEX_NONE;
        int32 LegalReservationChanges=0,UnexpectedReservationChanges=0,FreedResidentSlot=INDEX_NONE,ResidentTrenchAfterOrder=INDEX_NONE,ResidentDestinationAfterOrder=INDEX_NONE;
        int32 LastShots=G->ShotsDischarged,LastPacks=Gun->Packs,LastDeploys=Gun->Deployments,AfterOcclusionShots=0,ClearFrames=0;
        float Time=0,NextSample=0,StableAt=-1.f,ClearSeconds=0,ReadyNoShot=0,MaxReadyNoShot=0,StableTravel=0;
        bool Finite=true,DepartureAttempted=false,DepartureOrdered=false,DepartureVacatedAtOrder=false,ExplicitFatal=false,AllStableClear=true;
        float DepartureOrderAt=-1.f;
        FVector LastFeet=G->Feet(),ResidentDepartureFeet=Residents[0]->Feet();const float CombatStart=B->Elapsed;
        TArray<TSharedPtr<FJsonValue>> Samples;
        while(Time<65.f)
        {
            const float Steps[]={1.f/120.f,1.f/60.f,1.f/30.f,.1f};const float DT=Steps[Frame++%4];Time+=DT;B->Elapsed+=DT;
            if(!E->IsAlive())
            {
                B->Units.Remove(E);E->Destroy();++Replacements;E=Make(C,Id+1000+Replacements,1-Team,0,TargetBase,true);
                if(!E){Finite=false;break;}
            }
            if(!ExplicitFatal&&Time>=26.f)
            {
                E->ReceiveHit(1.f,0.f,Team);ExplicitFatal=!E->IsAlive();B->Units.Remove(E);E->Destroy();++Replacements;
                E=Make(C,Id+1000+Replacements,1-Team,0,TargetBase,true);if(!E){Finite=false;break;}
            }
            E->SetActorLocation(Ground(B,TargetBase+FVector(Dir*25.f*FMath::Sin(Time*.75f),65.f*FMath::Sin(Time*.65f),0))+FVector(0,0,88));Pose(E,0);
            const bool BriefOcclusion=Time>=8.f&&FMath::Fmod(Time-8.f,12.f)<.12f;
            if(Occluder){Occluder->SetActorLocation(FMath::Lerp(Gun->TrenchObservationOrigin(),FrontlineMachinegunsV30::SightTarget(E),.5f));Occluder->SetActorEnableCollision(BriefOcclusion);}
            if(!DepartureAttempted&&Time>=14.f)
            {
                DepartureAttempted=true;DepartureOrderAt=Time;ResidentDepartureFeet=Residents[0]->Feet();
                if(const auto* Resident=B->FindSquad(ResidentIds[0]))FreedResidentSlot=Resident->Slot;
                DepartureOrdered=B->AdvanceSquad(Next,ResidentIds[0],Team);
                if(const auto* Resident=B->FindSquad(ResidentIds[0]))
                {
                    ResidentTrenchAfterOrder=Resident->Trench;ResidentDestinationAfterOrder=Resident->Destination;
                    DepartureVacatedAtOrder=DepartureOrdered&&Resident->Trench==INDEX_NONE&&Resident->bAdvancing&&Resident->LastPassedTrench==Next;
                }
            }
            B->UpdateSquads();++SchedulerTicks;for(auto* U:Residents)Tick(U,DT);for(auto* U:Men)Tick(U,DT);
            const auto* S=B->FindSquad(Id);if(!S){Finite=false;break;}
            const bool Sight=B->HasSight(G,E),Muzzle=Gun->HasInfantryFiringLine(E),Alive=E->IsActiveCombatant();
            const bool Clear=Alive&&Sight&&Muzzle&&FrontlineWeapons::DirectFireRangeDistanceCm(G,E->Feet())<=FrontlineWeapons::ProfileFor(G).FireRangeCm*B->EncounterVisibilityScale();
            const bool Effective=Clear&&Gun->CoversYaw((E->Feet()-G->Feet()).Rotation().Yaw);
            if(Clear){ClearSeconds+=DT;++ClearFrames;}if(BriefOcclusion&&!Sight)++OccludedFrames;
            if(StableAt<0&&Gun->IsSupported()&&G->ShotsDischarged>BeginShots&&Time>=2.f)
            {StableAt=Time;StablePackBase=Gun->Packs;StableDeployBase=Gun->Deployments;LastSlot=S->Slot;LastDestination=S->Destination;LastTrench=S->Trench;}
            if(StableAt>=0)
            {
                SlotChanges+=S->Slot!=LastSlot;DestinationChanges+=S->Destination!=LastDestination;TrenchChanges+=S->Trench!=LastTrench;
                if(S->Slot!=LastSlot)
                {
                    const bool Legal=DepartureVacatedAtOrder&&Time>=DepartureOrderAt&&LastSlot==2&&S->Slot==FreedResidentSlot
                        &&LastDestination==Next&&S->Destination==Next&&LastTrench==INDEX_NONE&&S->Trench==INDEX_NONE;
                    if(Legal)++LegalReservationChanges;else ++UnexpectedReservationChanges;
                }
                StablePacks=Gun->Packs-StablePackBase;StableDeploys=Gun->Deployments-StableDeployBase;StableTravel+=FVector::Dist2D(G->Feet(),LastFeet);
                if(Alive&&!BriefOcclusion)AllStableClear&=Sight&&Muzzle&&Clear;
                if(Time>=8.12f&&G->ShotsDischarged>LastShots)AfterOcclusionShots+=G->ShotsDischarged-LastShots;
            }
            const bool Ready=Effective&&Gun->CanFire()&&!G->bMoving&&G->WeaponPhase==EFrontlineWeaponPhase::Ready
                &&G->ShotClock<=0&&G->AimSettleClock<=0&&G->FirePoseClock<=0;
            ReadyNoShot=Ready&&G->ShotsDischarged==LastShots?ReadyNoShot+DT:0;MaxReadyNoShot=FMath::Max(MaxReadyNoShot,ReadyNoShot);
            bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(G,B->SoldierDestination(G),Reform);
            Finite&=!G->Feet().ContainsNaN()&&!Goal.ContainsNaN()&&FMath::IsFinite(G->GetMotionSpeed())&&B->Phase==EFrontlinePhase::Fighting;
            const bool Changed=S->Slot!=LastSlot||S->Destination!=LastDestination||S->Trench!=LastTrench||Gun->Packs!=LastPacks||Gun->Deployments!=LastDeploys||G->ShotsDischarged!=LastShots;
            if(Time>=NextSample||Changed||BriefOcclusion)
            {
                auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("seconds"),Time);R->SetNumberField(TEXT("dt"),DT);R->SetNumberField(TEXT("scheduler_ticks"),SchedulerTicks);
                R->SetNumberField(TEXT("slot"),S->Slot);R->SetNumberField(TEXT("destination"),S->Destination);R->SetNumberField(TEXT("trench"),S->Trench);R->SetNumberField(TEXT("last_passed_trench"),S->LastPassedTrench);
                R->SetStringField(TEXT("pack_cause"),Gun->LastPackReasonV8450);R->SetStringField(TEXT("feet"),G->Feet().ToString());R->SetStringField(TEXT("goal"),Goal.ToString());
                R->SetBoolField(TEXT("advancing"),S->bAdvancing);R->SetBoolField(TEXT("engaging"),G->bEngaging);R->SetBoolField(TEXT("wants_movement"),G->bWantedMovement);
                R->SetBoolField(TEXT("brief_real_occluder"),BriefOcclusion);R->SetBoolField(TEXT("observation_clear"),Sight);R->SetBoolField(TEXT("muzzle_clear"),Muzzle);R->SetBoolField(TEXT("current_sector_effective_fire_contact"),Effective);
                R->SetBoolField(TEXT("resident_public_departure_ordered"),DepartureOrdered);R->SetNumberField(TEXT("departing_resident_forward_cm"),(Residents[0]->Feet().X-ResidentDepartureFeet.X)*Dir);
                R->SetBoolField(TEXT("resident_vacated_at_public_order"),DepartureVacatedAtOrder);R->SetNumberField(TEXT("freed_resident_slot"),FreedResidentSlot);
                if(const auto* Resident=B->FindSquad(ResidentIds[0])){R->SetNumberField(TEXT("departing_resident_trench"),Resident->Trench);R->SetNumberField(TEXT("departing_resident_destination"),Resident->Destination);}
                R->SetNumberField(TEXT("shots"),G->ShotsDischarged-BeginShots);R->SetNumberField(TEXT("rounds"),G->RoundsRemaining);R->SetNumberField(TEXT("contact_instance"),E->SquadId);
                R->SetObjectField(TEXT("gun"),Gun->DescribeTrenchFiringSectorV844());Samples.Add(MakeShared<FJsonValueObject>(R));NextSample=Time+.25f;
            }
            LastSlot=S->Slot;LastDestination=S->Destination;LastTrench=S->Trench;LastFeet=G->Feet();LastShots=G->ShotsDischarged;LastPacks=Gun->Packs;LastDeploys=Gun->Deployments;
        }
        if(Occluder)Occluder->SetActorEnableCollision(false);
        const auto* S=B->FindSquad(Id);const float ResidentForward=(Residents[0]->Feet().X-ResidentDepartureFeet.X)*Dir;
        Check(Prefix+TEXT("real_scheduler_and_contact_produce_bounded_natural_fire"),Finite&&StableAt>=0&&SchedulerTicks==Frame&&Time<=65.11f&&G->ShotsDischarged-BeginShots>=16&&ClearSeconds>8.f);
        Check(Prefix+TEXT("public_resident_departure_really_vacates_and_moves"),DepartureVacatedAtOrder&&ResidentForward>160.f);
        Check(Prefix+TEXT("public_vacancy_reassigns_lane_without_moving_supported_gun"),S&&S->Slot==FreedResidentSlot&&S->Destination==Next&&S->Trench==INDEX_NONE
            &&LegalReservationChanges>=1&&UnexpectedReservationChanges==0&&DestinationChanges==0&&TrenchChanges==0
            &&StableTravel<=.1f&&StablePacks==0&&StableDeploys==0);
        Check(Prefix+TEXT("resident_departure_and_replaced_contact_do_not_repeat_pack_deploy"),StableAt>=0&&StablePacks==0&&StableDeploys==0);
        Check(Prefix+TEXT("real_short_occlusion_and_normal_contact_replacement_are_exercised"),OccludedFrames>=3&&ExplicitFatal&&Replacements>0&&AfterOcclusionShots>=16);
        Check(Prefix+TEXT("physical_clear_contact_does_not_silently_stall_or_spend_reload_round"),AllStableClear&&ClearFrames>0&&MaxReadyNoShot<1.f&&G->ShotsDuringReload==0);
        Row->SetNumberField(TEXT("stable_slot_changes"),SlotChanges);Row->SetNumberField(TEXT("stable_destination_changes"),DestinationChanges);Row->SetNumberField(TEXT("stable_trench_changes"),TrenchChanges);
        Row->SetNumberField(TEXT("legal_public_vacancy_reservation_changes"),LegalReservationChanges);Row->SetNumberField(TEXT("unexpected_reservation_changes"),UnexpectedReservationChanges);
        Row->SetBoolField(TEXT("resident_vacated_at_public_order"),DepartureVacatedAtOrder);Row->SetNumberField(TEXT("resident_public_departure_order_seconds"),DepartureOrderAt);
        Row->SetNumberField(TEXT("freed_resident_slot"),FreedResidentSlot);Row->SetNumberField(TEXT("resident_trench_immediately_after_order"),ResidentTrenchAfterOrder);
        Row->SetNumberField(TEXT("resident_destination_immediately_after_order"),ResidentDestinationAfterOrder);
        if(const auto* Resident=B->FindSquad(ResidentIds[0]))Row->SetNumberField(TEXT("resident_trench_at_contact_phase_end"),Resident->Trench);
        Row->SetStringField(TEXT("scheduler_assertion_contract"),TEXT("Public Advance immediately vacates its original trench; a later real arrival is legal. OpenTrenchSlot may reassign the incoming lane from 2 to the freed resident slot after that public vacancy. The contact destination/trench and supported gun feet must remain stable, with no pack/deploy, unexpected lane changes or synthetic follow-up orders."));
        Row->SetNumberField(TEXT("stable_packs"),StablePacks);Row->SetNumberField(TEXT("stable_deployments"),StableDeploys);Row->SetNumberField(TEXT("stable_travel_cm"),StableTravel);
        Row->SetNumberField(TEXT("resident_public_departure_forward_cm"),ResidentForward);Row->SetNumberField(TEXT("brief_occluded_frames"),OccludedFrames);Row->SetNumberField(TEXT("post_occlusion_shots"),AfterOcclusionShots);
        Row->SetNumberField(TEXT("actual_killed_and_replaced_contacts"),Replacements);Row->SetNumberField(TEXT("clear_fire_seconds"),ClearSeconds);Row->SetNumberField(TEXT("max_clear_ready_without_discharge_seconds"),MaxReadyNoShot);
        // With contact truly gone the existing advancing order must release the
        // station and resume travel. No synthetic squad order is injected.
        const FVector BeforeDeparture=G->Feet();const int32 BeforeDeparturePacks=Gun->Packs;
        E->ReceiveHit(1.f,0.f,Team);const bool FinalFatal=!E->IsAlive();B->Units.Remove(E);
        for(int32 I=0;I<320;++I)
        {
            B->Elapsed+=.05f;B->UpdateSquads();++SchedulerTicks;for(auto* U:Residents)Tick(U,.05f);for(auto* U:Men)Tick(U,.05f);
            if(I%5==0||Gun->Packs!=LastPacks||Gun->Deployments!=LastDeploys)
            {
                auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("seconds"),Time+(I+1)*.05f);R->SetStringField(TEXT("phase"),TEXT("normal_fatal_contact_loss_existing_advance"));
                if(const auto* Current=B->FindSquad(Id)){R->SetNumberField(TEXT("slot"),Current->Slot);R->SetNumberField(TEXT("destination"),Current->Destination);R->SetNumberField(TEXT("trench"),Current->Trench);}
                R->SetStringField(TEXT("pack_cause"),Gun->LastPackReasonV8450);R->SetStringField(TEXT("feet"),G->Feet().ToString());R->SetBoolField(TEXT("wants_movement"),G->bWantedMovement);
                R->SetObjectField(TEXT("gun"),Gun->DescribeTrenchFiringSectorV844());Samples.Add(MakeShared<FJsonValueObject>(R));
            }
            LastPacks=Gun->Packs;LastDeploys=Gun->Deployments;
        }
        const float Forward=(G->Feet().X-BeforeDeparture.X)*Dir;
        Check(Prefix+TEXT("real_contact_loss_releases_existing_advance_order"),FinalFatal&&Forward>160.f&&Gun->Packs-BeforeDeparturePacks<=1);
        Row->SetNumberField(TEXT("contact_loss_forward_cm"),Forward);Row->SetNumberField(TEXT("contact_loss_packs"),Gun->Packs-BeforeDeparturePacks);
        Row->SetNumberField(TEXT("scheduler_ticks"),SchedulerTicks);Row->SetNumberField(TEXT("combat_seconds"),B->Elapsed-CombatStart);Row->SetNumberField(TEXT("stable_start_seconds"),StableAt);
        Row->SetNumberField(TEXT("shots"),G->ShotsDischarged-BeginShots);Row->SetNumberField(TEXT("reloads"),G->ReloadsCompleted-BeginReloads);Row->SetNumberField(TEXT("total_packs"),Gun->Packs-BeginPacks);Row->SetNumberField(TEXT("total_deployments"),Gun->Deployments-BeginDeploys);
        Row->SetArrayField(TEXT("timeline"),Samples);return Row;
    }
    static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> External)
    {
        auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
        const auto Check=[&](const FString& N,bool P){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),N);R->SetBoolField(TEXT("passed"),P);Checks.Add(MakeShared<FJsonValueObject>(R));Good&=P;External(N,P);};
        FString User;const bool Allowed=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV8450Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User);
        Check(TEXT("v8450_mg_disposable_uk_de_only"),Allowed);
        if(!Allowed){J->SetBoolField(TEXT("success"),false);J->SetArrayField(TEXT("checks"),Checks);return J;}
        const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());const bool EmptyFleet=!Fleet||Fleet->GetVehicles().IsEmpty();
        Check(TEXT("v8450_mg_fleet_empty_no_hidden_armour_contact"),EmptyFleet);
        if(!EmptyFleet){J->SetBoolField(TEXT("success"),false);J->SetArrayField(TEXT("checks"),Checks);return J;}
        FContext C(B);int32 CaseId=0;
        for(int32 Team:{0,1})for(int32 Context:{0,1,2,3})
        {
            C.Clear();const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team);
            const bool Held=Context==1||Context==2;const int32 Trench=Context==2?FrontlineBattlefield::CentreTrench:Home;
            const int32 Next=FMath::Clamp(Home+Dir,0,B->Trenches.Num()-1),Id=8450000+CaseId++;
            const FString Prefix=FString::Printf(TEXT("v8450_t%d_context%d_"),Team,Context);
            Check(Prefix+TEXT("physical_trench_available"),B->Trenches.IsValidIndex(Trench)&&B->Trenches.IsValidIndex(Next));
            if(!B->Trenches.IsValidIndex(Trench)||!B->Trenches.IsValidIndex(Next))continue;
            B->Trenches[Trench].Owner=Team;
            FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=EFrontlineRole::Lewis;Q.Slot=Context==2?2:0;
            Q.Trench=Held?Trench:INDEX_NONE;Q.LastPassedTrench=Home;Q.Destination=Held?Trench:Next;Q.bAdvancing=!Held;Q.bHoldPosition=Held;
            B->Squads.Add(Q);TArray<AWarfareFrontlineSoldier*> Men;
            FVector Start((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,0,0);
            if(Context==3)Start.X=B->Trenches[Home].X+Dir*145.f;
            for(int32 M=0;M<B->GetSquadSize(EFrontlineRole::Lewis,Team);++M)
            {
                auto* U=Make(C,Id,Team,M,Start+FVector(-Dir*M*80.f,(M-1)*125.f,0));if(!U)continue;
                B->FindSquad(Id)->Members.Add(U);Men.Add(U);
            }
            Check(Prefix+TEXT("complete_real_machinegun_section"),Men.Num()==B->GetSquadSize(EFrontlineRole::Lewis,Team)&&Men.Num()>=3);
            if(Men.Num()<3)continue;
            auto* G=Men[0];auto* Gun=FrontlineMachinegunsV30::Get(G);Check(Prefix+TEXT("actual_supported_component"),Gun&&Gun->HasAssets());if(!Gun)continue;
            if(Held)
            {
                for(auto* U:Men){FVector P=B->SoldierDestination(U);P.X=FrontlineFieldV32::TrenchCentreX(Trench,P.Y)-Dir*100.f;U->SetActorLocation(Ground(B,P)+FVector(0,0,88));}
                for(int32 I=0;I<240;++I){B->Elapsed+=.05f;for(auto* U:Men)Tick(U,.05f);}
            }
            const float StopRange=FrontlineWeapons::AdvanceContactRangeFor(G)*B->EncounterVisibilityScale();
            const float FireRange=FrontlineWeapons::ProfileFor(G).FireRangeCm*B->EncounterVisibilityScale();
            FVector TargetBase=G->Feet()+FVector(Dir*(Context==3?StopRange*.94f:650.f),Context==3?StopRange*.45f:0.f,0);
            auto* E=Make(C,Id+500,1-Team,0,TargetBase,true);Check(Prefix+TEXT("real_hostile_contact_spawned"),E!=nullptr);if(!E)continue;
            if(Held)
            {
                bool Clear=false;
                for(float X:{650.f,850.f,1100.f,1400.f})
                {
                    if(Clear)break;
                    for(float Y:{0.f,160.f,-160.f,350.f,-350.f,600.f,-600.f})
                    {
                        E->SetActorLocation(Ground(B,G->Feet()+FVector(Dir*X,Y,0))+FVector(0,0,88));Pose(E,0);
                        if(B->HasSight(G,E)&&Gun->HasInfantryFiringLine(E)){Clear=true;TargetBase=E->Feet();break;}
                    }
                }
                Check(Prefix+TEXT("real_initial_observation_and_muzzle_clear"),Clear);
            }
            // One partly spent initial magazine is a fixture starting state.
            // Subsequent feed, full magazines, burst/recoil and both reloads
            // are owned entirely by ordinary production Tick.
            G->RoundsRemaining=2;G->ShotClock=0;G->TargetReviewClock=0;
            const int32 BeginShots=G->ShotsDischarged,BeginReloads=G->ReloadsCompleted,BeginPacks=Gun->Packs,BeginDeploys=Gun->Deployments;
            AActor* Occluder=Wall(C,FMath::Lerp(Gun->TrenchObservationOrigin(),FrontlineMachinegunsV30::SightTarget(E),.5f));
            TArray<TSharedPtr<FJsonValue>> Samples;float Time=0,NextSample=0,ClearReadyNoShot=0,MaxClearReadyNoShot=0;
            float MaxIntrusion=0,MaxStep=0,LateTravel=0;int32 Frame=0,DeadContacts=0,StablePacks=0,StableDeploys=0,RangeMismatchFrames=0,OccludedFrames=0;
            int32 LastShots=G->ShotsDischarged,LastPacks=Gun->Packs,LastDeploys=Gun->Deployments;auto LastState=Gun->State;
            FVector LastFeet=G->Feet(),LastGoal=LastFeet;bool Finite=true,InitialStableRecorded=false;
            const float CombatStart=B->Elapsed;int32 InitialStablePacks=0,InitialStableDeploys=0;
            // Two complete natural feed cycles, bounded at 180 simulation s.
            // Every sample is UnitTick -> evaluated Pose -> component support.
            while(Time<180.f&&(Time<32.f||G->ReloadsCompleted-BeginReloads<2))
            {
                const float Steps[]={1.f/120.f,1.f/60.f,1.f/30.f,.1f};const float DT=Steps[Frame++%4];Time+=DT;B->Elapsed+=DT;
                if(!IsValid(E)||!E->IsAlive())
                {
                    if(IsValid(E)){B->Units.Remove(E);E->Destroy();}++DeadContacts;
                    E=Make(C,Id+500+DeadContacts,1-Team,0,TargetBase,true);if(!E){Finite=false;break;}
                }
                FVector Target=TargetBase;
                Target.X+=Dir*25.f*FMath::Sin(Time*.75f);Target.Y+=(Context==3?330.f:65.f)*FMath::Sin(Time*.65f);
                E->SetActorLocation(Ground(B,Target)+FVector(0,0,88));Pose(E,0);
                const bool BriefOcclusion=Time>=12.f&&Time<12.12f;
                if(Occluder){Occluder->SetActorLocation(FMath::Lerp(Gun->TrenchObservationOrigin(),FrontlineMachinegunsV30::SightTarget(E),.5f));Occluder->SetActorEnableCollision(BriefOcclusion);}
                if(Time-DT<16.f&&Time>=16.f)G->ReceiveHit(0.f,.7f,1-Team);
                for(auto* U:Men)Tick(U,DT);
                const FVector Feet=G->Feet();bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(G,B->SoldierDestination(G),Reform);
                const float RangeDistance=FrontlineWeapons::DirectFireRangeDistanceCm(G,E->Feet());
                const float PhysicalDistance=FVector::Dist2D(G->Feet(),E->Feet());const bool Sight=B->HasSight(G,E),Muzzle=Gun->HasInfantryFiringLine(E);
                if(RangeDistance<FireRange&&PhysicalDistance>StopRange&&Sight)++RangeMismatchFrames;
                if(BriefOcclusion&&!Sight)++OccludedFrames;
                Finite&=!Feet.ContainsNaN()&&!Goal.ContainsNaN()&&FMath::IsFinite(G->GetMotionSpeed());
                MaxIntrusion=FMath::Max(MaxIntrusion,FrontlineInfantryV45::BankIntrusion(G,Feet));
                MaxStep=FMath::Max(MaxStep,float(FVector::Dist2D(Feet,LastFeet)));
                if(Time>=8.f&&!InitialStableRecorded){InitialStableRecorded=true;InitialStablePacks=Gun->Packs;InitialStableDeploys=Gun->Deployments;}
                if(Time>=8.f)
                {
                    LateTravel+=FVector::Dist2D(Feet,LastFeet);StablePacks=Gun->Packs-InitialStablePacks;StableDeploys=Gun->Deployments-InitialStableDeploys;
                }
                const bool ClearReady=Sight&&Muzzle&&RangeDistance<FireRange&&Gun->CanFire()&&!G->bMoving
                    &&G->WeaponPhase==EFrontlineWeaponPhase::Ready&&G->ShotClock<=0&&G->AimSettleClock<=0&&G->FirePoseClock<=0;
                ClearReadyNoShot=ClearReady&&G->ShotsDischarged==LastShots?ClearReadyNoShot+DT:0;
                MaxClearReadyNoShot=FMath::Max(MaxClearReadyNoShot,ClearReadyNoShot);
                const bool Changed=Gun->State!=LastState||Gun->Packs!=LastPacks||Gun->Deployments!=LastDeploys
                    ||FVector::DistSquared2D(Goal,LastGoal)>FMath::Square(5.f);
                if(Time>=NextSample||Changed)
                {
                    auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("seconds"),Time);R->SetNumberField(TEXT("dt"),DT);R->SetNumberField(TEXT("member"),G->MemberIndex);
                    R->SetStringField(TEXT("feet"),Feet.ToString());R->SetStringField(TEXT("goal"),Goal.ToString());R->SetNumberField(TEXT("goal_error_cm"),FVector::Dist2D(Feet,Goal));
                    R->SetBoolField(TEXT("wants_movement"),G->bWantedMovement);R->SetBoolField(TEXT("at_destination"),G->bAtDestinationV43);R->SetBoolField(TEXT("moving"),G->bMoving);R->SetBoolField(TEXT("engaging"),G->bEngaging);R->SetBoolField(TEXT("crew_reform"),Reform);
                    R->SetNumberField(TEXT("motion_speed"),G->GetMotionSpeed());R->SetStringField(TEXT("playing"),GetNameSafe(G->Playing));R->SetNumberField(TEXT("shots"),G->ShotsDischarged-BeginShots);R->SetNumberField(TEXT("rounds"),G->RoundsRemaining);R->SetNumberField(TEXT("reloads_completed"),G->ReloadsCompleted-BeginReloads);
                    R->SetStringField(TEXT("hostile_feet"),E->Feet().ToString());R->SetNumberField(TEXT("hostile_instance"),E->SquadId);R->SetNumberField(TEXT("direct_range_distance_cm"),RangeDistance);R->SetNumberField(TEXT("physical_distance_cm"),PhysicalDistance);R->SetNumberField(TEXT("advance_contact_range_cm"),StopRange);R->SetNumberField(TEXT("fire_range_cm"),FireRange);
                    R->SetBoolField(TEXT("observation_clear"),Sight);R->SetBoolField(TEXT("muzzle_clear"),Muzzle);R->SetBoolField(TEXT("brief_real_occluder"),BriefOcclusion);R->SetObjectField(TEXT("gun"),Gun->DescribeTrenchFiringSectorV844());Samples.Add(MakeShared<FJsonValueObject>(R));NextSample=Time+.25f;
                }
                LastFeet=Feet;LastGoal=Goal;LastShots=G->ShotsDischarged;LastPacks=Gun->Packs;LastDeploys=Gun->Deployments;LastState=Gun->State;
            }
            Check(Prefix+TEXT("moving_and_replaced_contacts_produce_two_natural_reload_cycles"),Finite&&G->ReloadsCompleted-BeginReloads>=2&&G->ShotsDischarged-BeginShots>G->GetMagazineCapacity());
            Check(Prefix+TEXT("continuous_contact_has_no_repeated_pack_and_deploy"),InitialStableRecorded&&StablePacks<=1&&StableDeploys<=1);
            Check(Prefix+TEXT("clear_ready_weapon_does_not_silently_stall"),MaxClearReadyNoShot<1.f);
            Check(Prefix+TEXT("reload_never_spends_a_round"),G->ShotsDuringReload==0);
            Check(Prefix+TEXT("real_short_sight_loss_was_exercised"),OccludedFrames>0);
            if(Context==3)Check(Prefix+TEXT("diagonal_direct_fire_and_legacy_stop_range_disagree_in_real_world"),RangeMismatchFrames>0);
            if(Held)Check(Prefix+TEXT("resident_maintains_physical_bank_clearance"),MaxIntrusion<=8.1f&&Gun->IsTrench());
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("team"),Team);Row->SetStringField(TEXT("faction"),B->GetFactionName(Team));Row->SetNumberField(TEXT("context"),Context);Row->SetNumberField(TEXT("trench"),Held?Trench:INDEX_NONE);Row->SetNumberField(TEXT("slot"),Q.Slot);
            Row->SetNumberField(TEXT("combat_seconds"),B->Elapsed-CombatStart);Row->SetNumberField(TEXT("shots"),G->ShotsDischarged-BeginShots);Row->SetNumberField(TEXT("reloads"),G->ReloadsCompleted-BeginReloads);Row->SetNumberField(TEXT("total_packs"),Gun->Packs-BeginPacks);Row->SetNumberField(TEXT("total_deployments"),Gun->Deployments-BeginDeploys);Row->SetNumberField(TEXT("stable_packs"),StablePacks);Row->SetNumberField(TEXT("stable_deployments"),StableDeploys);Row->SetNumberField(TEXT("late_travel_cm"),LateTravel);Row->SetNumberField(TEXT("max_bank_intrusion_cm"),MaxIntrusion);Row->SetNumberField(TEXT("max_step_cm"),MaxStep);Row->SetNumberField(TEXT("max_clear_ready_without_discharge_seconds"),MaxClearReadyNoShot);Row->SetNumberField(TEXT("actual_killed_and_replaced_contacts"),DeadContacts);Row->SetNumberField(TEXT("legacy_stop_range_mismatch_frames"),RangeMismatchFrames);Row->SetNumberField(TEXT("brief_occluded_frames"),OccludedFrames);Row->SetArrayField(TEXT("timeline"),Samples);
            if(Occluder)Occluder->SetActorEnableCollision(false);
            if(IsValid(E)){B->Units.Remove(E);E->Destroy();}
            const FVector BeforeAdvance=G->Feet();const int32 BeforeAdvancePacks=Gun->Packs;
            const bool Ordered=Held?B->AdvanceSquad(Trench,Id,Team):true;
            for(int32 I=0;I<320;++I){B->Elapsed+=.05f;for(auto* U:Men)Tick(U,.05f);}
            const float Forward=(G->Feet().X-BeforeAdvance.X)*Dir;
            Check(Prefix+TEXT("genuine_departure_still_releases_and_advances"),Ordered&&Forward>160.f&&Gun->Packs-BeforeAdvancePacks<=2);
            Row->SetNumberField(TEXT("public_departure_forward_cm"),Forward);Row->SetNumberField(TEXT("public_departure_packs"),Gun->Packs-BeforeAdvancePacks);
            // A genuine fatal hit is never a temporary zero-health visibility
            // switch. Survivors continue their ordinary handling, and the old
            // primary never spends another round.
            const int32 BeforeFatalShots=G->ShotsDischarged;G->ReceiveHit(1.f,0.f,1-Team);
            for(int32 I=0;I<80;++I){B->Elapsed+=.05f;for(auto* U:Men)Tick(U,.05f);}
            Check(Prefix+TEXT("dead_primary_never_reenters_stance_or_fires"),!G->IsAlive()&&G->ShotsDischarged==BeforeFatalShots);
            Cases.Add(MakeShared<FJsonValueObject>(Row));
            Check(Prefix+TEXT("checkpoint_written"),Checkpoint(User,Prefix,Row));
        }
        for(int32 Team:{0,1})for(int32 Context:{4,5,6,8})
        {
            auto Row=Focused(C,Team,Context,CaseId++,Check);Cases.Add(MakeShared<FJsonValueObject>(Row));
            const FString Prefix=FString::Printf(TEXT("v8450_t%d_context%d_"),Team,Context);
            Check(Prefix+TEXT("checkpoint_written"),Checkpoint(User,Prefix,Row));
        }
        for(int32 Team:{0,1})
        {
            auto Row=Scheduled(C,Team,CaseId++,Check);Cases.Add(MakeShared<FJsonValueObject>(Row));
            const FString Prefix=FString::Printf(TEXT("v8450_t%d_context7_"),Team);
            Check(Prefix+TEXT("checkpoint_written"),Checkpoint(User,Prefix,Row));
        }
        J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("trajectories"),Cases);
        J->SetNumberField(TEXT("trajectory_count"),Cases.Num());
        J->SetStringField(TEXT("scope"),TEXT("Eighteen real UK/DE complete-section trajectories on the prepared operation's actual floor/banks. Eight preserve the original two-feed baseline. Six 45-second contact cases exercise old stopping-range retention with repeated 0.12-second real occlusion, +/-20-degree score winners with mounted-sector retention followed by at most 30 seconds of normal-fatal-hit opposite-sector recovery, and enemy-present trench arrival. Two 20-second +/-10-degree cases exercise in-sector natural priority changes plus an actual pending burst whose current contact moves outside the mount while the clear inside candidate has a worse original score. Two at-most-82-second cases run the actual UpdateSquads scheduler before each unit Tick with two complete residents, public resident departure, true contact death/replacement and contact-loss release of the existing advance order. No synthetic follow-up orders, burst counters, health toggles or sight overrides are used. Evaluated pose, real muzzle/range gates, reload checks and per-case checkpoints remain authoritative. Empty vehicle fleet prevents hidden armour contamination. No full campaign, game-balance, foreground, frame-generation or aesthetic certification."));return J;
    }
};
