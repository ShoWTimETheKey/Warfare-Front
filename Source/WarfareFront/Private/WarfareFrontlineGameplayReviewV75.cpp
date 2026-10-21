#include "WarfareFrontlineGameplayReviewV75.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineSupportV32.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequenceBase.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

struct FFrontlineGameplayReviewV75 : public TSharedFromThis<FFrontlineGameplayReviewV75>
{
    struct FMan
    {
        TWeakObjectPtr<AWarfareFrontlineSoldier> Unit;
        FVector Previous=FVector::ZeroVector;
        float LateTravel=0,PreCaptureTravel=0,MaxVerticalStep=0,SmallStepMaxDrop=0,MaxBankIntrusion=0,FirstShot=-1;
        FVector InitialSupportGoal=FVector::ZeroVector;
        float MaxSupportGoalDrift=0,FirstDeployment=-1;
        int32 Frames=0,MoveFlips=0,LatePacks=0,PriorPacks=0;bool WasMoving=false,HasSupportGoal=false;
        float LateNearestBodyV8213=MAX_flt,LateRequestedPeakV8213=0,LateActualPeakV8213=0;
        int32 LateMoveFlipsV8213=0,LateWalkFlipsV8213=0,LatePostureFlipsV8213=0,LateWantedFramesV8213=0,LateWalkFramesV8213=0;
        int32 PriorPostureV8213=0;bool WasWalkV8213=false;
    };
    TWeakObjectPtr<AWarfareFrontlineController> PC;
    TArray<FMan> Men;
    TArray<TPair<TWeakObjectPtr<AWarfareFrontlineSoldier>,FVector>> MovingTargets;
    TWeakObjectPtr<AFrontlineTank> Train;
    TWeakObjectPtr<AActor> GroundSightWallV81;
    TWeakObjectPtr<AFrontlineTank> ATTargetV81;
    FVector ATStationV81=FVector::ZeroVector,ATDepartureFeetV81=FVector::ZeroVector;
    float ATDeployStartV81=-1,ATDeployReadyV81=-1,ATExpectedDeployV81=0,ATMaxStepV81=0,ATPlantedTravelV81=0;
    int32 ATReloadFramesV81=0,ATSupportedFramesV81=0,ATPacksBeforeDepartureV81=INDEX_NONE;
    bool ATObservationV81=true,ATClipsV81=true,ATSplitV81=false,ATBlockedShotsV81=true,ATDepartedV81=false;
    TArray<TSharedPtr<FJsonValue>> Checks,Cases,Timeline;
    FString Directory;
    double Started=0,CaseStartedWorld=0,LastWorld=0;
    int32 Case=0,Operation=17,Frames=0,Phase=0;
    float NextSample=0;bool Good=true,Captured=false,BeforeCaptureChecked=false,BanksOnly=false,StanceOnlyV81=false,EdgeStationsOnlyV81=false;
    bool GroundSplitObservedV81=false,GroundBlockedShotsV81=true;
    bool FormationOnlyV8213=false;

