#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineEquipmentV41.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
TSharedRef<FJsonObject> ActualPose(AWarfareFrontlineSoldier* U)
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("present"),IsValid(U));if(!IsValid(U))return J;
    J->SetStringField(TEXT("mesh"),GetPathNameSafe(U->Soldier->GetSkeletalMeshAsset()));J->SetNumberField(TEXT("role"),int32(U->Role));J->SetNumberField(TEXT("member"),U->MemberIndex);
    J->SetNumberField(TEXT("shots"),U->ShotsDischarged);J->SetNumberField(TEXT("rounds_remaining"),U->RoundsRemaining);J->SetNumberField(TEXT("travelled_cm"),U->DistanceTravelled);
    J->SetStringField(TEXT("position"),U->Feet().ToString());J->SetStringField(TEXT("muzzle"),U->GetMuzzleLocation().ToString());
    bool Finite=true;for(const FTransform& T:U->Soldier->GetComponentSpaceTransforms())Finite&=!T.ContainsNaN();J->SetBoolField(TEXT("all_evaluated_bones_finite"),Finite);
    if(auto* N=U->Soldier->GetSingleNodeInstance()){J->SetStringField(TEXT("clip"),GetNameSafe(N->GetAnimationAsset()));J->SetNumberField(TEXT("clip_seconds"),N->GetCurrentTime());}
    if(auto* A=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))
    {
        const auto D=A->GetCharacterDiagnostics();J->SetNumberField(TEXT("evaluated_frames"),D.EvaluatedFrames);J->SetNumberField(TEXT("invalid_bone_frames"),D.InvalidBoneFrames);
        J->SetNumberField(TEXT("maximum_grip_error_cm"),A->GetMaximumGripResidualCm());J->SetNumberField(TEXT("maximum_grip_correction_cm"),A->GetMaximumGripCorrectionCm());
        J->SetNumberField(TEXT("maximum_arm_length_error_cm"),D.MaximumArmLengthErrorCm);J->SetNumberField(TEXT("maximum_leg_length_error_cm"),D.MaximumTerrainLegLengthErrorCm);
    }
    if(auto* Gun=FrontlineMachinegunsV30::Get(U)){J->SetBoolField(TEXT("supported"),Gun->CanFire());J->SetBoolField(TEXT("bipod_both_feet_contact"),Gun->HasBipodContactV36());J->SetNumberField(TEXT("deployment_seconds"),Gun->LastDeploymentSeconds);}
    return J;
}

TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> PrepareCrowd(AWarfareFrontlineGameMode* B)
{
    TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Movers;
    for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Empty();B->Squads.Empty();FrontlineMobilityV41::Reset(B);
    const int32 Dir=B->GetTeamDirection(0);
    for(int32 Row=0;Row<4;++Row)
    {
        FFrontlineSquad S;S.Id=941900+Row;S.Team=0;S.Role=EFrontlineRole::Rifle;S.Slot=1;S.bAdvancing=true;S.Destination=FrontlineBattlefield::CentreTrench+Dir;
        for(int32 I=0;I<4;++I)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)continue;U->Initialize(B,S.Id,0,EFrontlineRole::Rifle,I);U->FormationOffset=FVector::ZeroVector;
            const float X=(Row==3?-120.f:-1100.f-Row*145.f)*Dir,Y=(I-1.5f)*145.f-300;
            U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));U->SetActorRotation(FRotator(0,Dir>0?0:180,0));U->SetActorTickEnabled(Row<3);
            S.Members.Add(U);B->Units.Add(U);if(Row<3)Movers.Add(U);
        }
        B->Squads.Add(S);
    }
    if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
    {
        V->ResetBattle();V->ResetReadinessForDebug(0);V->Recruit(EFrontlineVehicle::MarkIVMale);V->ResetReadinessForDebug(0);V->Recruit(EFrontlineVehicle::Whippet);
        if(V->GetVehicles().Num()>=2)
        {
            auto* W=V->GetVehicles()[0].Get();auto* F=V->GetVehicles()[1].Get();W->SetActorLocation(FVector(-350,900,AWarfareFrontlineGround::HeightAt(-350,900)));W->ReceiveModuleHit(EFrontlineTankModule::Hull,W->GetMaximumHullIntegrity()+1);
            F->SetActorLocation(FVector(-1500,900,AWarfareFrontlineGround::HeightAt(-1500,900)));for(int32 I=0;I<100;++I)F->Simulate(.1f);
        }
    }
    return Movers;
}
}

