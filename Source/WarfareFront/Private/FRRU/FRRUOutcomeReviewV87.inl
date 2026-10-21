#if WITH_EDITOR
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineCharacterV34.h"
#include "WarfareFrontlineUIReviewAccessV8447.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/SkeletalMesh.h"
#include "Components/StaticMeshComponent.h"
namespace FRRUOutcomeReviewV87
{
struct FObserved
{
    TWeakObjectPtr<AWarfareFrontlineSoldier> Unit;TWeakObjectPtr<UStaticMeshComponent> Released;
    FString Body,Skeleton,Last;TSet<FString> Clips;TMap<FString,int32> Starts;
    int32 Segments=0,ReleaseMaximum=0;float LastTime=-1;bool Skeletons=true,Detached=true,FollowStopped=true,StableEmplacement=true;
    bool ReleaseSeen=false,HadSupportedMount=false;FTransform MountBefore,ReleasedAt;
};
struct FRun
{
    FString User;TWeakObjectPtr<AWarfareFrontlineController> PC;TArray<TSharedPtr<FJsonValue>> Checks,Actors,Captures;
    TArray<FObserved> Observed;TSet<int32> Nations;int32 Country=0,Round=0,Stage=0,PendingShot=INDEX_NONE;
    int32 Recruits[2][2]={{INDEX_NONE,INDEX_NONE},{INDEX_NONE,INDEX_NONE}};
    double Began=FPlatformTime::Seconds(),At=0,Next=0;float Outro=0;uint64 CameraFrame=0,ClockFrame=MAX_uint64;
    bool Good=true,Held=false,PumpShot=false,SaluteShot=false,SurrenderShot=false,OldHUD=true;
    TWeakObjectPtr<AWarfareFrontlineSoldier> CameraUnit;
    EFRRUNationV81 Nation()const{return Country==0?EFRRUNationV81::French:EFRRUNationV81::Russian;}
    FString Prefix()const{return FString(Country==0?TEXT("fr"):TEXT("ru"))+(Round?TEXT("-restart"):TEXT("-normal"));}
    void Check(const FString& Name,bool Pass)
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Prefix()+TEXT(".")+Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;
        if(!Pass)UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_OUTCOME_CHECK_FAILED %s.%s"),*Prefix(),*Name);}
    void Clear(AWarfareFrontlineController* C)
    {
        auto& S=ScreenState(C);S.Menu=S.Settings=S.Help=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=false;
        S.Dossier=1917;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.PreparationError.Empty();C->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);
        UGameplayStatics::SetGamePaused(C,false);
    }
    bool Finish()
    {
        auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;
        if(C&&C->GetHUD())C->GetHUD()->bShowHUD=OldHUD;
        if(B){B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;B->Phase=EFrontlinePhase::Fighting;B->SetActorTickEnabled(false);}
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("actors"),Actors);J->SetArrayField(TEXT("captures"),Captures);
        TArray<TSharedPtr<FJsonValue>> ActualNations;for(int32 N:Nations)ActualNations.Add(MakeShared<FJsonValueNumber>(N));J->SetArrayField(TEXT("actual_nations_observed"),ActualNations);
        J->SetBoolField(TEXT("controlled_outcome_pose_fixture"),true);J->SetBoolField(TEXT("natural_victory_accepted"),false);J->SetBoolField(TEXT("called_finish_or_submit_outcome"),false);
        J->SetStringField(TEXT("scope"),TEXT("Public national Start/scene preparation/Recruit and Restart actors, actual body/animation Skeleton identity and real actor/SingleNode ticks. Only the controlled BattleOutroElapsed clock advances after battle decisions are frozen; host Elapsed is never assigned. No FinishBattle, SubmitOutcome, synthetic honours or natural win proof."));
        J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Began);
        FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));if(!FFileHelper::SaveStringToFile(Text,*(User/TEXT("outcome-checks.json"))))Good=false;
        V28ReviewPointer.Reset();FPlatformMisc::RequestExitWithStatus(false,Good?0:1);return false;
    }
    int32 Recruit(AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Team)
    {
        if(!B->RecruitBlockReason(Role,Team).IsEmpty())return INDEX_NONE;
        TSet<int32> Before;for(const auto& S:B->Squads)Before.Add(S.Id);
        if(!B->Recruit(Role,Team))return INDEX_NONE;
        for(const auto& S:B->Squads)if(S.Team==Team&&S.Role==Role&&!Before.Contains(S.Id))return S.Id;
        return INDEX_NONE;
    }
    bool Ready(AWarfareFrontlineGameMode* B)
    {
        bool HaveAll=true;
        for(int32 Team=0;Team<2;++Team)for(int32 Kind=0;Kind<2;++Kind)
        {
            // Opponent sections can also be issued by its normal doctrine,
            // which uses the same public Recruit command and shared cooldown.
            if(Team==1&&Recruits[Team][Kind]==INDEX_NONE)
                for(const auto& Q:B->Squads)if(Q.Team==Team&&Q.Role==(Kind==0?EFrontlineRole::Lewis:EFrontlineRole::Rifle)&&Q.LivingCount()>0){Recruits[Team][Kind]=Q.Id;break;}
            if(Recruits[Team][Kind]==INDEX_NONE)Recruits[Team][Kind]=Recruit(B,Kind==0?EFrontlineRole::Lewis:EFrontlineRole::Rifle,Team);
            HaveAll&=Recruits[Team][Kind]!=INDEX_NONE;
        }
        if(!Held&&Recruits[0][0]!=INDEX_NONE)if(const auto* Gun=B->FindSquad(Recruits[0][0]))
        {
            bool Entered=Gun->LivingCount()>0;
            for(const auto& W:Gun->Members)if(const auto* U=W.Get();U&&U->IsAlive())Entered&=U->bFieldEntered;
            // Hold is a real movement order. Issuing it in the reserve lane
            // would correctly stop that squad before it crossed the field.
            if(Entered)Held=WarfareFRRUWorldV82::HoldSquad(B,Recruits[0][0],true);
        }
        if(!HaveAll)return false;
        for(int32 Team=0;Team<2;++Team)for(int32 Kind=0;Kind<2;++Kind)
        {
            const auto* S=B->FindSquad(Recruits[Team][Kind]);if(!S||S->LivingCount()<=0)return false;
            for(const auto& W:S->Members)if(const auto* U=W.Get();U&&U->IsAlive())if(!U->bFieldEntered)return false;
        }
        return true;
    }
    bool Binding(AWarfareFrontlineSoldier* U,FObserved& O)
    {
        auto* Mesh=U&&U->Soldier?U->Soldier->GetSkeletalMeshAsset():nullptr;auto* Skeleton=Mesh?Mesh->GetSkeleton():nullptr;
        if(!Mesh||!Skeleton||Mesh->GetRefSkeleton().GetNum()!=67)return false;
        O.Body=Mesh->GetPathName();O.Skeleton=Skeleton->GetPathName();
        for(auto* A:{U->GetIdleMotion(),U->GetAimMotion(),U->GetReloadMotion(),U->GetWalkMotion(),U->GetRunMotion()})if(!A||A->GetSkeleton()!=Skeleton)return false;
        return true;
    }
    void Sample(AWarfareFrontlineGameMode* B)
    {
        for(auto& O:Observed)if(auto* U=O.Unit.Get())
        {
            auto* Mesh=U->Soldier?U->Soldier->GetSkeletalMeshAsset():nullptr;auto* N=U->Soldier?U->Soldier->GetSingleNodeInstance():nullptr;auto* A=N?N->GetAnimationAsset():nullptr;
            O.Skeletons&=Mesh&&Mesh->GetPathName()==O.Body&&A&&A->GetSkeleton()==Mesh->GetSkeleton();
            const FString Clip=GetPathNameSafe(A);
            if(Clip!=O.Last){O.Last=Clip;O.Clips.Add(Clip);if(Clip.Contains(TEXT("A_Victory_"))||Clip.Contains(TEXT("A_Surrender_"))){++O.Segments;++O.Starts.FindOrAdd(Clip);}}
            else if(N&&N->GetCurrentTime()+.15f<O.LastTime&&(Clip.Contains(TEXT("A_Victory_"))||Clip.Contains(TEXT("A_Surrender_")))){++O.Segments;++O.Starts.FindOrAdd(Clip);}
            O.LastTime=N?N->GetCurrentTime():-1;
            TInlineComponentArray<UStaticMeshComponent*> Parts(U);UStaticMeshComponent* Released=nullptr;int32 Count=0;
            for(auto* C:Parts)if(C->ComponentHasTag(TEXT("WF_V34_RELEASED_WEAPON"))){++Count;Released=C;}
            O.ReleaseMaximum=FMath::Max(O.ReleaseMaximum,Count);
            if(Released)
            {
                if(!O.ReleaseSeen){O.ReleaseSeen=true;O.Released=Released;O.ReleasedAt=Released->GetComponentTransform();}
                O.Detached&=Count==1&&O.Released.Get()==Released&&Released->GetAttachParent()==nullptr;
                const auto Before=Released->GetComponentTransform();WarfareFRRUPresentationV82::TickUnit(U,.016f);WarfareFRRUPresentationV82::TickUnit(U,.016f);
                O.FollowStopped&=Released->GetComponentTransform().Equals(Before,.001f)&&!WarfareFRRUPresentationV82::HasBoundWeapon(U);
                if(U->IsMachineGunner())O.StableEmplacement&=!Released->IsSimulatingPhysics()&&Released->GetComponentTransform().Equals(O.ReleasedAt,.1f);
            }
        }
    }
    AWarfareFrontlineSoldier* FindShot(AWarfareFrontlineGameMode* B,int32 Kind)
    {
        for(auto& O:Observed)if(auto* U=O.Unit.Get();U&&U->Soldier)
        {
            auto* N=U->Soldier->GetSingleNodeInstance();if(!N||N->GetCurrentTime()<.8f||N->GetCurrentTime()>N->GetLength()-.35f)continue;const FString Name=GetNameSafe(N->GetAnimationAsset());
            if(Kind<2&&U->Team==B->BattleOutroWinner&&Name.Contains(Kind==0?TEXT("OneArmPump"):TEXT("OneArmSalute")))return U;
            if(Kind==2&&U->Team!=B->BattleOutroWinner&&U->IsMachineGunner()&&O.ReleaseSeen)return U;
        }
        return nullptr;
    }
    void CompleteRound(AWarfareFrontlineGameMode* B)
    {
        bool Bindings=true,Loops=true,Release=true,MG=false,Pump=false,Salute=false;
        const bool CoatV2=FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87MainCoatV2Candidate"));int32 CoatActors=0;bool CoatLifecycle=true,CoatDefaults=true,CoatMips=true;
        for(const auto& O:Observed)if(auto* U=O.Unit.Get())
        {
            Bindings&=O.Skeletons;
            for(const auto& Clip:O.Clips){Pump|=Clip.Contains(TEXT("OneArmPump_V828_Human67_V82"));Salute|=Clip.Contains(TEXT("OneArmSalute_V828_Human67_V82"));}
            if(U->Role==EFrontlineRole::Rifle)Loops&=O.Segments>=2;
            if(U->Team!=B->BattleOutroWinner)
            {
                Release&=O.ReleaseSeen&&O.ReleaseMaximum==1&&O.Detached&&O.FollowStopped;
                if(U->IsMachineGunner())
                {auto Probe=WarfareFRRUPresentationV82::ProbeUnit(U,false);MG|=Probe->GetBoolField(TEXT("releasedWeaponLeftEmplacedV87"))&&O.StableEmplacement;}
            }
            auto Row=WarfareFRRUPresentationV82::ProbeUnit(U,false);Row->SetStringField(TEXT("round"),Prefix());Row->SetStringField(TEXT("body_at_start"),O.Body);Row->SetStringField(TEXT("skeleton_at_start"),O.Skeleton);
            const auto Nation=WarfareFRRUCampaignV82::Nation(U);
            if(CoatV2&&(Nation==EFRRUNationV81::French||Nation==EFRRUNationV81::Russian||Nation==EFRRUNationV81::AustroHungarian))
            {++CoatActors;CoatLifecycle&=Row->GetBoolField(TEXT("nationalCoatV2ActualMIDLifecycleBound"));CoatDefaults&=Row->GetBoolField(TEXT("nationalCoatV2NewActorDefaults"));CoatMips&=Row->GetBoolField(TEXT("nationalCoatV2ActualMipChain"));}
            Row->SetNumberField(TEXT("observed_outcome_segments"),O.Segments);Row->SetNumberField(TEXT("released_component_maximum"),O.ReleaseMaximum);
            Row->SetBoolField(TEXT("released_weapon_detached_once"),O.Detached);Row->SetBoolField(TEXT("native_tick_no_longer_follows_weapon"),O.FollowStopped);Row->SetBoolField(TEXT("stable_emplaced_gun"),O.StableEmplacement);
            Row->SetBoolField(TEXT("supported_mount_snapshot_available"),O.HadSupportedMount);
            if(O.HadSupportedMount&&O.ReleaseSeen)Row->SetNumberField(TEXT("released_mount_xy_displacement_cm"),FVector::Dist2D(O.MountBefore.GetLocation(),O.ReleasedAt.GetLocation()));
            TArray<TSharedPtr<FJsonValue>> Clips;for(const auto& Clip:O.Clips)Clips.Add(MakeShared<FJsonValueString>(Clip));Row->SetArrayField(TEXT("actual_outcome_clips"),Clips);Actors.Add(MakeShared<FJsonValueObject>(Row));
        }
        Check(TEXT("current_actor_body_and_actual_clip_skeleton_match"),Bindings);Check(TEXT("both_new_human67_single_arm_gestures_observed"),Pump&&Salute);
        Check(TEXT("rifle_victory_and_surrender_continue_real_gesture_cycles"),Loops);Check(TEXT("losers_release_exact_actual_weapon_once_and_stop_following"),Release);
        Check(TEXT("crew_served_machinegun_stays_emplaced"),MG);Check(TEXT("three_real_actor_captures"),PumpShot&&SaluteShot&&SurrenderShot);
        if(CoatV2){Check(TEXT("national_coat_actual_mid_and_shared_lifecycle_parameters"),CoatActors>=9&&CoatLifecycle);Check(TEXT("national_coat_normal_or_restart_new_actor_defaults"),CoatActors>=9&&CoatDefaults);Check(TEXT("national_coat_actual_source_platform_and_resident_mips"),CoatActors>=9&&CoatMips);}
        FFRRUCampaignSummaryV82 Record;Check(TEXT("pose_fixture_does_not_credit_campaign_result"),WarfareFRRUCampaignV82::CurrentRecord(B,Record)&&Record.Victories==0&&Record.Defeats==0&&Record.Completed==0);
    }
    bool Tick(float)
    {
        const double Now=FPlatformTime::Seconds();if(Now-Began>360){Check(TEXT("outcome_review_bounded_360_seconds"),false);return Finish();}
        auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;
        if(!C||!B)
        {if(GEngine)for(const auto& X:GEngine->GetWorldContexts())if(auto* W=X.World();W&&W->WorldType==EWorldType::Game)if(auto* P=Cast<AWarfareFrontlineController>(W->GetFirstPlayerController());P&&P->HasActorBegunPlay())if(auto* M=FrontlineFor(P);M&&M->HasActorBegunPlay()){PC=P;break;}return true;}
        FString Actual;if(!FRRUParityReviewSafetyV87::UserDir(Actual)||Actual!=User||!B->IsProgressionVolatile()){Check(TEXT("strict_review_isolation"),false);return Finish();}
        auto& S=ScreenState(C);
        if(Stage==0)
        {
            Clear(C);C->SetActorTickEnabled(true);OldHUD=C->GetHUD()?C->GetHUD()->bShowHUD:true;
            if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);R->SetHDREnabled(false,false);R->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);R->SetQuality(EFrontlineRenderQuality::Native,false);}
            Check(TEXT("public_national_start"),WarfareFRRUCampaignV82::Start(B,Nation(),EFrontlineDifficulty::Veteran));
            Check(TEXT("controlled_pose_campaign_marked_assisted"),WarfareFRRUCampaignV82::MarkHonourAssisted(B));
            uint32 Roles=(1u<<int32(EFrontlineRole::Rifle))|(1u<<int32(EFrontlineRole::Lewis))|WarfareFRRUCampaignV82::RequiredRoleMask(B);
            // A fresh campaign stores zero as "use the legal default roster".
            // Resolve that public roster before submitting a nonempty selection.
            Check(TEXT("public_required_role_selection"),WarfareFRRUCampaignV82::SetSelection(B,Roles,FrontlineCampaignV32::GetLoadout(B).SelectedSupports));
            if(!Good)return Finish();S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(C,B);At=Now;Stage=1;return true;
        }
        if(Stage==1)
        {
            if(!S.PreparationError.IsEmpty()||!B->AssetFailure.IsEmpty()){Check(TEXT("actual_scene_and_equipment_preparation"),false);return Finish();}
            if(S.Preparing||B->Phase!=EFrontlinePhase::Fighting){if(Now-At>90){Check(TEXT("actual_scene_preparation_bounded"),false);return Finish();}return true;}
            Check(TEXT("actual_native_scene_began"),WarfareFRRUWorldV82::Ready(B));if(!Good)return Finish();C->SetActorTickEnabled(false);Stage=2;At=Now;return true;
        }
        if(Stage==2)
        {
            const auto V=WarfareFRRUWorldV82::View(B);if(V.StoryPending&&!V.ChoiceIds.IsEmpty()){WarfareFRRUWorldV82::ReviewStoryChoiceDwell(B,V.StoryId,V.ChoiceIds[0]);return true;}
            if(!Ready(B)){if(Now-At>65)
                {
                    for(int32 Team=0;Team<2;++Team)for(int32 Kind=0;Kind<2;++Kind)
                    {
                        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("scope"),TEXT("Actual recruitment/entry timeout state, before any outcome fixture"));
                        J->SetNumberField(TEXT("team"),Team);J->SetNumberField(TEXT("role"),Kind==0?int32(EFrontlineRole::Lewis):int32(EFrontlineRole::Rifle));J->SetNumberField(TEXT("squad"),Recruits[Team][Kind]);
                        J->SetStringField(TEXT("current_public_block_reason"),B->RecruitBlockReason(Kind==0?EFrontlineRole::Lewis:EFrontlineRole::Rifle,Team));
                        TArray<TSharedPtr<FJsonValue>> Men;if(const auto* Q=B->FindSquad(Recruits[Team][Kind]))for(const auto& W:Q->Members)if(auto* U=W.Get())
                        {auto M=WarfareFRRUPresentationV82::ProbeUnit(U,false);M->SetBoolField(TEXT("entered"),U->bFieldEntered);M->SetBoolField(TEXT("alive"),U->IsAlive());M->SetStringField(TEXT("feet"),U->Feet().ToString());Men.Add(MakeShared<FJsonValueObject>(M));}
                        J->SetArrayField(TEXT("members"),Men);Actors.Add(MakeShared<FJsonValueObject>(J));
                    }
                    Check(TEXT("public_recruits_actually_enter_field_bounded"),false);return Finish();
                }return true;}
            for(int32 Team=0;Team<2;++Team)for(int32 Kind=0;Kind<2;++Kind)if(const auto* Q=B->FindSquad(Recruits[Team][Kind]))for(const auto& W:Q->Members)if(auto* U=W.Get();U&&U->IsAlive())
            {FObserved O;O.Unit=U;Check(TEXT("public_recruit_current_native_animation_binding"),Binding(U,O));Nations.Add(int32(WarfareFRRUCampaignV82::Nation(U)));if(auto* Gun=FrontlineMachinegunsV30::Get(U))O.HadSupportedMount=Gun->ReadSupportedWeaponAnchorV8447(O.MountBefore);Observed.Add(MoveTemp(O));}
            Check(TEXT("both_sides_public_rifle_and_gun_actors"),Observed.Num()>=12);if(!Good)return Finish();
            B->SetActorTickEnabled(false);B->Phase=Round?EFrontlinePhase::Defeat:EFrontlinePhase::Victory;B->BattleOutroWinner=Round?1:0;B->BattleOutroElapsed=0;
            if(C->GetHUD())C->GetHUD()->bShowHUD=false;Outro=0;ClockFrame=MAX_uint64;At=Now;Stage=3;return true;
        }
        if(Stage==3)
        {
            if(ClockFrame!=GFrameCounter){ClockFrame=GFrameCounter;const float DT=B->GetWorld()->GetDeltaSeconds();if(FMath::IsFinite(DT)&&DT>0){Outro+=DT;B->BattleOutroElapsed=Outro;}}
            Sample(B);
            if(PendingShot>=0)
            {
                if(GFrameCounter<=CameraFrame+2||FScreenshotRequest::IsScreenshotRequested())return true;
                if(auto* U=CameraUnit.Get();U&&U->Soldier)
                {
                    auto* N=U->Soldier->GetSingleNodeInstance();const FString Clip=N?GetNameSafe(N->GetAnimationAsset()):FString();
                    const TCHAR* Expected=PendingShot==0?TEXT("OneArmPump"):PendingShot==1?TEXT("OneArmSalute"):TEXT("A_Surrender_");
                    if(!N||!Clip.Contains(Expected)){PendingShot=INDEX_NONE;CameraUnit.Reset();return true;}
                }
                else{PendingShot=INDEX_NONE;return true;}
                const FString Kind=PendingShot==0?TEXT("one-arm-pump"):PendingShot==1?TEXT("one-arm-salute"):TEXT("surrender-ground-gun");const FString Name=Prefix()+TEXT("-")+Kind+TEXT(".png");
                FScreenshotRequest::RequestScreenshot(User/TEXT("Captures")/Name,false,false);auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("file"),Name);J->SetStringField(TEXT("round"),Prefix());J->SetBoolField(TEXT("controlled_pose_fixture"),true);if(auto* U=CameraUnit.Get()){J->SetNumberField(TEXT("squad"),U->SquadId);J->SetNumberField(TEXT("team"),U->Team);J->SetNumberField(TEXT("nation"),int32(WarfareFRRUCampaignV82::Nation(U)));}Captures.Add(MakeShared<FJsonValueObject>(J));
                if(PendingShot==0)PumpShot=true;else if(PendingShot==1)SaluteShot=true;else SurrenderShot=true;PendingShot=INDEX_NONE;return true;
            }
            for(int32 Kind=0;Kind<3;++Kind)if(!(Kind==0?PumpShot:Kind==1?SaluteShot:SurrenderShot))if(auto* U=FindShot(B,Kind))
            {CameraUnit=U;FFrontlineUIV8447Review::Camera(C,U->Feet()+FVector(0,0,105),Kind==2?1850.f:1450.f);V28ReviewPointer=FVector2D(-10000,-10000);PendingShot=Kind;CameraFrame=GFrameCounter;return true;}
            if(Outro<22)return true;
            if((!PumpShot||!SaluteShot||!SurrenderShot)&&Outro<34)return true;
            CompleteRound(B);if(!Good)return Finish();
            B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;B->Phase=EFrontlinePhase::Fighting;B->SetActorTickEnabled(true);if(C->GetHUD())C->GetHUD()->bShowHUD=OldHUD;
            Observed.Reset();for(auto& Team:Recruits)for(int32& Id:Team)Id=INDEX_NONE;Held=PumpShot=SaluteShot=SurrenderShot=false;PendingShot=INDEX_NONE;
            if(Round==0)
            {Round=1;Check(TEXT("public_restart_returns_to_briefing"),B->RestartBattle());Clear(C);C->SetActorTickEnabled(true);S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(C,B);Stage=1;At=Now;return true;}
            Round=0;++Country;if(Country<2){B->AbandonBattleToOperations();Stage=0;return true;}
            Check(TEXT("france_russia_and_both_actual_enemy_nations_covered"),Nations.Contains(2)&&Nations.Contains(3)&&Nations.Contains(1)&&Nations.Contains(4));
            Check(TEXT("no_synthetic_honour_or_route_written"),FrontlineAchievementsV76::EarnedCount()==0&&FrontlineAchievementsV76::CompletedCampaignRouteMaskV82()==0
                &&!IFileManager::Get().FileExists(*(User/TEXT("AccountReview/achievements-v2.json"))));Stage=4;Next=Now+.4;return true;
        }
        if(Stage==4)
        {if(Now<Next||FScreenshotRequest::IsScreenshotRequested())return true;bool Written=Captures.Num()==12;for(const auto& V:Captures)Written&=IFileManager::Get().FileSize(*(User/TEXT("Captures")/V->AsObject()->GetStringField(TEXT("file"))))>1024;Check(TEXT("all_actual_outcome_captures_written"),Written);return Finish();}
        Check(TEXT("known_outcome_stage"),false);return Finish();
    }
};
void Start(const FString& User)
{auto Run=MakeShared<FRun>();Run->User=User;FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float DT){return Run->Tick(DT);}));}
}
#endif