    void Check(const FString& Name,bool Pass)
    {
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);
        Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;
        UE_LOG(LogTemp,Display,TEXT("WF_V75_GAMEPLAY_CHECK %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));
    }
    AWarfareFrontlineGameMode* Battle()const
    {return PC.IsValid()?Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode()):nullptr;}
    void Finish()
    {
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);
        J->SetNumberField(TEXT("operation"),Operation);J->SetArrayField(TEXT("checks"),Checks);
        J->SetArrayField(TEXT("cases"),Cases);J->SetArrayField(TEXT("timeline"),Timeline);
        J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Started);
        J->SetStringField(TEXT("scope"),FormationOnlyV8213?TEXT("Two live-world 15-second entry fixtures, both armies simultaneously in their actual home trenches. Three full rifle sections, then two full rifle sections with an unchanged MG section. Records actual Y, goal/body clearance, motor intent and speed, late walk/posture flips and MG fire. Disposable no-save profile; no campaign balance or visual acceptance claim."):EdgeStationsOnlyV81?TEXT("Four labelled open-ground MG section fixtures at both lateral field edges in both armies. Ordinary soldier, evaluated skeleton and support component ticks run at a fixed 120Hz before any screenshot. Checks reachable helper stations, station stability, body clearance, bounded movement and real gunfire. Isolated no-save profile; no campaign, natural encounter or visual acceptance claim."):StanceOnlyV81?TEXT("Four live trench-entry cases, two open-ground MG section cases in both factions and one live German AT section. Ground cases separate observation from blocked prone muzzle, remove a low wall, then change target bearing. AT checks the measured personal deployment duration, real blocked/clear fire, complete reload without repacking, and range-loss departure. Counts assistant stations, late packing and facing. Disposable profile only; screenshots require independent visual review."):BanksOnly?TEXT("Actual operation-fitted rear-bank support and front-wall clearance samples for both armies. No combat or rail rerun; disposable profile only.")
            :TEXT("Four live-world 15-second MG trench-entry cases with full crews, both armies and static/moving targets; focused real heavy-rifle, shrapnel and strafe checks; one live 20-second armoured-train fight, passenger deployment and repeat-request/destruction checks. Disposable campaign only. Screenshots require independent visual review; this does not certify all maps or campaign balance."));
        FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));
        FFileHelper::SaveStringToFile(Text,*(Directory/TEXT("review.json")));
        FPlatformMisc::RequestExit(false);
    }
    void Clear(AWarfareFrontlineGameMode* B)
    {
        if(auto* Wall=GroundSightWallV81.Get())Wall->Destroy();GroundSightWallV81.Reset();
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();
        B->Units.Empty();B->Squads.Empty();B->Tracers.Empty();B->SmokeZones.Empty();
        FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);Men.Reset();MovingTargets.Reset();
    }
    void SmallStepBankCheck(AWarfareFrontlineGameMode* B,int32 Trench,int32 Team)
    {
        AWarfareFrontlineSoldier* U=nullptr;
        for(const auto& M:Men)if(auto* Man=M.Unit.Get();Man&&Man->IsMachineGunner()){U=Man;break;}
        if(!U)return;auto* S=B->FindSquad(U->SquadId);if(!S)return;
        const int32 OldTrench=S->Trench;const bool OldAdvance=S->bAdvancing;
        S->Trench=Trench;S->bAdvancing=false;
        const float Dir=B->GetTeamDirection(Team),Y=U->Feet().Y,Centre=FrontlineFieldV32::TrenchCentreX(Trench,Y);
        int32 OldFalseBlocks=0,SupportedRearSamples=0,FailedEntrySamples=0;FVector FirstFrom=FVector::ZeroVector,FirstTo=FVector::ZeroVector;bool Enter=true,Wall=true;int32 WallSamples=0;
        // Bank geometry differs by operation and lateral slice. Assert the
        // actual inward-entry contract, not that this exact slice happens to
        // reproduce the historical operation-6 predicate. Nearby genuine bank
        // surfaces exercise both orientations without lifting a synthetic man.
        for(int32 Slice=-30;Slice<=30;++Slice)
        {
            const float SampleY=Y+Slice*36.f;if(FMath::Abs(SampleY)>FrontlineBattlefield::HalfFrontage-60.f)continue;
            const float SampleCentre=FrontlineFieldV32::TrenchCentreX(Trench,SampleY);
            for(int32 Offset=-139;Offset<0;++Offset)
            {
                FVector From(SampleCentre+Dir*Offset,SampleY,0);From.Z=FrontlineInfantryV45::SurfaceHeight(B,From);
                const FVector To=From+FVector(Dir,0,0); // 1 cm = a normal 240 Hz walking step.
                if(From.Z<=AWarfareFrontlineGround::HeightAt(From.X,From.Y)+8.f)continue;
                ++SupportedRearSamples;const bool Allowed=FrontlineInfantryV45::AllowsTrenchStep(U,From,To);Enter&=Allowed;
                if(!Allowed){if(FailedEntrySamples==0){FirstFrom=From;FirstTo=To;}++FailedEntrySamples;}
                const float Old=FrontlineInfantryV45::BankIntrusion(U,From),Next=FrontlineInfantryV45::BankIntrusion(U,To);
                if(Next>8.f&&Next>Old+.05f)++OldFalseBlocks;
            }
        }
        for(int32 Offset=0;Offset<=138;++Offset)
        {
            FVector From(Centre+Dir*Offset,Y,0);From.Z=AWarfareFrontlineGround::HeightAt(From.X,From.Y);
            const FVector To=From+FVector(Dir,0,0);
            const float Old=FrontlineInfantryV45::BankIntrusion(U,From),Next=FrontlineInfantryV45::BankIntrusion(U,To);
            if(Next>8.f&&Next>Old+.05f)
            {++WallSamples;Wall&=!FrontlineInfantryV45::AllowsTrenchStep(U,From,To);}
        }
        S->Trench=OldTrench;S->bAdvancing=OldAdvance;
        auto BankEvidence=MakeShared<FJsonObject>();BankEvidence->SetStringField(TEXT("scene"),TEXT("bank_contact"));BankEvidence->SetNumberField(TEXT("case"),Case);BankEvidence->SetNumberField(TEXT("team"),Team);BankEvidence->SetNumberField(TEXT("actor_team"),U->Team);BankEvidence->SetNumberField(TEXT("trench"),Trench);BankEvidence->SetNumberField(TEXT("supported_rear_samples"),SupportedRearSamples);BankEvidence->SetNumberField(TEXT("failed_entry_samples"),FailedEntrySamples);BankEvidence->SetNumberField(TEXT("front_wall_samples"),WallSamples);BankEvidence->SetStringField(TEXT("first_failed_from"),FirstFrom.ToString());BankEvidence->SetStringField(TEXT("first_failed_to"),FirstTo.ToString());Cases.Add(MakeShared<FJsonValueObject>(BankEvidence));
        const FString Prefix=FString::Printf(TEXT("case%d_"),Case);
        Check(Prefix+TEXT("one_cm_supported_rear_bank_entry_is_unblocked"),SupportedRearSamples>0&&Enter);
        Check(Prefix+TEXT("one_cm_front_wall_retains_body_clearance"),WallSamples>0&&Wall);
        UE_LOG(LogTemp,Display,TEXT("WF_V75_BANK_SMALL_STEP case=%d supported_rear_samples=%d legacy_false_blocks=%d front_wall_samples=%d enter=%d wall=%d"),Case,SupportedRearSamples,OldFalseBlocks,WallSamples,Enter,Wall);
    }
    bool Setup()
    {
        auto* C=PC.Get();auto* B=Battle();if(!C||!B)return false;
        if(Case==0)
        {
            if(!FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,Operation))return false;
            B->BeginBattle();B->SetPaused(false);B->SetActorTickEnabled(false);
            C->SetActorTickEnabled(false);if(C->GetHUD())C->GetHUD()->bShowHUD=false;
        }
        Clear(B);B->Elapsed=0;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
        if(FormationOnlyV8213)return SetupFormationV8213(B);
        if(StanceOnlyV81&&Case==6)return SetupATV81(B);
        if(StanceOnlyV81&&Case>=4)return SetupGroundV81(B);
        const int32 Team=Case/2,Variant=Case%2,Dir=B->GetTeamDirection(Team);
        const int32 Trench=Variant?FrontlineBattlefield::CentreTrench:B->GetHomeTrenchIndex(Team);
        B->Trenches[Trench].Owner=Team;
        for(int32 Slot=0;Slot<3;++Slot)
        {
            const bool MG=Variant?(Slot==1):(Slot!=1);
            if(!B->SpawnSquad(Team,MG?EFrontlineRole::Lewis:EFrontlineRole::Rifle,Trench))return false;
            auto& S=B->Squads.Last();S.Slot=Slot;S.bAdvancing=true;S.bReinforcement=false;
            S.Trench=INDEX_NONE;S.Destination=Trench;S.LastPassedTrench=Trench-Dir;
            for(const auto& W:S.Members)if(auto* U=W.Get())
            {
                U->ReturnToIdle();U->bFieldEntered=true;U->DepartureClock=0;U->ReactionSeconds=.18f;
                // The actual role roster, personal offsets and all companions
                // walk across the real bank. No teleport to the firing bay.
                FVector P=B->SoldierDestination(U);
                P.X=FrontlineFieldV32::TrenchCentreX(Trench,P.Y)-Dir*(315.f+U->MemberIndex*18.f);
                P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
                U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,Dir>0?0:180,0));
                U->ShotClock=U->IsMachineGunner()?0.f:10000.f;U->GrenadeClock=10000;
                U->SetActorTickEnabled(true);U->SetMotion(U->GetWalkMotion());
                FMan Track;Track.Unit=U;Track.Previous=P;Men.Add(Track);
            }
        }
        // Already visible enemy contact during entry is deliberate: the old
        // review introduced contact only after the gun had reached its slot.
        for(int32 I=0;I<12;++I)
        {
            FVector P(B->Trenches[Trench].X+Dir*(1050.f+(I%3)*95.f),(I/3-1.5f)*480.f+(I%3)*55.f,0);
            P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return false;
            U->Initialize(B,690000+Case*100+I,1-Team,EFrontlineRole::Rifle,0);
            U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,Dir>0?180:0,0));
            U->bFieldEntered=true;U->SetActorTickEnabled(false);U->SetMotion(U->GetAimMotion());B->Units.Add(U);
            if(Variant)MovingTargets.Add({U,P});
        }
        if(Case==0&&!Men.IsEmpty())
        {
            auto* U=Men[0].Unit.Get();auto* Enemy=B->Units.Last().Get();
            Check(TEXT("friendly_available_bay_finishes_near_entry"),B->ShouldFinishFriendlyTrenchEntryV69(U));
            const int32 Owner=B->Trenches[Trench].Owner;B->Trenches[Trench].Owner=1-Team;
            Check(TEXT("enemy_owned_bay_does_not_bypass_contact"),!B->ShouldFinishFriendlyTrenchEntryV69(U));B->Trenches[Trench].Owner=Owner;
            const FVector Old=Enemy->GetActorLocation();Enemy->SetActorLocation(U->GetActorLocation()+FVector(Dir*250.f,0,0));
            Check(TEXT("close_enemy_still_stops_friendly_entry"),!B->ShouldFinishFriendlyTrenchEntryV69(U));Enemy->SetActorLocation(Old);
        }
        if(C->BattleCamera)
        {
            const FVector Focus(B->Trenches[Trench].X+Dir*500.f,0,550),Location=Focus+FVector(0,14000,4000);
            C->BattleCamera->SetActorLocation(Location);C->BattleCamera->SetActorRotation((Focus-Location).Rotation());
            C->BattleCamera->GetCameraComponent()->OrthoWidth=3900;C->SetViewTarget(C->BattleCamera);
        }
        if(Variant)SmallStepBankCheck(B,Trench,Team);
        Frames=0;NextSample=0;Captured=BeforeCaptureChecked=false;CaseStartedWorld=LastWorld=B->GetWorld()->GetTimeSeconds();
        Check(FString::Printf(TEXT("case%d_full_rosters_and_live_world"),Case),Men.Num()==(Variant?15:12)&&!B->GetWorld()->IsPaused());
        return true;
    }
    static int32 FormationPostureV8213(const AWarfareFrontlineSoldier* U)
    {
        const auto* G=FrontlineMachinegunsV30::Get(U);
        return (G?(1+int32(G->State))*4:0)+(G&&G->IsProne()?2:0)+(FrontlineMachinegunsV30::UsesLowTrenchPoseV64(U)?1:0);
    }
    bool SetupFormationV8213(AWarfareFrontlineGameMode* B)
    {
        for(int32 Team=0;Team<2;++Team)
        {
            const int32 Trench=B->GetHomeTrenchIndex(Team),Dir=B->GetTeamDirection(Team);
            if(!B->Trenches.IsValidIndex(Trench))return false;
            B->Trenches[Trench].Owner=Team;
            for(int32 Slot=0;Slot<3;++Slot)
            {
                const bool MG=Case==1&&Slot==2;
                if(!B->SpawnSquad(Team,MG?EFrontlineRole::Lewis:EFrontlineRole::Rifle,Trench))return false;
                auto& S=B->Squads.Last();S.Slot=Slot;S.bAdvancing=true;S.bReinforcement=false;
                S.Trench=INDEX_NONE;S.Destination=Trench;S.LastPassedTrench=Trench-Dir;
                for(const auto& W:S.Members)if(auto* U=W.Get())
                {
                    U->ReturnToIdle();U->bFieldEntered=true;U->DepartureClock=0;U->ReactionSeconds=.18f;
                    FVector P=B->SoldierDestination(U);
                    P.X=FrontlineFieldV32::TrenchCentreX(Trench,P.Y)-Dir*(315.f+U->MemberIndex*18.f);
                    P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
                    U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,Dir>0?0:180,0));
                    U->ShotClock=U->IsMachineGunner()?0.f:10000.f;U->GrenadeClock=10000.f;
                    U->SetActorTickEnabled(true);U->SetMotion(U->GetWalkMotion());
                    FMan M;M.Unit=U;M.Previous=P;M.WasWalkV8213=true;M.PriorPostureV8213=FormationPostureV8213(U);Men.Add(M);
                }
            }
            // Use a full ordinary contact screen for the MG's late window.
            // No health, hit rule, destination or support-state override is used.
            for(int32 I=0;I<24;++I)
            {
                FVector P(B->Trenches[Trench].X+Dir*(1050.f+(I%3)*95.f),(I/3-3.5f)*120.f+(I%3)*35.f,0);
                P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
                auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return false;
                U->Initialize(B,821300+Case*1000+Team*100+I,1-Team,EFrontlineRole::Rifle,0);
                U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,Dir>0?180:0,0));
                U->bFieldEntered=true;U->SetActorTickEnabled(false);U->SetMotion(U->GetAimMotion());B->Units.Add(U);
            }
        }
        Frames=0;NextSample=0;CaseStartedWorld=LastWorld=B->GetWorld()->GetTimeSeconds();
        Check(FString::Printf(TEXT("formation_case%d_full_both_army_rosters"),Case),Men.Num()==(Case==0?36:30)&&!B->GetWorld()->IsPaused());
        return true;
    }
    bool TickFormationV8213(AWarfareFrontlineGameMode* B,float Time,float DT)
    {
        TArray<FVector> Goals;Goals.Reserve(Men.Num());
        for(const auto& M:Men)
        {
            auto* U=M.Unit.Get();bool Reform=false;
            Goals.Add(U?FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform):FVector::ZeroVector);
        }
        for(auto& M:Men)if(auto* U=M.Unit.Get())
        {
            const FVector Feet=U->Feet();const bool Walk=U->Playing==U->GetWalkMotion()||U->Playing==U->GetRunMotion();
            const int32 Posture=FormationPostureV8213(U);
            if(Time>=12.f)
            {
                M.LateTravel+=FVector::Dist2D(Feet,M.Previous);
                M.LateRequestedPeakV8213=FMath::Max(M.LateRequestedPeakV8213,U->RequestedTravelSpeedV43);
                M.LateActualPeakV8213=FMath::Max(M.LateActualPeakV8213,U->GetMotionSpeed());
                M.LateWantedFramesV8213+=U->bWantedMovement?1:0;M.LateWalkFramesV8213+=Walk?1:0;
                M.LateMoveFlipsV8213+=M.WasMoving!=U->bMoving?1:0;
                M.LateWalkFlipsV8213+=M.WasWalkV8213!=Walk?1:0;
                M.LatePostureFlipsV8213+=M.PriorPostureV8213!=Posture?1:0;
                for(const auto& Other:Men)if(auto* V=Other.Unit.Get();V&&V!=U&&V->Team==U->Team)
                    M.LateNearestBodyV8213=FMath::Min(M.LateNearestBodyV8213,float(FVector::Dist2D(Feet,V->Feet())));
            }
            if(M.WasMoving!=U->bMoving)++M.MoveFlips;
            M.WasMoving=U->bMoving;M.WasWalkV8213=Walk;M.PriorPostureV8213=Posture;M.Previous=Feet;++M.Frames;
            if(U->ShotsDischarged>0&&M.FirstShot<0)M.FirstShot=Time;
        }
        if(Time>=NextSample)
        {
            Sample(B,Time);NextSample=Time+.25f;
            auto Frame=MakeShared<FJsonObject>();Frame->SetStringField(TEXT("scene"),TEXT("formation_clearance"));
            Frame->SetNumberField(TEXT("case"),Case);Frame->SetNumberField(TEXT("seconds"),Time);TArray<TSharedPtr<FJsonValue>> Rows;
            for(int32 I=0;I<Men.Num();++I)if(auto* U=Men[I].Unit.Get())
            {
                float Body=MAX_flt,Goal=MAX_flt;
                for(int32 K=0;K<Men.Num();++K)if(auto* V=Men[K].Unit.Get();K!=I&&V&&V->Team==U->Team)
                {Body=FMath::Min(Body,float(FVector::Dist2D(U->Feet(),V->Feet())));Goal=FMath::Min(Goal,float(FVector::Dist2D(Goals[I],Goals[K])));}
                auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("team"),U->Team);R->SetNumberField(TEXT("squad"),U->SquadId);
                R->SetNumberField(TEXT("member"),U->MemberIndex);R->SetNumberField(TEXT("actual_y_cm"),U->Feet().Y);
                R->SetNumberField(TEXT("goal_y_cm"),Goals[I].Y);R->SetNumberField(TEXT("nearest_body_cm"),Body);
                R->SetNumberField(TEXT("nearest_goal_cm"),Goal);Rows.Add(MakeShared<FJsonValueObject>(R));
            }
            Frame->SetArrayField(TEXT("men"),Rows);Timeline.Add(MakeShared<FJsonValueObject>(Frame));
        }
        if(Time<15.f)return true;
        for(int32 Team=0;Team<2;++Team)
        {
            const FString Prefix=FString::Printf(TEXT("formation_case%d_team%d_"),Case,Team);
            bool Alive=true,Separated=true,GoalsClear=true,Settled=true,NoFlips=true,GunReady=true;int32 Count=0;TArray<TSharedPtr<FJsonValue>> Rows;
            for(int32 I=0;I<Men.Num();++I)if(auto* U=Men[I].Unit.Get();U&&U->Team==Team)
            {
                const auto& M=Men[I];const auto* G=FrontlineMachinegunsV30::Get(U);++Count;
                float NearestGoal=MAX_flt;
                for(int32 K=0;K<Men.Num();++K)if(auto* V=Men[K].Unit.Get();K!=I&&V&&V->Team==Team)
                    NearestGoal=FMath::Min(NearestGoal,float(FVector::Dist2D(Goals[I],Goals[K])));
                Alive&=U->IsAlive();Separated&=M.LateNearestBodyV8213>=55.4f;
                GoalsClear&=NearestGoal>=56.f;
                Settled&=M.LateTravel<12.f&&!U->bWantedMovement&&M.LateWalkFramesV8213==0;
                NoFlips&=M.LateMoveFlipsV8213==0&&M.LateWalkFlipsV8213==0&&M.LatePostureFlipsV8213==0;
                if(U->IsMachineGunner())GunReady&=G&&G->IsSupported()&&G->IsTrench()&&U->ShotsDischarged>0;
                auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("squad"),U->SquadId);R->SetNumberField(TEXT("member"),U->MemberIndex);
                R->SetNumberField(TEXT("actual_y_cm"),U->Feet().Y);R->SetNumberField(TEXT("goal_y_cm"),Goals[I].Y);
                R->SetNumberField(TEXT("nearest_goal_cm"),NearestGoal);
                R->SetNumberField(TEXT("slot_error_cm"),FVector::Dist2D(U->Feet(),Goals[I]));
                R->SetNumberField(TEXT("late_nearest_body_cm"),M.LateNearestBodyV8213);R->SetNumberField(TEXT("last_3s_travel_cm"),M.LateTravel);
                R->SetBoolField(TEXT("wanted_movement"),U->bWantedMovement);R->SetBoolField(TEXT("moving"),U->bMoving);
                R->SetBoolField(TEXT("arrived"),U->bAtDestinationV43);R->SetNumberField(TEXT("requested_speed"),U->RequestedTravelSpeedV43);
                R->SetNumberField(TEXT("actual_speed"),U->GetMotionSpeed());R->SetNumberField(TEXT("late_requested_speed_peak"),M.LateRequestedPeakV8213);
                R->SetNumberField(TEXT("late_actual_speed_peak"),M.LateActualPeakV8213);R->SetNumberField(TEXT("late_wanted_frames"),M.LateWantedFramesV8213);
                R->SetNumberField(TEXT("late_walk_frames"),M.LateWalkFramesV8213);R->SetNumberField(TEXT("late_move_flips"),M.LateMoveFlipsV8213);
                R->SetNumberField(TEXT("late_walk_flips"),M.LateWalkFlipsV8213);R->SetNumberField(TEXT("late_posture_flips"),M.LatePostureFlipsV8213);
                R->SetStringField(TEXT("motion"),GetNameSafe(U->Playing));R->SetNumberField(TEXT("stance"),G?int32(G->State):-1);
                R->SetNumberField(TEXT("packs"),G?G->Packs:0);R->SetNumberField(TEXT("shots"),U->ShotsDischarged);
                R->SetNumberField(TEXT("first_shot_seconds"),M.FirstShot);Rows.Add(MakeShared<FJsonValueObject>(R));
            }
            Check(Prefix+TEXT("complete_original_roster_alive"),Alive&&Count==(Case==0?18:15));
            Check(Prefix+TEXT("full_roster_goal_clearance"),GoalsClear);Check(Prefix+TEXT("late_body_clearance"),Separated);
            Check(Prefix+TEXT("no_late_walking_or_motor_orbit"),Settled&&NoFlips);
            if(Case==1)Check(Prefix+TEXT("unchanged_mg_deploys_and_fires"),GunReady);
            auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("case"),Case);J->SetNumberField(TEXT("team"),Team);
            J->SetStringField(TEXT("scene"),Case==0?TEXT("three_full_rifle_sections"):TEXT("two_full_rifle_sections_and_mg"));
            J->SetNumberField(TEXT("world_seconds"),Time);J->SetNumberField(TEXT("frames"),Frames);J->SetArrayField(TEXT("men"),Rows);
            Cases.Add(MakeShared<FJsonValueObject>(J));
        }
        Check(FString::Printf(TEXT("formation_case%d_live_world_advanced"),Case),Frames>=120&&DT>0.f);
        ++Case;if(Case>=2){Finish();return false;}
        if(!Setup()){Check(TEXT("formation_next_fixture_setup"),false);Finish();return false;}return true;
    }
    bool SetupGroundV81(AWarfareFrontlineGameMode* B)
    {
        const int32 Team=Case-4,Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team);
        B->Trenches[Home].Owner=Team;
        if(!B->SpawnSquad(Team,EFrontlineRole::Lewis,Home))return false;
        auto& S=B->Squads.Last();S.bAdvancing=true;S.bReinforcement=false;S.Trench=INDEX_NONE;
        S.Destination=B->GetHomeTrenchIndex(1-Team);S.LastPassedTrench=Home;
        FVector Station(B->Trenches[Home].X+Dir*700.f,-700.f,0);
        Station.Z=FrontlineInfantryV45::SurfaceHeight(B,Station);
        for(const auto& W:S.Members)if(auto* U=W.Get())
        {
            FVector P=Station+FVector(-Dir*U->MemberIndex*90.f,U->MemberIndex*145.f,0);
            P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
            U->ReturnToIdle();U->bFieldEntered=true;U->ReactionSeconds=.12f+U->GetMotionPhase()*.4f;U->DepartureClock=0;
            U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,Dir>0?0:180,0));
            U->ShotClock=U->IsMachineGunner()?0.f:10000.f;U->GrenadeClock=10000.f;U->SetActorTickEnabled(true);
            FMan Track;Track.Unit=U;Track.Previous=P;Men.Add(Track);
        }
        for(int32 I=0;I<8;++I)
        {
            FVector P=Station+FVector(Dir*(920.f+I*24.f),(I-3.5f)*28.f,0);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return false;
            U->Initialize(B,810000+Team*100+I,1-Team,EFrontlineRole::Rifle,0);U->bFieldEntered=true;
            U->SetActorTickEnabled(false);U->SetActorLocation(P+FVector(0,0,88));U->SetMotion(U->GetAimMotion());B->Units.Add(U);
            MovingTargets.Add({U,P});
        }
        auto* Wall=B->GetWorld()->SpawnActor<AActor>();if(!Wall)return false;GroundSightWallV81=Wall;
        auto* Block=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Block);Wall->AddInstanceComponent(Block);
        Block->SetBoxExtent(FVector(20,1000,35));Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Block->SetCollisionResponseToAllChannels(ECR_Ignore);Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Block->RegisterComponent();
        Wall->SetActorLocation(Station+FVector(Dir*180.f,0,35.f));
        if(auto* C=PC.Get();C&&C->BattleCamera)
        {
            const FVector Focus=Station+FVector(Dir*450.f,0,250.f),Eye=Focus+FVector(0,14000,4000);
            C->BattleCamera->SetActorLocation(Eye);C->BattleCamera->SetActorRotation((Focus-Eye).Rotation());
            C->BattleCamera->GetCameraComponent()->OrthoWidth=2200;C->SetViewTarget(C->BattleCamera);
        }
        GroundSplitObservedV81=false;GroundBlockedShotsV81=true;
        Frames=0;NextSample=0;Captured=BeforeCaptureChecked=false;CaseStartedWorld=LastWorld=B->GetWorld()->GetTimeSeconds();
        Check(FString::Printf(TEXT("case%d_real_ground_section_and_low_wall"),Case),Men.Num()==3&&!B->GetWorld()->IsPaused());return true;
    }
    void FixedEdgeStationChecksV81(AWarfareFrontlineGameMode* B)
    {
        constexpr float DT=1.f/120.f;
        for(int32 Team=0;Team<2;++Team)for(int32 Edge:{-1,1})
        {
            Case=4+Team;Clear(B);B->Elapsed=0;
            const FString Prefix=FString::Printf(TEXT("edge_team%d_side%d_"),Team,Edge);
            if(!SetupGroundV81(B)){Check(Prefix+TEXT("full_section_setup"),false);continue;}
            if(auto* Wall=GroundSightWallV81.Get())Wall->Destroy();GroundSightWallV81.Reset();
            const float Shift=Edge*(FrontlineBattlefield::HalfFrontage-70.f)+700.f;
            // Put the actual roster inside the movement boundary with normal
            // spacing. The outer helper then needs a reachable covering bay.
            for(auto& M:Men)if(auto* U=M.Unit.Get())
            {
                FVector P=U->Feet();P.Y+=Shift-Edge*U->MemberIndex*145.f-U->MemberIndex*145.f;
                P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));
                U->SetActorTickEnabled(false);M.Previous=P;
            }
            for(const auto& Entry:MovingTargets)if(auto* U=Entry.Key.Get())
            {FVector P=Entry.Value;P.Y+=Shift-Edge*120.f;P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));}
            for(AWarfareFrontlineSoldier* U:B->Units)if(U)
            {U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(0,false);U->Soldier->RefreshBoneTransforms();}
            FrontlineMobilityV41::Reset(B);
            bool Finite=true,Inside=true,Separated=true,PlantedMotion=true;float MaxStep=0;
            for(int32 Frame=0;Frame<1800;++Frame)
            {
                B->Elapsed+=DT;
                for(auto& M:Men)if(auto* U=M.Unit.Get())
                {
                    const FVector Before=U->Feet();U->Tick(DT);
                    U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();
                    if(auto* G=FrontlineMachinegunsV30::Get(U))G->TickComponent(DT,LEVELTICK_All,nullptr);
                    const FVector Feet=U->Feet();Finite&=!Feet.ContainsNaN();Inside&=FMath::Abs(Feet.Y)<=FrontlineBattlefield::HalfFrontage-35.f;
                    MaxStep=FMath::Max(MaxStep,float(FVector::Dist2D(Before,Feet)));
                    if(Frame>=1440)M.LateTravel+=FVector::Dist2D(Before,Feet);
                    if(U->GetMotionSpeed()<.1f)PlantedMotion&=U->Playing!=U->GetWalkMotion()&&U->Playing!=U->GetRunMotion();
                    if(!U->IsMachineGunner())
                    {
                        bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);
                        if(auto* G=FrontlineMachinegunsV30::Get(U);G&&G->IsSupported())
                        {
                            if(!M.HasSupportGoal){M.InitialSupportGoal=Goal;M.HasSupportGoal=true;}
                            M.MaxSupportGoalDrift=FMath::Max(M.MaxSupportGoalDrift,float(FVector::Dist2D(Goal,M.InitialSupportGoal)));
                        }
                    }
                }
                for(int32 I=0;I<Men.Num();++I)for(int32 K=I+1;K<Men.Num();++K)
                    if(Men[I].Unit.IsValid()&&Men[K].Unit.IsValid())Separated&=FVector::Dist2D(Men[I].Unit->Feet(),Men[K].Unit->Feet())>=55.4f;
            }
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("scene"),TEXT("fixed_small_step_edge_stations"));
            J->SetNumberField(TEXT("team"),Team);J->SetNumberField(TEXT("edge"),Edge);J->SetNumberField(TEXT("dt_seconds"),DT);
            J->SetNumberField(TEXT("frames"),1800);J->SetNumberField(TEXT("max_step_cm"),MaxStep);
            bool GoalsInside=true,Settled=true,StationStable=true,GunFire=false;TArray<TSharedPtr<FJsonValue>> Rows;
            for(const auto& M:Men)if(auto* U=M.Unit.Get())
            {
                auto* G=FrontlineMachinegunsV30::Get(U);bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);
                auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("member"),U->MemberIndex);R->SetStringField(TEXT("feet"),U->Feet().ToString());
                R->SetStringField(TEXT("goal"),Goal.ToString());R->SetNumberField(TEXT("slot_error_cm"),FVector::Dist2D(Goal,U->Feet()));
                R->SetNumberField(TEXT("last_3s_travel_cm"),M.LateTravel);R->SetNumberField(TEXT("packs"),G?G->Packs:0);R->SetNumberField(TEXT("shots"),U->ShotsDischarged);
                R->SetBoolField(TEXT("supported"),G&&G->IsSupported());R->SetStringField(TEXT("motion"),GetNameSafe(U->Playing));Rows.Add(MakeShared<FJsonValueObject>(R));
                Settled&=G&&G->IsSupported()&&M.LateTravel<2.f;
                if(U->IsMachineGunner())GunFire=U->ShotsDischarged>0;
                else {GoalsInside&=FMath::Abs(Goal.Y)<=FrontlineBattlefield::HalfFrontage-35.f;Settled&=FVector::Dist2D(Goal,U->Feet())<=32.1f;StationStable&=M.HasSupportGoal&&M.MaxSupportGoalDrift<.1f;}
            }
            J->SetArrayField(TEXT("men"),Rows);Cases.Add(MakeShared<FJsonValueObject>(J));
            Check(Prefix+TEXT("covering_stations_are_inside_movement_frontage"),GoalsInside);
            Check(Prefix+TEXT("full_section_reaches_stable_supported_stations"),Settled&&StationStable);
            Check(Prefix+TEXT("clear_gun_fires_and_accepted_travel_drives_motion"),GunFire&&PlantedMotion);
            Check(Prefix+TEXT("ordinary_steps_preserve_bounds_and_body_clearance"),Finite&&Inside&&Separated&&MaxStep<4.f);
        }
    }
    bool SetupATV81(AWarfareFrontlineGameMode* B)
    {
        const int32 Team=B->GetFactionForTeam(0)==EFrontlineFaction::German?0:1,Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team);
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        if(!Fleet||!B->SpawnSquad(Team,EFrontlineRole::AntiTankRifle,Home))return false;
        auto& S=B->Squads.Last();S.bAdvancing=true;S.bReinforcement=false;S.Trench=INDEX_NONE;
        S.Destination=B->GetHomeTrenchIndex(1-Team);S.LastPassedTrench=Home;
        ATStationV81=FVector(B->Trenches[Home].X+Dir*700.f,-700.f,0);ATStationV81.Z=FrontlineInfantryV45::SurfaceHeight(B,ATStationV81);
        for(const auto& W:S.Members)if(auto* U=W.Get())
        {
            FVector P=ATStationV81+FVector(-Dir*U->MemberIndex*80.f,U->MemberIndex*145.f,0);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
            U->ReturnToIdle();U->bFieldEntered=true;U->ReactionSeconds=.18f;U->DepartureClock=0;
            U->ShotClock=U->MemberIndex==0?0.f:10000.f;U->GrenadeClock=10000.f;U->SetActorTickEnabled(true);
            U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,Dir>0?0:180,0));
            FMan M;M.Unit=U;M.Previous=P;Men.Add(M);
        }
        auto* Tank=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,1-Team,false);if(!Tank)return false;ATTargetV81=Tank;
        Tank->bEntered=true;Tank->FitGroundV35(ATStationV81+FVector(Dir*950.f,0,0),0.f);
        // This is a sight/stance fixture. Retain the actual hull and collision,
        // but prevent its movement and retaliation from changing that question.
        Tank->TrackIntegrity[0]=Tank->TrackIntegrity[1]=0;
        for(float& Clock:Tank->GunClock)Clock=10000.f;for(float& Clock:Tank->CannonClock)Clock=10000.f;
        Fleet->NextEnemyAt=10000.f;
        auto* Wall=B->GetWorld()->SpawnActor<AActor>();if(!Wall)return false;GroundSightWallV81=Wall;
        auto* Block=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Block);Wall->AddInstanceComponent(Block);
        Block->SetBoxExtent(FVector(20,1000,50));Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Block->SetCollisionResponseToAllChannels(ECR_Ignore);Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Block->RegisterComponent();
        Wall->SetActorLocation(ATStationV81+FVector(Dir*180.f,0,50.f));
        ATDeployStartV81=ATDeployReadyV81=-1;ATExpectedDeployV81=ATMaxStepV81=ATPlantedTravelV81=0;
        ATReloadFramesV81=ATSupportedFramesV81=0;ATPacksBeforeDepartureV81=INDEX_NONE;
        ATObservationV81=ATClipsV81=ATBlockedShotsV81=true;ATSplitV81=ATDepartedV81=false;
        Frames=0;Captured=false;CaseStartedWorld=LastWorld=B->GetWorld()->GetTimeSeconds();
        if(auto* C=PC.Get();C&&C->BattleCamera)
        {
            const FVector Focus=ATStationV81+FVector(Dir*450.f,0,250.f),Eye=Focus+FVector(0,14000,4000);
            C->BattleCamera->SetActorLocation(Eye);C->BattleCamera->SetActorRotation((Focus-Eye).Rotation());
            C->BattleCamera->GetCameraComponent()->OrthoWidth=2200;C->SetViewTarget(C->BattleCamera);
        }
        Check(TEXT("case6_live_german_at_section_and_real_hull"),Men.Num()==2&&Tank->IsOperational()&&!B->GetWorld()->IsPaused());return true;
    }
    bool TickATV81(AWarfareFrontlineGameMode* B,float Time,float DT)
    {
        auto* U=Men.IsEmpty()?nullptr:Men[0].Unit.Get();auto* Gun=FrontlineMachinegunsV30::Get(U);auto* Tank=ATTargetV81.Get();
        if(!U||!Gun||!Tank){Check(TEXT("case6_at_fixture_remains_valid"),false);Finish();return false;}
        ATMaxStepV81=FMath::Max(ATMaxStepV81,DT);
        if(Gun->State==EFrontlineGunStanceV30::Deploy&&ATDeployStartV81<0)
        {
            ATDeployStartV81=Time;const auto* Clip=Cast<UAnimSequenceBase>(Gun->TransitionMotion());
            ATExpectedDeployV81=Clip?Clip->GetPlayLength()/Gun->TransitionRate():0.f;
        }
        if(Gun->IsSupported())
        {
            if(ATDeployReadyV81<0)ATDeployReadyV81=Time;
            ++ATSupportedFramesV81;ATClipsV81&=GetNameSafe(U->Playing).StartsWith(TEXT("A_AT_Ground_"));
        }
        if(Time<12.5f)
        {
            ATObservationV81&=Gun->ObserveArmourV48()==Tank;
            ATPlantedTravelV81+=FVector::Dist2D(U->Feet(),Men[0].Previous);Men[0].Previous=U->Feet();
        }
        if(Time<3.f)
        {
            ATSplitV81|=Gun->IsSupported()&&Gun->ObserveArmourV48()==Tank&&!Gun->HasArmourFiringLineV48(Tank);
            ATBlockedShotsV81&=U->ShotsDischarged==0;
        }
        else if(auto* Wall=GroundSightWallV81.Get()){Wall->Destroy();GroundSightWallV81.Reset();}
        // One actual round automatically invokes the ordinary eight-second
        // single-round reload. Suppress only a second shot so its next reload
        // cannot overlap the later, deliberately independent departure test.
        if(U->ShotsDischarged>0)U->ShotClock=10000.f;
        if(U->WeaponPhase==EFrontlineWeaponPhase::Reload)++ATReloadFramesV81;
        if(!Captured&&Time>=12.f){FScreenshotRequest::RequestScreenshot(Directory/TEXT("case6_AT_reload.png"),false,false);Captured=true;}
        if(Time>=12.5f&&!ATDepartedV81)
        {
            ATPacksBeforeDepartureV81=Gun->Packs;ATDepartureFeetV81=U->Feet();ATDepartedV81=true;
            Tank->SetActorLocation(ATStationV81+FVector(B->GetTeamDirection(U->Team)*12000.f,0,0));
        }
        if(Time<15.f)return true;
        const float ActualDeploy=ATDeployReadyV81-ATDeployStartV81;
        Check(TEXT("case6_personal_at_rate_matches_measured_live_deployment"),ATDeployStartV81>=0&&ATExpectedDeployV81>0&&FMath::Abs(ActualDeploy-ATExpectedDeployV81)<=ATMaxStepV81+.05f);
        Check(TEXT("case6_at_observation_survives_blocked_muzzle_and_complete_reload"),ATObservationV81&&ATSplitV81&&ATBlockedShotsV81&&ATReloadFramesV81>30&&U->ReloadsCompleted==1&&U->ShotsDuringReload==0);
        Check(TEXT("case6_at_fires_once_after_real_low_wall_removed"),U->ShotsDischarged==1);
        Check(TEXT("case6_at_remains_planted_without_pack_during_reload"),ATPlantedTravelV81<1.f&&ATPacksBeforeDepartureV81==0&&ATSupportedFramesV81>30);
        Check(TEXT("case6_at_uses_supported_clips_and_resumes_after_range_loss"),ATClipsV81&&Gun->ObserveArmourV48()==nullptr&&Gun->Packs==1&&Gun->State==EFrontlineGunStanceV30::Carry&&FVector::Dist2D(U->Feet(),ATDepartureFeetV81)>30.f);
        auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("case"),6);J->SetStringField(TEXT("scene"),TEXT("AT_reload_and_departure"));
        J->SetNumberField(TEXT("expected_deploy_seconds"),ATExpectedDeployV81);J->SetNumberField(TEXT("actual_deploy_seconds"),ActualDeploy);
        J->SetNumberField(TEXT("shots"),U->ShotsDischarged);J->SetNumberField(TEXT("reload_frames"),ATReloadFramesV81);J->SetNumberField(TEXT("reloads_completed"),U->ReloadsCompleted);
        J->SetNumberField(TEXT("stationary_travel_cm"),ATPlantedTravelV81);J->SetNumberField(TEXT("packs_before_departure"),ATPacksBeforeDepartureV81);J->SetNumberField(TEXT("packs_after_departure"),Gun->Packs);
        J->SetNumberField(TEXT("departure_travel_cm"),FVector::Dist2D(U->Feet(),ATDepartureFeetV81));Cases.Add(MakeShared<FJsonValueObject>(J));Finish();return false;
    }
    void SupportChecks(AWarfareFrontlineGameMode* B)
    {
        const float Clock=B->Elapsed;const auto OldStrikes=B->Strikes;B->Elapsed=1000;B->Strikes.Reset();
        const uint32 OldSupportMask=B->CampaignRuns[0].SupportSelectionMask;
        B->CampaignRuns[0].SupportSelectionMask=(1u<<uint8(EFrontlineSupport::AirRecon))|(1u<<uint8(EFrontlineSupport::AirStrike));
        FrontlineSupportV32::ResetReadinessForDebug(B,0);
        const FVector Target(0,-450,AWarfareFrontlineGround::HeightAt(0,-450));
        const bool Requested=B->RequestSupport(EFrontlineSupport::AirRecon,Target,0);
        bool Airburst=Requested&&!B->Strikes.IsEmpty();
        for(const auto& S:B->Strikes)Airburst&=S.bShrapnelV64&&S.Target.Z-AWarfareFrontlineGround::HeightAt(S.Target.X,S.Target.Y)>=379.f
            &&S.ImpactAt>S.LaunchedAt&&FMath::IsNearlyEqual(S.ImpactAt-S.LaunchedAt,.72f,.001f)
            &&FMath::Abs(S.Target.X)<=FrontlineBattlefield::SupportHalfLength&&FMath::Abs(S.Target.Y)<=FrontlineBattlefield::HalfFrontage;
        Check(TEXT("shrapnel_commits_bounded_timed_airbursts_above_ground"),Airburst);
        Check(TEXT("shrapnel_uses_two_or_three_three_round_salvos"),B->Strikes.Num()==6||B->Strikes.Num()==9);
        bool Fast=true;
        for(auto& S:B->Strikes)
        {
            auto* A=FrontlineSupportArtV36::SpawnShell(B,S);
            Fast&=A&&A->IsVisualReady();
            if(A)A->Destroy();
        }
        Check(TEXT("actual_shrapnel_shells_have_managed_visual_owner"),Fast&&Airburst);
        if(Airburst)
        {
            auto S=B->Strikes[0];B->Elapsed=S.ImpactAt-.18f;auto* Shell=FrontlineSupportArtV36::SpawnShell(B,S);
            if(Shell)
            {
                S.Projectile=Shell;B->Strikes={S};Shell->UpdateVisual(B->Elapsed);const FVector Managed=Shell->GetActorLocation();
                B->Tick(0.f);
                Check(TEXT("game_mode_does_not_overwrite_managed_terminal_shell_flight"),Shell->GetActorLocation().Equals(Managed,.01f));Shell->Destroy();
            }
            else Check(TEXT("game_mode_does_not_overwrite_managed_terminal_shell_flight"),false);
        }
        B->Strikes=OldStrikes;B->Elapsed=Clock;B->CampaignRuns[0].SupportSelectionMask=OldSupportMask;
        auto* Air=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
        const bool Ready=Air&&Air->Configure(B,EFrontlineSupport::AirStrike,Target,0);
        Check(TEXT("actual_strafe_mission_has_visible_aircraft"),Ready);if(!Ready){if(Air)Air->Destroy();return;}
        Air->SetActorTickEnabled(false);bool Forward=true,Deferred=true,Immutable=true;
        for(int32 Pass=0;Pass<2;++Pass)for(int32 I=0;I<150;++I)
        {
            const float T=Air->Arrival+Pass*12+1.15f+I*.0104f;B->Elapsed=Air->RequestedAt+T;
            const int32 Prior=Air->RoundsFired;Air->Strafe(T);if(Air->RoundsFired==Prior)continue;
            const auto& R=Air->PendingStrafe.Last();const auto* Plane=Air->Visuals.IsEmpty()?nullptr:Air->Visuals[0].Get();
            Forward&=Plane&&R.Origin.Equals(Plane->GetComponentTransform().TransformPosition(FVector(287,-31,49)),.05f)
                &&FVector::DotProduct((R.Point-R.Origin).GetSafeNormal(),Plane->GetForwardVector())>.998f;
            Deferred&=R.ImpactAt>B->Elapsed&&R.ImpactAt-B->Elapsed<.20f;
            const FVector Committed=R.Point;const int32 Count=Air->PendingStrafe.Num();Air->Strafe(T);
            Immutable&=Count==Air->PendingStrafe.Num()&&Air->PendingStrafe.Last().Point.Equals(Committed,.001f);
        }
        Check(TEXT("two_real_forward_passes_commit_at_least_fifty_rounds"),Forward&&Air->RoundsFired>=50&&Air->RoundsFired<=64);
        Check(TEXT("strafe_hits_wait_for_immutable_physical_arrival"),Deferred&&Immutable);
        if(!Air->PendingStrafe.IsEmpty())
        {
            const auto Round=Air->PendingStrafe[Air->PendingStrafe.Num()/2];Air->PendingStrafe={Round};
            TArray<AWarfareFrontlineSoldier*> Targets;
            for(int32 I=0;I<16;++I)
            {
                FVector P=Round.Point+FVector((I%4-1.5f)*18,(I/4-1.5f)*18,0);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
                auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)continue;
                U->Initialize(B,751500+I,1,EFrontlineRole::Rifle,0);U->SetActorLocation(P+FVector(0,0,88));U->bFieldEntered=true;U->SetActorTickEnabled(false);B->Units.Add(U);Targets.Add(U);
            }
            B->Elapsed=Round.ImpactAt+.001f;Air->ResolveStrafeRounds();int32 Killed=0;
            for(auto* U:Targets)if(!U->IsAlive())++Killed;
            Check(TEXT("actual_committed_strafe_causes_observable_casualties_at_impact"),Killed>=2&&Air->PendingStrafe.IsEmpty());
        }
        B->Elapsed=Clock;Air->Destroy();
        for(TActorIterator<AFrontlineOrdnanceV36> It(B->GetWorld());It;++It)It->Destroy();
    }
    bool SetupTrain(AWarfareFrontlineGameMode* B)
    {
        Clear(B);FrontlineSpecialistsV41::RunChecksV75(B,[this](const FString& N,bool P){Check(N,P);});SupportChecks(B);Clear(B);
        if(!FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::German,17))return false;
        B->BeginBattle();B->SetPaused(false);B->SetActorTickEnabled(false);B->Elapsed=42;
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Fleet)return false;
        Fleet->NextEnemyAt=100000;Fleet->RailReadyAtV75[0]=0;
        Check(TEXT("German_rail_hero_available_only_in_assigned_rail_operation"),Fleet->RailAvailableV75(0)&&!Fleet->RailAvailableV75(1));
        if(!Fleet->RequestRailV75(0))return false;
        Train=Fleet->Vehicles.Last();if(!Train.IsValid())return false;
        Train->SetActorLocation(FVector(0,-1130,28));Train->bEntered=true;
        Check(TEXT("hero_uses_detailed_six_car_mesh_two_cannons_six_MGs"),Train->AreAssetsReady()&&Train->FootprintHalfLengthV37()>2600);
        Check(TEXT("one_operational_hero_per_army_with_145s_independent_cooldown"),!Fleet->RequestRailV75(0)&&FMath::IsNearlyEqual(Fleet->RailReadyAtV75[0]-B->Elapsed,145.f,.01f));
        for(int32 Side:{-1,1})for(int32 I=0;I<12;++I)
        {
            FVector P(Side*(3700.f+I%3*125),-850+(I/3)*205.f,0);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return false;
            U->Initialize(B,750900+Side*100+I,1,EFrontlineRole::Rifle,0);U->SetActorLocation(P+FVector(0,0,88));
            U->bFieldEntered=true;U->SetActorTickEnabled(false);U->SetMotion(U->GetAimMotion());B->Units.Add(U);
        }
        auto* EnemyTank=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,1,false);
        if(EnemyTank){EnemyTank->SetActorLocation(FVector(-4200,-700,100));EnemyTank->bEntered=true;}
        if(auto* C=PC.Get();C&&C->BattleCamera)
        {
            const FVector Focus(0,-600,900),Eye=Focus+FVector(0,14000,6000);
            C->BattleCamera->SetActorLocation(Eye);C->BattleCamera->SetActorRotation((Focus-Eye).Rotation());
            C->BattleCamera->GetCameraComponent()->OrthoWidth=10000;C->SetViewTarget(C->BattleCamera);
        }
        CaseStartedWorld=LastWorld=B->GetWorld()->GetTimeSeconds();Frames=0;Captured=false;return true;
    }
    void EndTrain(AWarfareFrontlineGameMode* B)
    {
        auto* T=Train.Get();auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        Check(TEXT("live_rail_hero_fires_and_deploys_both_carried_sections"),T&&T->CannonShots>0&&T->MachinegunShots>0&&T->RailHasUnloadedV75()&&T->RailPassengersV75()==0);
        if(!T||!Fleet)return;
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("case"),TEXT("rail_hero"));J->SetNumberField(TEXT("cannon_shots"),T->CannonShots);J->SetNumberField(TEXT("MG_rounds"),T->MachinegunShots);J->SetNumberField(TEXT("hull_integrity"),T->GetHullIntegrity());Cases.Add(MakeShared<FJsonValueObject>(J));
        const float Morale=B->Morale[0];T->ReceiveModuleHit(EFrontlineTankModule::Hull,T->GetHullIntegrity()+1.f);
        Check(TEXT("rail_destruction_applies_heavy_morale_loss_and_disables_collision"),!T->IsOperational()&&Morale-B->Morale[0]>=19.9f&&T->Body->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
        Check(TEXT("wreck_does_not_cancel_long_rearming_period"),!Fleet->RequestRailV75(0));
        B->Elapsed=Fleet->RailReadyAtV75[0]+.01f;
        Check(TEXT("rail_hero_can_be_called_again_after_wreck_and_rearming"),Fleet->RequestRailV75(0));
    }
    void Sample(AWarfareFrontlineGameMode* B,float Time)
    {
        auto Frame=MakeShared<FJsonObject>();Frame->SetNumberField(TEXT("case"),Case);Frame->SetNumberField(TEXT("seconds"),Time);
        TArray<TSharedPtr<FJsonValue>> Rows;
        for(const auto& Man:Men)if(auto* U=Man.Unit.Get())
        {
            auto J=MakeShared<FJsonObject>();auto* G=FrontlineMachinegunsV30::Get(U);
            bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);
            J->SetNumberField(TEXT("squad"),U->SquadId);J->SetNumberField(TEXT("member"),U->MemberIndex);
            J->SetStringField(TEXT("feet"),U->Feet().ToString());J->SetNumberField(TEXT("distance_to_slot"),FVector::Dist2D(U->Feet(),Goal));
            J->SetNumberField(TEXT("stance"),G?int32(G->State):-1);J->SetNumberField(TEXT("packs"),G?G->Packs:0);
            J->SetBoolField(TEXT("wanted_movement"),U->bWantedMovement);J->SetBoolField(TEXT("arrived"),U->bAtDestinationV43);
            J->SetNumberField(TEXT("requested_speed"),U->RequestedTravelSpeedV43);J->SetNumberField(TEXT("departure_clock"),U->DepartureClock);
            J->SetNumberField(TEXT("fire_pose_clock"),U->FirePoseClock);J->SetNumberField(TEXT("fall_speed"),U->TrenchFallSpeedV69);
            J->SetNumberField(TEXT("motion_speed"),U->GetMotionSpeed());J->SetNumberField(TEXT("shots"),U->ShotsDischarged);
            J->SetStringField(TEXT("motion"),GetNameSafe(U->Playing));J->SetNumberField(TEXT("cover"),B->CoverFor(U));
            const auto* Target=B->AcquireTarget(U);J->SetBoolField(TEXT("sees_enemy"),Target!=nullptr);
            J->SetBoolField(TEXT("muzzle_clear"),G&&Target&&G->HasInfantryFiringLine(Target));
            J->SetBoolField(TEXT("validated_firing_bay"),FrontlineMachinegunsV30::HasValidatedTrenchBayV69(U));
            Rows.Add(MakeShared<FJsonValueObject>(J));
        }
        Frame->SetArrayField(TEXT("men"),Rows);Timeline.Add(MakeShared<FJsonValueObject>(Frame));
    }
    void FixedBayCornerCheckV81(AWarfareFrontlineGameMode* B)
    {
        auto* U=Men.IsEmpty()?nullptr:Men[0].Unit.Get();auto* G=FrontlineMachinegunsV30::Get(U);
        auto* S=U?B->FindSquad(U->SquadId):nullptr;
        if(!U||!G||!S){Check(TEXT("case0_fixed_small_step_corner_fixture"),false);return;}
        // The actual Shipping failure coordinate in operation17. The live
        // seven-case review has already finished this case; next Setup clears
        // these disposable actors. No screenshot or world hitch can advance
        // this independent ordinary Tick/skeleton/component regression.
        FVector From(-3830.316,-918.918,0);From.Z=FrontlineInfantryV45::SurfaceHeight(B,From);
        U->ReturnToIdle();G->State=EFrontlineGunStanceV30::Carry;G->StanceClock=0;
        U->SetActorLocation(From+FVector(0,0,88));U->SetActorRotation(FRotator::ZeroRotator);
        U->ShotClock=U->AimSettleClock=0;U->bWasSettled=false;U->DepartureClock=0;
        U->SetMotion(U->GetWalkMotion());G->Update(1.f/120.f,true);FrontlineMobilityV41::Reset(B);
        bool Reform=false;const FVector InitialGoal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);
        const FVector Heading=(InitialGoal-From).GetSafeNormal2D();
        bool DirectBlocked=false;
        for(float Probe:{1.f,2.f,3.f,4.f})DirectBlocked|=!FrontlineInfantryV45::AllowsTrenchStep(U,From,From+Heading*Probe);
        const int32 Shots=U->ShotsDischarged,Packs=G->Packs;
        constexpr float DT=1.f/120.f;float Arrival=-1,FirstShot=-1,MaxStep=0,MaxIntrusion=0;int32 CornerFrames=0;
        bool Finite=true,PlantedMotion=true;
        for(;CornerFrames<1440;++CornerFrames)
        {
            B->Elapsed+=DT;const FVector Before=U->Feet();U->Tick(DT);
            U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();G->TickComponent(DT,LEVELTICK_All,nullptr);
            const FVector Feet=U->Feet();Finite&=!Feet.ContainsNaN();
            MaxStep=FMath::Max(MaxStep,float(FVector::Dist2D(Before,Feet)));
            MaxIntrusion=FMath::Max(MaxIntrusion,FrontlineInfantryV45::BankIntrusion(U,Feet));
            bool LocalReform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),LocalReform);
            if(Arrival<0&&FVector::Dist2D(Feet,Goal)<=3.1f&&G->IsSupported())Arrival=(CornerFrames+1)*DT;
            if(U->GetMotionSpeed()<.1f)PlantedMotion&=U->Playing!=U->GetWalkMotion()&&U->Playing!=U->GetRunMotion();
            if(U->ShotsDischarged>Shots){FirstShot=(CornerFrames+1)*DT;++CornerFrames;break;}
        }
        auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("case"),0);J->SetStringField(TEXT("scene"),TEXT("fixed_small_step_bank_corner"));
        J->SetStringField(TEXT("from"),From.ToString());J->SetStringField(TEXT("initial_goal"),InitialGoal.ToString());J->SetBoolField(TEXT("direct_step_blocked"),DirectBlocked);
        J->SetNumberField(TEXT("dt_seconds"),DT);J->SetNumberField(TEXT("frames"),CornerFrames);J->SetNumberField(TEXT("arrival_seconds"),Arrival);J->SetNumberField(TEXT("first_shot_seconds"),FirstShot);
        J->SetNumberField(TEXT("max_step_cm"),MaxStep);J->SetNumberField(TEXT("max_bank_intrusion_cm"),MaxIntrusion);J->SetNumberField(TEXT("new_packs"),G->Packs-Packs);Cases.Add(MakeShared<FJsonValueObject>(J));
        Check(TEXT("case0_fixed_small_step_corner_fixture"),DirectBlocked&&FrontlineMachinegunsV30::HasValidatedTrenchBayV69(U)&&FVector::Dist2D(From,InitialGoal)<40.f);
        Check(TEXT("case0_fixed_small_step_corner_reaches_bay_and_actually_fires"),Arrival>=0&&Arrival<12.f&&FirstShot>=0&&FirstShot<12.f);
        Check(TEXT("case0_fixed_small_step_corner_preserves_bank_gate_speed_and_stance"),Finite&&MaxStep<4.f&&MaxIntrusion<=8.1f&&G->Packs==Packs&&PlantedMotion);
    }
    void EndCase(AWarfareFrontlineGameMode* B,float Time)
    {
        const bool Ground=StanceOnlyV81&&Case>=4;
        auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("case"),Case);J->SetNumberField(TEXT("team"),Ground?Case-4:Case/2);
        J->SetStringField(TEXT("scene"),Ground?TEXT("ground_section"):TEXT("trench_entry"));
        J->SetNumberField(TEXT("world_seconds"),Time);J->SetNumberField(TEXT("frames"),Frames);
        bool Fire=true,Settled=true,NoCycle=true,Finite=true,Accurate=true;
        bool CompanionFacing=true,StationStable=true;TSet<int32> PersonalDeployStarts;
        TArray<TSharedPtr<FJsonValue>> Rows;
        for(const auto& M:Men)if(auto* U=M.Unit.Get())
        {
            auto R=MakeShared<FJsonObject>();auto* G=FrontlineMachinegunsV30::Get(U);
            bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);
            const float Error=FVector::Dist2D(Goal,U->Feet());
            R->SetNumberField(TEXT("squad"),U->SquadId);R->SetNumberField(TEXT("member"),U->MemberIndex);
            R->SetStringField(TEXT("mesh"),GetPathNameSafe(U->Soldier->GetSkeletalMeshAsset()));
            R->SetStringField(TEXT("material0"),GetPathNameSafe(U->Soldier->GetMaterial(0)));
            R->SetNumberField(TEXT("shots"),U->ShotsDischarged);R->SetNumberField(TEXT("first_shot_seconds"),M.FirstShot);
            R->SetNumberField(TEXT("slot_error_cm"),Error);R->SetNumberField(TEXT("last_3s_travel_cm"),M.LateTravel);
            R->SetNumberField(TEXT("pre_capture_9_to_12s_travel_cm"),M.PreCaptureTravel);
            R->SetNumberField(TEXT("movement_state_flips"),M.MoveFlips);R->SetNumberField(TEXT("max_z_step_cm"),M.MaxVerticalStep);
            R->SetNumberField(TEXT("max_drop_cm_at_dt_below_33ms"),M.SmallStepMaxDrop);
            R->SetNumberField(TEXT("max_bank_intrusion_cm"),M.MaxBankIntrusion);R->SetNumberField(TEXT("packs"),G?G->Packs:0);
            R->SetNumberField(TEXT("deployments"),G?G->Deployments:0);Rows.Add(MakeShared<FJsonValueObject>(R));
            R->SetNumberField(TEXT("late_packs_after_12s"),M.LatePacks);R->SetNumberField(TEXT("support_station_drift_cm"),M.MaxSupportGoalDrift);
            R->SetNumberField(TEXT("first_deployment_seconds"),M.FirstDeployment);
            if(M.FirstDeployment>=0)PersonalDeployStarts.Add(FMath::RoundToInt(M.FirstDeployment*100.f));
            Finite&=!U->Feet().ContainsNaN();Settled&=M.LateTravel<12.f;
            if(U->IsMachineGunner())
            {Fire&=U->ShotsDischarged>0&&M.FirstShot>=0&&M.FirstShot<12.f;NoCycle&=G&&G->Packs<=1&&G->IsTrench()!=Ground&&G->IsSupported();Accurate&=Ground?M.LateTravel<12.f:Error<=3.1f;}
            if(StanceOnlyV81&&G)NoCycle&=M.LatePacks==0;
            if(Ground&&!U->IsMachineGunner())
            {
                StationStable&=M.HasSupportGoal&&M.MaxSupportGoalDrift<.1f;
                const auto* Target=U->FocusTarget.Get();
                CompanionFacing&=Target&&FMath::Abs(FMath::FindDeltaAngleDegrees(U->GetActorRotation().Yaw,(Target->Feet()-U->Feet()).Rotation().Yaw))<8.f;
                NoCycle&=G&&G->IsSupported()&&G->Packs<=1;
            }
            UE_LOG(LogTemp,Display,TEXT("WF_V75_GAMEPLAY_MAN case=%d squad=%d member=%d shots=%d first=%.2f slot_error=%.2f late_travel=%.2f flips=%d packs=%d deploys=%d"),
                Case,U->SquadId,U->MemberIndex,U->ShotsDischarged,M.FirstShot,Error,M.LateTravel,M.MoveFlips,G?G->Packs:0,G?G->Deployments:0);
        }
        J->SetArrayField(TEXT("men"),Rows);Cases.Add(MakeShared<FJsonValueObject>(J));
        const FString Prefix=FString::Printf(TEXT("case%d_"),Case);
        Check(Prefix+(Ground?TEXT("primary_mg_fires_after_low_wall_removed"):TEXT("every_primary_mg_fires_in_real_trench")),Fire);
        Check(Prefix+TEXT("all_members_finish_entry_without_late_orbit"),Settled&&Finite);
        Check(Prefix+TEXT("gun_deployment_does_not_pack_loop"),NoCycle);
        Check(Prefix+(Ground?TEXT("primary_ground_gun_stays_planted"):TEXT("gun_reaches_the_validated_firing_bay")),Accurate);
        if(Ground)
        {
            Check(Prefix+TEXT("standing_section_observation_survives_blocked_prone_muzzle"),GroundSplitObservedV81&&GroundBlockedShotsV81);
            Check(Prefix+TEXT("assistants_keep_actual_station_through_gun_sector_change"),StationStable);
            Check(Prefix+TEXT("companions_face_the_hostile_contact_after_reforming"),CompanionFacing);
            Check(Prefix+TEXT("section_deployments_have_distinct_personal_start_times"),PersonalDeployStarts.Num()>=2);
            AWarfareFrontlineSoldier* Primary=nullptr;
            for(const auto& M:Men)if(auto* U=M.Unit.Get();U&&U->IsMachineGunner()){Primary=U;break;}
            auto* PrimarySupport=FrontlineMachinegunsV30::Get(Primary);bool DepartureReleasesStation=PrimarySupport!=nullptr;
            if(PrimarySupport)
            {
                PrimarySupport->Update(.01f,true);
                for(const auto& M:Men)if(auto* U=M.Unit.Get();U&&!U->IsMachineGunner())
                {
                    bool Reform=false;const FVector March=B->SoldierDestination(U);
                    DepartureReleasesStation&=FrontlineMachinegunsV30::Destination(U,March,Reform).Equals(March,.01f)&&!Reform;
                }
                PrimarySupport->Update(.01f,false);
            }
            Check(Prefix+TEXT("a_real_departure_releases_cached_support_stations"),DepartureReleasesStation);
        }
        Check(Prefix+TEXT("world_clock_and_frames_advanced"),Time>=15.f&&Frames>=120);
        if(Case%2==1&&!Ground)
        {
            // Keep the physical obstruction gate: making a bay reachable must
            // never be confused with allowing bullets through real geometry.
            AWarfareFrontlineSoldier* Gunner=nullptr;
            for(const auto& M:Men)if(auto* U=M.Unit.Get();U&&U->IsMachineGunner()){Gunner=U;break;}
            auto* Gun=FrontlineMachinegunsV30::Get(Gunner);
            auto* Target=Gunner?B->AcquireTarget(Gunner):nullptr;
            const bool Precondition=Gun&&Gun->CanFire()&&Target&&Gun->HasInfantryFiringLine(Target);
            bool WallBlocks=false;
            if(Precondition)
            {
                auto* Wall=B->GetWorld()->SpawnActor<AActor>();
                if(Wall)
                {
                    auto* Block=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Block);Wall->AddInstanceComponent(Block);
                    Block->SetBoxExtent(FVector(30,2000,2000));Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                    Block->SetCollisionResponseToAllChannels(ECR_Ignore);Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Block->RegisterComponent();
                    Wall->SetActorLocation((Gunner->GetMuzzleLocation()+Target->Feet()+FVector(0,0,135))*.5f);
                    const int32 Shots=Gunner->ShotsDischarged;const auto OldPhase=Gunner->WeaponPhase;
                    Gunner->WeaponPhase=EFrontlineWeaponPhase::Ready;Gunner->DischargeWeapon(Target);Gunner->WeaponPhase=OldPhase;
                    WallBlocks=!Gun->HasInfantryFiringLine(Target)&&!B->HasSight(Gunner,Target)&&Gunner->ShotsDischarged==Shots;
                    Wall->Destroy();
                }
            }
            Check(Prefix+TEXT("real_wall_still_blocks_a_previously_clear_gun"),Precondition&&WallBlocks);
        }
        if(StanceOnlyV81&&Case==0)FixedBayCornerCheckV81(B);
    }
    bool Tick(float)
    {
        auto* B=Battle();if(!B){Check(TEXT("battle_exists"),false);Finish();return false;}
        if(FPlatformTime::Seconds()-Started>(StanceOnlyV81?180.:130.)){Check(TEXT("bounded_review_completes"),false);Finish();return false;}
        if(Phase==0)
        {
            if(FPlatformTime::Seconds()-Started<3.)return true;
            Check(TEXT("disposable_profile"),B->IsProgressionVolatile());
            if(!B->IsProgressionVolatile()||!Setup()){Check(TEXT("live_fixture_setup"),false);Finish();return false;}
            if(EdgeStationsOnlyV81){FixedEdgeStationChecksV81(B);Finish();return false;}
            if(BanksOnly)
            {
                Case=1;if(!Setup()){Check(TEXT("British_bank_setup"),false);Finish();return false;}
                Case=3;if(!Setup())Check(TEXT("German_bank_setup"),false);
                Finish();return false;
            }
            Phase=1;return true;
        }
        const double Now=B->GetWorld()->GetTimeSeconds();const float DT=float(Now-LastWorld);
        if(DT<=0)return true;LastWorld=Now;const float Time=float(Now-CaseStartedWorld);++Frames;
        // Actors, animation/component ticks, world time and particles remain
        // live. Only strategic AI/win processing is excluded from this fixture.
        B->Elapsed+=DT;B->UpdateSquads();
        if(FormationOnlyV8213)return TickFormationV8213(B,Time,DT);
        if(StanceOnlyV81&&Case==6)return TickATV81(B,Time,DT);
        if(Case==4&&!StanceOnlyV81)
        {
            if(!Captured&&Time>=12.f){FScreenshotRequest::RequestScreenshot(Directory/TEXT("rail_hero.png"),false,false);Captured=true;}
            if(Time<20.f)return true;EndTrain(B);Finish();return false;
        }
        for(const auto& Entry:MovingTargets)if(auto* U=Entry.Key.Get())
        {FVector P=Entry.Value;P.Y+=StanceOnlyV81&&Case>=4?(Time>=8.f?750.f:0.f):FMath::Sin(Time*.45f)*160.f;P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));}
        if(StanceOnlyV81&&Case>=4)
        {
            if(Time>=6.f)if(auto* Wall=GroundSightWallV81.Get()){Wall->Destroy();GroundSightWallV81.Reset();}
            if(Time<6.f)for(const auto& M:Men)if(auto* U=M.Unit.Get();U&&U->IsMachineGunner())
            {
                auto* G=FrontlineMachinegunsV30::Get(U);auto* Target=B->AcquireTarget(U,true);
                GroundSplitObservedV81|=G&&G->IsSupported()&&Target&&!G->HasInfantryFiringLine(Target);
                GroundBlockedShotsV81&=U->ShotsDischarged==0;
            }
        }
        for(auto& M:Men)if(auto* U=M.Unit.Get())
        {
            const FVector P=U->Feet();const float Travel=FVector::Dist2D(P,M.Previous);
            if(Time>=12.f)M.LateTravel+=Travel;
            if(Time>=9.f&&Time<12.f)M.PreCaptureTravel+=Travel;
            M.MaxVerticalStep=FMath::Max(M.MaxVerticalStep,FMath::Abs(float(P.Z-M.Previous.Z)));
            if(DT<=1.f/30.f)M.SmallStepMaxDrop=FMath::Max(M.SmallStepMaxDrop,float(M.Previous.Z-P.Z));
            if(B->CoverFor(U)>0)M.MaxBankIntrusion=FMath::Max(M.MaxBankIntrusion,FrontlineInfantryV45::BankIntrusion(U,P));
            if(M.WasMoving!=U->bMoving)++M.MoveFlips;M.WasMoving=U->bMoving;M.Previous=P;++M.Frames;
            if(U->ShotsDischarged>0&&M.FirstShot<0)M.FirstShot=Time;
            if(auto* G=FrontlineMachinegunsV30::Get(U))
            {
                if(G->State==EFrontlineGunStanceV30::Deploy&&M.FirstDeployment<0)M.FirstDeployment=Time;
                if(Time>=12.f)M.LatePacks+=FMath::Max(0,G->Packs-M.PriorPacks);M.PriorPacks=G->Packs;
                if(StanceOnlyV81&&Case>=4&&!U->IsMachineGunner()&&G->IsSupported())
                {
                    bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);
                    if(!M.HasSupportGoal){M.InitialSupportGoal=Goal;M.HasSupportGoal=true;}
                    M.MaxSupportGoalDrift=FMath::Max(M.MaxSupportGoalDrift,FVector::Dist2D(Goal,M.InitialSupportGoal));
                }
            }
        }
        if(Time>=NextSample){Sample(B,Time);NextSample=Time+.25f;}
        if(!BeforeCaptureChecked&&Time>=12.f)
        {
            bool Ready=true,Fire=true;
            for(const auto& M:Men)if(auto* U=M.Unit.Get())
            {
                bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);
                const float Limit=U->IsMachineGunner()?3.1f:FrontlineMachinegunsV30::Get(U)?14.1f:24.1f;
                Ready&=M.PreCaptureTravel<12.f&&((StanceOnlyV81&&Case>=4)?M.LateTravel<12.f:FVector::Dist2D(Goal,U->Feet())<=Limit);
                if(U->IsMachineGunner())Fire&=U->ShotsDischarged>0;
            }
            const FString Prefix=FString::Printf(TEXT("case%d_"),Case);
            Check(Prefix+TEXT("full_crew_settled_by_12s_before_any_screenshot"),Ready);
            Check(Prefix+TEXT("primary_guns_fired_by_12s_before_any_screenshot"),Fire);
            BeforeCaptureChecked=true;
        }
        if(!Captured&&Time>=13.f)
        {FScreenshotRequest::RequestScreenshot(Directory/FString::Printf(TEXT("case%d.png"),Case),false,false);Captured=true;}
        if(Time<15.f)return true;
        EndCase(B,Time);++Case;
        if(!StanceOnlyV81&&Case>=4){if(!SetupTrain(B)){Check(TEXT("rail_hero_live_fixture_setup"),false);Finish();return false;}return true;}
        if(!Setup()){Check(TEXT("next_case_setup"),false);Finish();return false;}return true;
    }
};