void StartFrontlineV41Review(AWarfareFrontlineGameMode* Battle)
{
    FString UserDir;
    if(!Battle||!FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
    struct FRun
    {
        int32 Stage=0,Shots=0,ActionMode=INDEX_NONE;bool Passed=true,Crowd=false;
        double Start=FPlatformTime::Seconds(),Next=Start+3;float CrowdSeconds=0;
        FString Directory;TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;TWeakObjectPtr<AWarfareFrontlineSoldier> Specialist;
        TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Movers;TArray<TSharedPtr<FJsonValue>> Checks;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->Battle=Battle;R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V41/Review"));IFileManager::Get().MakeDirectory(*R->Directory,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
    {
        auto* B=R->Battle.Get();auto* PC=B?Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController()):nullptr;auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        const double Now=FPlatformTime::Seconds();if(!B)return false;if(!PC||!HUD)return Now-R->Start<25;
        if(R->ActionMode!=INDEX_NONE)FrontlineMotionV41::TickActionReview(B,DT);
        if(R->Crowd){const float Step=FMath::Clamp(DT,0.f,.05f);R->CrowdSeconds+=Step;B->Elapsed+=Step;}
        if(Now<R->Next)return true;
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const TCHAR* Name){++R->Shots;FScreenshotRequest::RequestScreenshot(R->Directory/(FString(Name)+TEXT(".png")),true,false);};
        if(Now-R->Start>155){Check(TEXT("bounded_focused_review_timeout"),false);R->Stage=99;}
        if(R->Stage==17&&R->CrowdSeconds<4.8f)return true;
        if(R->Stage==19||R->Stage==21)if(B->Elapsed<5.08f)return true;
        if(R->ActionMode!=INDEX_NONE&&R->Stage<99)
        {
            const float Gate=R->Stage==23?.35f:R->Stage==24?1.3f:R->Stage==25?2.3f:R->Stage==26?4.8f:R->Stage==28?4.5f:R->Stage==30?1.3f:R->Stage==31?4.8f:0;
            if(FrontlineMotionV41::ActionReviewState(B)->GetNumberField(TEXT("review_game_seconds"))<Gate)return true;
        }
        auto Clear=[&]()
        {
            R->ActionMode=INDEX_NONE;R->Crowd=false;FrontlineReviewUIV41::Clear(PC);UGameplayStatics::SetGamePaused(PC,false);B->SetActorTickEnabled(false);
            for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(It->ActorHasTag(TEXT("V41SpecialistFixture")))It->Destroy();
        };
        auto Select=[&](EFrontlineFaction F,int32 Mission,bool Start)
        {
            Clear();Check(FString::Printf(TEXT("prepare_%d_%d"),int32(F),Mission),FrontlineCampaignV41::PrepareReviewOperation(B,F,Mission));FrontlineReviewUIV41::Dossier(PC);PC->ResetCamera();
            if(Start){FrontlineReviewUIV41::Clear(PC);B->BeginBattle();B->Elapsed=5;B->SetActorTickEnabled(false);Check(TEXT("production_field_ready_")+FString::FromInt(R->Stage),B->Phase==EFrontlinePhase::Fighting&&B->AssetFailure.IsEmpty());}
        };
        auto ActionState=[&](const TCHAR* Key)
        {
            auto J=FrontlineMotionV41::ActionReviewState(B);R->Report->SetObjectField(Key,J);bool Finite=true;
            for(const auto& V:J->GetArrayField(TEXT("units"))){const auto A=V->AsObject();Finite&=A->GetBoolField(TEXT("physics_finite"));double Invalid=0;if(A->TryGetNumberField(TEXT("invalid_bone_frames"),Invalid))Finite&=Invalid==0;}
            Check(FString(Key)+TEXT("_finite_pose_and_physics"),Finite);
        };
        double Delay=.6;
        switch(R->Stage)
        {
        case 0:
        {
            Check(TEXT("isolated_volatile_profile"),B->IsProgressionVolatile());if(!R->Passed){R->Stage=98;break;}
            FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);FrontlineReviewUIV41::BeginAudit();
            if(auto* Render=B->GetWorld()->GetSubsystem<UFrontlineRenderSettingsSubsystem>()){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            Select(EFrontlineFaction::British,19,true);FrontlineCampaignV41::RunChecks(B,Check);Select(EFrontlineFaction::British,19,true);FrontlineMobilityV41::RunChecks(B,Check);FrontlineSpecialistsV41::RunChecks(B,Check);
            if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld())){const auto J=V->RunMobilityChecksV41();R->Report->SetObjectField(TEXT("actual_vehicle_mobility"),J);Check(TEXT("six_tanks_shared_cooldown_wreck_clearance"),J->GetBoolField(TEXT("success")));}else Check(TEXT("vehicle_subsystem_available"),false);
            FrontlineMotionV41::RunFocusedChecks(B,Check);if(auto* A=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->RunV41AudioChecks(Check);else Check(TEXT("audio_subsystem_available"),false);
            Select(EFrontlineFaction::British,2,false);Delay=1.2;break;
        }
        case 1:Shot(TEXT("01-british-bullecourt-route-and-briefing"));break;
        case 2:Select(EFrontlineFaction::German,12,false);FrontlineReviewUIV41::Pointer(FVector2D(748,724));Delay=1.1;break;
        case 3:Shot(TEXT("02-german-lys-command-and-route"));break;
        case 4:Select(EFrontlineFaction::British,18,false);Delay=1;break;
        case 5:Shot(TEXT("03-british-final-advance-operation"));break;
        case 6:Select(EFrontlineFaction::German,19,false);Delay=1;break;
        case 7:Shot(TEXT("04-german-final-operation"));break;
        case 8:Select(EFrontlineFaction::British,19,false);FrontlineReviewUIV41::Deployment(PC);Delay=1;break;
        case 9:Shot(TEXT("05-british-eight-unit-deployment"));break;
        case 10:FrontlineReviewUIV41::Technology(PC,1,24);FrontlineReviewUIV41::Pointer(FVector2D(486,752.5));Delay=.9;break;
        case 11:Shot(TEXT("06-british-specialist-research"));break;
        case 12:Select(EFrontlineFaction::German,19,false);FrontlineReviewUIV41::Deployment(PC);Delay=1;break;
        case 13:Shot(TEXT("07-german-eight-unit-deployment"));break;
        case 14:FrontlineReviewUIV41::Technology(PC,1,24);FrontlineReviewUIV41::Pointer(FVector2D(486,752.5));Delay=.9;break;
        case 15:Shot(TEXT("08-german-specialist-research"));break;
        case 16:Select(EFrontlineFaction::British,19,true);R->Movers=PrepareCrowd(B);R->Crowd=true;R->CrowdSeconds=0;FrontlineReviewUIV41::Camera(PC,0,180,4700,130);Delay=.4;break;
        case 17:
        {
            TArray<TSharedPtr<FJsonValue>> Rows;bool Travel=R->Movers.Num()==12;
            for(const auto& W:R->Movers){auto J=ActualPose(W.Get());Rows.Add(MakeShared<FJsonValueObject>(J));Travel&=W.IsValid()&&W->DistanceTravelled>200&&J->GetBoolField(TEXT("all_evaluated_bones_finite"));}
            R->Report->SetArrayField(TEXT("moving_crowd_actual_poses"),Rows);Check(TEXT("all_twelve_real_tick_movers_progress_during_capture"),Travel);Shot(TEXT("09-crowd-passage-and-inert-armour-wreck"));R->Crowd=false;break;
        }
        case 18:Select(EFrontlineFaction::British,19,true);R->Specialist=FrontlineSpecialistsV41::SetupVisualFixture(B,EFrontlineRole::RifleGrenadier);FrontlineReviewUIV41::Camera(PC,-100,100,1600,85);Delay=.08;break;
        case 19:
        {
            B->SetActorTickEnabled(false);auto* U=R->Specialist.Get();if(U){FrontlineEquipmentV41::RunChecks(U,Check);if(auto* N=U->Soldier->GetSingleNodeInstance())N->SetPlaying(false);}
            R->Report->SetObjectField(TEXT("No23_actual_firing_pose"),ActualPose(U));Check(TEXT("No23_visual_is_an_actual_discharged_round"),U&&U->ShotsDischarged==1&&!B->Strikes.IsEmpty()&&B->Strikes[0].bRifleLaunched);Shot(TEXT("10-No23-inclined-rifle-launch"));break;
        }
        case 20:Select(EFrontlineFaction::German,19,true);R->Specialist=FrontlineSpecialistsV41::SetupVisualFixture(B,EFrontlineRole::AntiTankRifle);FrontlineReviewUIV41::Camera(PC,150,100,1700,75);Delay=.08;break;
        case 21:
        {
            B->SetActorTickEnabled(false);auto* U=R->Specialist.Get();if(U){FrontlineEquipmentV41::RunChecks(U,Check);if(auto* N=U->Soldier->GetSingleNodeInstance())N->SetPlaying(false);}
            R->Report->SetObjectField(TEXT("Tankgewehr_actual_firing_pose"),ActualPose(U));auto* Gun=FrontlineMachinegunsV30::Get(U);Check(TEXT("AT_visual_is_an_actual_braced_discharge"),U&&U->ShotsDischarged==1&&Gun&&Gun->CanFire());Shot(TEXT("11-Tankgewehr-prone-supported-discharge"));break;
        }
        case 22:Select(EFrontlineFaction::British,19,true);FrontlineMotionV41::PrepareActionReview(B,0);R->ActionMode=0;FrontlineReviewUIV41::Camera(PC,0,0,2800,70);Delay=.15;break;
        case 23:
        {
            ActionState(TEXT("casualties_035"));const auto J=FrontlineMotionV41::ActionReviewState(B);const auto& Units=J->GetArrayField(TEXT("units"));
            bool Correct=Units.Num()==6;if(Correct){auto Fuel=Units[5]->AsObject();Correct&=Fuel->GetBoolField(TEXT("fuel_ignited"))&&!Fuel->GetBoolField(TEXT("fuel_spent"))&&Fuel->GetNumberField(TEXT("fuel_cookoff_remaining_seconds"))>.6;for(int32 I=0;I<3;++I)Correct&=!Units[I]->AsObject()->GetBoolField(TEXT("fuel_ignited"));}
            Check(TEXT("fire_casualty_retains_finite_fuel_before_delayed_rupture"),Correct);Shot(TEXT("12-bullet-and-fire-impact-reactions"));Delay=.1;break;
        }
        case 24:ActionState(TEXT("casualties_130"));Shot(TEXT("13-fire-and-bullet-authored-falls"));Delay=.1;break;
        case 25:
        {
            ActionState(TEXT("casualties_230"));const auto J=FrontlineMotionV41::ActionReviewState(B);const auto& Units=J->GetArrayField(TEXT("units"));
            Check(TEXT("finite_fuel_rupture_is_complete_after_fire_action"),Units.Num()==6&&Units[5]->AsObject()->GetBoolField(TEXT("fuel_spent"))&&Units[5]->AsObject()->GetNumberField(TEXT("fuel_cookoff_remaining_seconds"))==0);
            bool CarrierResponded=Units.Num()==6;
            if(CarrierResponded)
            {
                const auto Fuel=Units[5]->AsObject();
                CarrierResponded=Fuel->GetBoolField(TEXT("physics_started"))&&Fuel->GetBoolField(TEXT("fuel_carrier_impulse_applied"))
                    &&Fuel->GetBoolField(TEXT("fuel_rupture_interrupted_authored_fall"))&&!Fuel->GetBoolField(TEXT("physics_handoff_queued"))
                    &&Fuel->GetNumberField(TEXT("fuel_rupture_pose_seconds"))>.8&&Fuel->GetNumberField(TEXT("fuel_rupture_pose_seconds"))<1.5
                    &&Fuel->GetNumberField(TEXT("fuel_delta_speed_cm_s"))>220&&Fuel->GetNumberField(TEXT("fuel_delta_speed_cm_s"))<360;
            }
            Check(TEXT("fuel_rupture_interrupts_actual_carrier_into_bounded_physics"),CarrierResponded);
            bool Once=false;
            for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->GetDeathState().bFuelRupture)
            {
                const auto Before=U->GetDeathState();const int32 Casualties=B->Casualties[0]+B->Casualties[1];
                const float Damage=B->DamageDealt[0]+B->DamageDealt[1];U->OnFuelRuptureV41(999.f);
                const auto& After=U->GetDeathState();Once=After.bFuelRuptureApplied&&After.FuelRuptureAt==Before.FuelRuptureAt
                    &&After.FuelRuptureDeltaV==Before.FuelRuptureDeltaV&&Casualties==B->Casualties[0]+B->Casualties[1]
                    &&Damage==B->DamageDealt[0]+B->DamageDealt[1];break;
            }
            Check(TEXT("fuel_carrier_impulse_is_idempotent_without_second_casualty"),Once);
            Shot(TEXT("14-fall-to-physics-handoff"));Delay=.1;break;
        }
        case 26:ActionState(TEXT("casualties_480"));Shot(TEXT("15-grounded-casualty-settling"));R->ActionMode=INDEX_NONE;break;
        case 27:Select(EFrontlineFaction::British,19,true);FrontlineMotionV41::PrepareActionReview(B,1);R->ActionMode=1;FrontlineReviewUIV41::Camera(PC,0,0,2500,60);Delay=.1;break;
        case 28:ActionState(TEXT("trench_waiting_450"));Shot(TEXT("16-independent-trench-waiting"));R->ActionMode=INDEX_NONE;break;
        case 29:Select(EFrontlineFaction::British,19,true);FrontlineMotionV41::PrepareActionReview(B,2);R->ActionMode=2;FrontlineReviewUIV41::Camera(PC,0,0,2800,70);Delay=.1;break;
        case 30:ActionState(TEXT("outcome_130"));Shot(TEXT("17-victory-and-surrender-opening"));Delay=.1;break;
        case 31:ActionState(TEXT("outcome_480"));Shot(TEXT("18-victory-and-surrender-completion"));R->ActionMode=INDEX_NONE;break;
        default:
        {
            R->Crowd=false;R->ActionMode=INDEX_NONE;Check(TEXT("eighteen_changed_area_captures_requested"),R->Shots==18);const auto UI=FrontlineReviewUIV41::EndAudit();R->Report->SetObjectField(TEXT("actual_UI_layout"),UI);Check(TEXT("changed_UI_layout_fits"),UI->GetBoolField(TEXT("success")));
            Check(TEXT("visible_English_coverage"),FrontlineLocalizationV35::WriteCoverageReport());int32 W=0,H=0;PC->GetViewportSize(W,H);Check(TEXT("actual_4K_review_viewport"),W==3840&&H==2160);
            R->Report->SetNumberField(TEXT("width"),W);R->Report->SetNumberField(TEXT("height"),H);R->Report->SetNumberField(TEXT("screenshots_requested"),R->Shots);R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetStringField(TEXT("scope"),TEXT("Focused V41 production movement, single-round specialists, armour limits/collision, campaign, equipment, action and audio checks. Actual 4K UI and posed actors. No full historical suite or hardware performance claim."));
            R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetBoolField(TEXT("success"),R->Passed);FString Text;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Directory/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FPlatformMisc::RequestExit(false);return false;
        }
        }
        ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
    }));
}