void StartFrontlineGameplayReviewV75(AWarfareFrontlineController* C)
{
    FString UserDir;
    if(!C||!FParse::Param(FCommandLine::Get(),TEXT("WFV75GameplayReview"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
    auto R=MakeShared<FFrontlineGameplayReviewV75>();R->PC=C;R->Started=FPlatformTime::Seconds();
    R->BanksOnly=FParse::Param(FCommandLine::Get(),TEXT("WFV75BanksOnly"));
    R->StanceOnlyV81=FParse::Param(FCommandLine::Get(),TEXT("WFV81StanceOnly"));
    R->EdgeStationsOnlyV81=FParse::Param(FCommandLine::Get(),TEXT("WFV81EdgeStationsOnly"));
    R->FormationOnlyV8213=FParse::Param(FCommandLine::Get(),TEXT("WFV8213FormationOnly"));
    FParse::Value(FCommandLine::Get(),TEXT("WFV75Operation="),R->Operation);R->Operation=FMath::Clamp(R->Operation,0,19);
    R->Directory=FPaths::ProjectSavedDir()/(R->FormationOnlyV8213?TEXT("Gameplay/V8213/FormationReview"):TEXT("Gameplay/V75/Review"));IFileManager::Get().MakeDirectory(*R->Directory,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}
