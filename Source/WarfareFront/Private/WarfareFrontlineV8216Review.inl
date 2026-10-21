#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineFireV33.h"
#include "WarfareFrontlineMotionV40.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DynamicRHI.h"
#include "RenderTimer.h"

namespace
{
// One disposable production-world run. Account mutation is allowed only under
// the same explicit archive-review boundary used by the account implementation.
void StartFrontlineV8216Review(AWarfareFrontlineController* Controller)
{
    FString User;
    if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8216Review"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV8215ArchiveReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||FPaths::IsRelative(User))return;
    User=FPaths::ConvertRelativePathToFull(User);FPaths::NormalizeDirectoryName(User);
    const FString Allowed=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8216/ArchiveReview/Run"));
    if(User.Contains(TEXT(".."))||(!User.Equals(Allowed,ESearchCase::IgnoreCase)
        &&!User.StartsWith(Allowed+TEXT("/"),ESearchCase::IgnoreCase)))return;
    FString AccountDir;if(!FrontlineUltimateArchiveV8215::ReviewUserDir(AccountDir))return;
    struct FRun
    {
        int32 Stage=0,Shots=0;bool Passed=true;TFunction<void()> AfterCapture;
        double Start=FPlatformTime::Seconds(),Next=Start+3,Prepared=0,SceneAt=0;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks,Phases;
        TWeakObjectPtr<AFrontlineSupportMissionV28> Gas;
        TWeakObjectPtr<AWarfareFrontlineImpact> Mine;
        TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> GasMen;
        TWeakObjectPtr<AWarfareFrontlineSoldier> Heavy,HeavyTarget;
        bool GasArrived=false,GasEscapeShot=false;float GasArrivedHost=0,BurnAtHost=0,WreckAtHost=0;
        TArray<FVector> GasInitialFeet;TWeakObjectPtr<AWarfareFrontlineSoldier> Burning;TWeakObjectPtr<AFrontlineTank> Wreck;TArray<double> WreckFrameMs,WreckGpuMs;double LastWreckSample=0;
    };
    auto R=MakeShared<FRun>();R->Directory=User;
    IFileManager::Get().MakeDirectory(*(R->Directory/TEXT("Captures")),true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float Delta)
    {
        auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        const double Now=FPlatformTime::Seconds();
        auto Check=[&](const FString& Name,bool Pass)
        {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Pass;};
        auto Finish=[&]()
        {
            auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Passed);J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("phases"),R->Phases);
            J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Start);
            J->SetStringField(TEXT("user_dir"),R->Directory);
            auto Timings=MakeShared<FJsonObject>();auto Summary=[&](const TCHAR* Prefix,TArray<double> A){if(A.IsEmpty())return;A.Sort();double Sum=0;for(double X:A)Sum+=X;Timings->SetNumberField(FString(Prefix)+TEXT("_samples"),A.Num());Timings->SetNumberField(FString(Prefix)+TEXT("_mean_ms"),Sum/A.Num());Timings->SetNumberField(FString(Prefix)+TEXT("_p95_ms"),A[FMath::Clamp(FMath::CeilToInt(.95f*A.Num())-1,0,A.Num()-1)]);};Summary(TEXT("actual_wreck_frame"),R->WreckFrameMs);Summary(TEXT("actual_wreck_gpu"),R->WreckGpuMs);Timings->SetStringField(TEXT("scope"),TEXT("Short warm actual-wreck frames before capture, 4K SDR High Quality SR FG off. Narrow rendering-cost sample, not full battle performance certification; a concurrent V82.15 user game may affect timing."));J->SetObjectField(TEXT("timing"),Timings);
            J->SetStringField(TEXT("scope"),TEXT("Isolated German mission 8 production preparation; focused support, mine, gas/action and account/archive checks; 4K SDR High screenshots. Gas casualties use actual single-dose exposure. Mine captures are cosmetic blood/dust profile phases; final HE frames resolve real casualties and incident blood, fire frames use actual fire fatalities and hull destruction. Real instance UI click routing, medal historical classes and persistence, MG stance/contact and committed explosion regressions. No natural campaign, HDR or performance certification; concurrent V82.15 user instance permitted."));
            if(B)if(auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))J->SetObjectField(TEXT("render"),Settings->GetDiagnostics());
            FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Directory/TEXT("checks.json")));
            FPlatformMisc::RequestExit(false);return false;
        };
        if(!B||!HUD){Check(TEXT("v8216_review_world_and_hud_present"),false);return Finish();}
        if(!B->IsProgressionVolatile()){Check(TEXT("v8216_review_never_operates_on_player_profile"),false);return Finish();}
        if(Now-R->Start>=178.){Check(TEXT("v8216_review_bounded_180_second_run"),false);return Finish();}
        auto& S=ScreenState(PC);
        // Battle tick stays frozen: only the real systems under capture receive
        // the host clock. No reinforcement AI or victory can disturb fixtures.
        const float DT=FMath::Clamp(Delta,.0001f,.10f);
        if(R->Stage>=3&&R->Stage<=5)
        {
            B->Elapsed+=DT;
            if(auto* Gas=R->Gas.Get())Gas->Tick(DT);
            for(const auto& WeakMan:R->GasMen)if(auto* U=WeakMan.Get())
            {
                if(U->IsAlive()){U->Tick(DT);}
                else U->TickDeath(DT);
            }
        }
        if(R->Stage>=38&&R->Stage<=41){B->Elapsed+=DT;if(R->Stage<=39)if(auto* U=R->Burning.Get())U->Tick(DT);if(R->Stage>=40)if(auto* V=R->Wreck.Get())V->Simulate(DT);}
        if(R->Stage>=35&&R->Stage<=37){for(const auto& W:R->GasMen)if(auto* U=W.Get();U&&!U->IsAlive())U->TickDeath(DT);}
        if(R->Stage>=7&&R->Stage<=8)
        {
            B->Elapsed+=DT;
            if(auto* U=R->Heavy.Get())U->Tick(DT);
            FrontlineSpecialistsV41::Tick(B);
            if(auto* U=R->HeavyTarget.Get();U&&!U->IsAlive())U->TickDeath(DT);
        }
        if(R->Stage==41&&B->Elapsed-R->WreckAtHost>.6f&&!FScreenshotRequest::IsScreenshotRequested()){if(R->LastWreckSample>0)R->WreckFrameMs.Add((Now-R->LastWreckSample)*1000.);R->LastWreckSample=Now;const double GPU=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());if(GPU>0)R->WreckGpuMs.Add(GPU);}
        if(FScreenshotRequest::IsScreenshotRequested())return true;
        if(R->AfterCapture){auto Action=MoveTemp(R->AfterCapture);Action();}
        if(Now<R->Next)return true;
        auto Clear=[&]()
        {
            UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.UnitIntroduction=S.Preparing=S.Deployment=S.HistoricalDispatch=S.CampaignEndingV51=S.AchievementsV76=false;
            S.bPausedByOverlay=S.bPausedForAuxOverlay=false;MedalWallV8215::View(PC).Wall=false;MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::View(PC).Picker=false;UltimateArchiveUIV8215::Close(PC);PC->CancelTargeting();
        };
        auto RemoveActors=[&]()
        {
            for(const auto& WeakUnit:B->Units)if(auto* U=WeakUnit.Get())U->Destroy();B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();
            FrontlineMotionV40::Reset(B);FrontlineSpecialistsV41::Reset(B);
            for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)if(IsValid(*I))I->Destroy();
            if(auto* Gas=R->Gas.Get())Gas->Destroy();R->Gas.Reset();R->GasMen.Empty();R->Heavy.Reset();R->HeavyTarget.Reset();R->Burning.Reset();if(auto* V=R->Wreck.Get())V->Destroy();R->Wreck.Reset();
        };
        auto MakeMan=[&](int32 Id,int32 Team,EFrontlineRole Role,int32 Member,FVector At)
        {
            At.Z=AWarfareFrontlineGround::HeightAt(At.X,At.Y)+88.f;
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(At,FRotator::ZeroRotator);
            if(U){U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->SetActorLocation(At);U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));U->bFieldEntered=true;U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->RoundsRemaining=U->GetMagazineCapacity();U->ReturnToIdle();B->Units.Add(U);}
            return U;
        };
        auto Shot=[&](const TCHAR* Name)
        {
            ++R->Shots;FScreenshotRequest::RequestScreenshot(R->Directory/TEXT("Captures")/(FString(Name)+TEXT(".png")),true,false);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("capture"),Name);J->SetNumberField(TEXT("scene_age_wall_seconds"),Now-R->SceneAt);J->SetNumberField(TEXT("host_clock_seconds"),B->Elapsed);
            if(auto* Gas=R->Gas.Get())
            {
                J->SetNumberField(TEXT("gas_exposure_passes"),Gas->RoundsFired);TArray<UStaticMeshComponent*> Components;Gas->GetComponents<UStaticMeshComponent>(Components);int32 Visible=0;
                for(auto* C:Components)Visible+=C&&C->IsVisible();J->SetNumberField(TEXT("gas_volume_boxes"),Components.Num());J->SetNumberField(TEXT("gas_visible_volumes"),Visible);
            }
            int32 Deaths=0;TArray<TSharedPtr<FJsonValue>> Men;
            for(const auto& Man:R->GasMen)if(auto* U=Man.Get())
            {
                const auto G=FrontlineMotionV40::GasExposureV8215(U);auto M=MakeShared<FJsonObject>();M->SetNumberField(TEXT("member"),U->MemberIndex);M->SetBoolField(TEXT("alive"),U->IsAlive());M->SetBoolField(TEXT("sealed"),G.Sealed);M->SetBoolField(TEXT("escaping"),G.Escaping);M->SetStringField(TEXT("feet"),U->Feet().ToString());M->SetNumberField(TEXT("dose"),G.Dose);M->SetNumberField(TEXT("threshold"),G.Threshold);M->SetStringField(TEXT("incoming_clip"),U->GetDeathState().ActualEntryClip);M->SetStringField(TEXT("authored_death_clip"),GetPathNameSafe(U->GetDeathState().AuthoredAnimation.Get()));Men.Add(MakeShared<FJsonValueObject>(M));Deaths+=!U->IsAlive();
            }
            J->SetNumberField(TEXT("gas_casualties"),Deaths);J->SetArrayField(TEXT("gas_men"),Men);
            if(auto* U=R->Heavy.Get()){J->SetNumberField(TEXT("tankgewehr_shots"),U->ShotsDischarged);J->SetNumberField(TEXT("tankgewehr_recoil_age"),U->HeavyRecoilAgeV8215());}
            if(auto* M=R->Mine.Get()){J->SetNumberField(TEXT("mine_age"),M->GetVisualAgeSeconds());J->SetNumberField(TEXT("mine_fragments"),M->GetBallisticFragmentCount());J->SetNumberField(TEXT("mine_visible_smoke"),M->GetVisibleSmokeCount());}
            R->Phases.Add(MakeShared<FJsonValueObject>(J));
        };
        double Delay=.65;
        switch(R->Stage)
        {
        case 0:
        {
            Check(TEXT("v8216_review_starts_in_empty_briefing"),B->Phase==EFrontlinePhase::Briefing&&B->Units.IsEmpty());
            if(!R->Passed)return Finish();
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
            {Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);}
            FrontlineAchievementsV76::RunMedalChecksV8215(B,Check);
            for(int32 N=0;N<4;++N)for(int32 I:FrontlineMedalsV8215::NationIndices(EFRRUNationV81(N)))for(int32 G=1;G<=FrontlineMedalsV8215::Variants(I).Num();++G){const FString Path=FrontlineMedalsV8215::VariantArtworkPath(I,G);Check(FString::Printf(TEXT("v8216_cooked_medal_%d_%d"),I,G),LoadObject<UTexture2D>(nullptr,*Path,nullptr,LOAD_NoWarn)!=nullptr);}
            for(const TCHAR* Name:{TEXT("M_BlastVolume_V8215"),TEXT("M_GasVolume_V8215"),TEXT("M_PenetrationShard_V8215")})
            {
                const FString Path=FString::Printf(TEXT("/Game/Warfare/Field/V8215/Materials/%s.%s"),Name,Name);
                Check(FString(TEXT("v8215_cooked_material_"))+Name,LoadObject<UMaterialInterface>(nullptr,*Path,nullptr,LOAD_NoWarn)!=nullptr);
            }
            for(const TCHAR* Name:{TEXT("T_GermanTechA_V8215"),TEXT("T_GermanTechAArchive_V8215"),TEXT("T_MobileGunTech_V8215"),TEXT("T_ForwardServiceTeams_V8215"),TEXT("T_MedalAtlasV8215")})
            {
                const FString Path=FString::Printf(TEXT("/Game/Warfare/UI/V8215/%s.%s"),Name,Name);
                Check(FString(TEXT("v8215_cooked_plate_"))+Name,LoadObject<UTexture2D>(nullptr,*Path,nullptr,LOAD_NoWarn)!=nullptr);
            }
            Check(TEXT("v8216_review_german_campaign_created"),B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran));
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))
            {Run->CompletedMissions=14;Run->WonOperationMaskV52=(1u<<14)-1u;Run->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1u;Run->Research.ResearchPoints=30;Run->ArmourResearchMaskV63=511;}
            Check(TEXT("v8216_review_german_mission_8_selected"),B->SelectCampaignMission(7));
            Clear();S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;Delay=.1;break;
        }
        case 1:
        {
            if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&Now-R->Prepared<35.){R->Next=Now+.1;return true;}
            const bool Ready=B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&B->AssetFailure.IsEmpty();
            auto Prep=MakeShared<FJsonObject>();Prep->SetStringField(TEXT("phase"),TEXT("production_preparation"));
            Prep->SetBoolField(TEXT("ready"),Ready);Prep->SetBoolField(TEXT("preparing"),S.Preparing);
            Prep->SetBoolField(TEXT("world_paused"),UGameplayStatics::IsGamePaused(B));
            Prep->SetNumberField(TEXT("battle_phase"),int32(B->Phase));Prep->SetStringField(TEXT("asset_failure"),B->AssetFailure);
            Prep->SetStringField(TEXT("preparation_error"),S.PreparationError);Prep->SetNumberField(TEXT("scene_frames"),S.PreparationSceneFrames);
            Prep->SetNumberField(TEXT("textures_total"),S.PreparationTextures.Num());Prep->SetNumberField(TEXT("textures_resident"),ResidentOperationTextures(S));
            Prep->SetNumberField(TEXT("pending_psos"),PipelineStateCache::GetNumActivePipelinePrecompileTasks());
            Prep->SetNumberField(TEXT("scene_seconds"),S.PreparationFirstSceneAt>0?Now-S.PreparationFirstSceneAt:0.);
            R->Phases.Add(MakeShared<FJsonValueObject>(Prep));
            Check(TEXT("v8216_review_production_preparation_ready"),Ready);if(!Ready)return Finish();
            int32 W=0,H=0;PC->GetViewportSize(W,H);Check(TEXT("v8216_review_actual_4k_viewport"),W==3840&&H==2160);
            B->SetActorTickEnabled(false);Clear();
            RunFrontlineSupportRulesChecksV8215(B,Check);FrontlineFieldV32::RunMineRulesChecksV8215(B,Check);FrontlineMotionV40::RunFocusedChecksV8215(B,Check);
            FrontlineMedalMotionV8216::RunChecks(Check);
            Check(TEXT("v8216_medal_mount_does_not_suspend_IronCross_I_pin_or_multi_piece_sash"),!MedalMountV8216::HangableGrades(3).Contains(2)&&!MedalMountV8216::HangableGrades(20).Contains(5)&&MedalMountV8216::HangableGrades(3).Contains(1));
            Check(TEXT("v8216_weapon_badge_is_not_a_hanging_pendant"),!MedalMountV8216::HangableGrades(31).Contains(1));FrontlineMachinegunsV30::RunStanceChecksV8216(B,Check);AWarfareFrontlineImpact::RunChecksV8216(B,Check);
            RemoveActors();B->Elapsed=180.f;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
            FFrontlineReviewV32::Camera(PC,0,200,4200,180);Delay=1.;break;
        }
        case 2:
        {
            const float Gap=B->Trenches.Num()>1?(B->Trenches[0].X+B->Trenches[1].X)*.5f:0.f;
            R->GasMen.Add(MakeMan(9821501,0,EFrontlineRole::Rifle,0,FVector(Gap-180,100,0)));
            R->GasMen.Add(MakeMan(9821502,0,EFrontlineRole::Rifle,1,FVector(Gap,260,0)));
            R->GasMen.Add(MakeMan(9821503,0,EFrontlineRole::Rifle,2,FVector(Gap+170,100,0)));
            R->GasMen.Add(MakeMan(9821504,0,EFrontlineRole::Mills,0,FVector(Gap+40,450,0)));
            for(const auto& M:R->GasMen)R->GasInitialFeet.Add(M.IsValid()?M->Feet():FVector::ZeroVector);
            const FVector At(Gap,200,AWarfareFrontlineGround::HeightAt(Gap,200));auto* Gas=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
            const bool Configured=Gas&&Gas->Configure(B,EFrontlineSupport::Gas,At,1);R->Gas=Gas;if(Gas)Gas->SetActorTickEnabled(false);
            FFrontlineReviewV32::Camera(PC,Gap,200,4200,180);
            Check(TEXT("v8216_review_real_gas_support_configured"),Configured);R->SceneAt=Now;Delay=.1;break;
        }
        case 3:
        {
            auto* Gas=R->Gas.Get();if(!Gas||Gas->RoundsFired<=0){if(Now-R->SceneAt<35.){R->Next=Now+.01;return true;}Check(TEXT("v8216_review_gas_arrived_in_bound"),false);return Finish();}
            if(!R->GasArrived){R->GasArrived=true;R->GasArrivedHost=B->Elapsed;R->SceneAt=Now;}
            if(!R->GasEscapeShot)for(int32 I=0;I<R->GasMen.Num();++I)if(auto* U=R->GasMen[I].Get();U&&U->IsAlive()&&FrontlineMotionV40::GasExposureV8215(U).Escaping&&R->GasInitialFeet.IsValidIndex(I)&&FVector::Dist2D(U->Feet(),R->GasInitialFeet[I])>3.f){Shot(TEXT("27-gas-real-living-escape"));R->GasEscapeShot=true;Check(TEXT("v8216_gas_capture_has_living_actually_displaced_escapee"),true);R->Next=Now+.01;return true;}
            if(B->Elapsed-R->GasArrivedHost<9.2f){R->Next=Now+.01;return true;}Check(TEXT("v8216_review_gas_escape_captured"),R->GasEscapeShot);Delay=.01;break;
        }
        case 4:
            Shot(TEXT("01-gas-six-clouds-and-choking"));
            if(auto* Gas=R->Gas.Get()){TArray<UStaticMeshComponent*> C;Gas->GetComponents<UStaticMeshComponent>(C);int32 Visible=0;for(auto* V:C)Visible+=V&&V->IsVisible();Check(TEXT("v8216_review_all_six_persistent_gas_volumes_visible"),Visible==6);}
            Delay=2.4;break;
        case 5:
        {
            Shot(TEXT("02-gas-authored-collapse"));int32 Fatal=0;bool GasDeath=true;
            for(int32 I=0;I<3;++I)if(auto* U=R->GasMen[I].Get()){Fatal+=!U->IsAlive();GasDeath&=!U->IsAlive()&&U->GetDeathState().Kind==EFrontlineDamageKind::Gas&&!U->GetDeathState().ActualEntryClip.IsEmpty();}
            Check(TEXT("v8216_review_unmasked_gas_collapse"),Fatal==3&&GasDeath);
            if(auto* U=R->GasMen[3].Get())Check(TEXT("v8216_review_sealed_mask_postpones_lethal_dose"),U->IsAlive()&&FrontlineMotionV40::MaskSealed(U));
            Delay=.2;break;
        }
        case 6:
        {
            RemoveActors();B->Elapsed=180.f;const int32 Dir=B->GetTeamDirection(0);
            auto* Heavy=MakeMan(9821510,0,EFrontlineRole::AntiTankRifle,0,FVector(0,280,0));
            auto* Target=MakeMan(9821511,1,EFrontlineRole::Rifle,0,FVector(0,280,0));
            R->Heavy=Heavy;R->HeavyTarget=Target;
            bool ClearSite=false;FVector Chosen=FVector::ZeroVector;
            if(Heavy&&Target)
            {
                FCollisionQueryParams Q(SCENE_QUERY_STAT(V8215HeavyReviewSite),true);
                for(AWarfareFrontlineSoldier* Man:B->Units)if(Man)Q.AddIgnoredActor(Man);
                // Search an actual inter-trench approach. The old +/-750 fixture
                // put its target below a central parapet and ordered no advance.
                for(int32 Gap=0;Gap+1<B->Trenches.Num()&&!ClearSite;++Gap)
                    for(float Shift:{0.f,300.f,-300.f})for(float Lane:{280.f,-350.f,600.f,-600.f,0.f})
                    {
                        if(ClearSite)break;
                        const float Middle=(B->Trenches[Gap].X+B->Trenches[Gap+1].X)*.5f+Shift;
                        const float HX=Middle-Dir*600.f,TX=Middle+Dir*600.f;
                        const FVector HF(HX,Lane,AWarfareFrontlineGround::HeightAt(HX,Lane));
                        const FVector TF(TX,Lane,AWarfareFrontlineGround::HeightAt(TX,Lane));
                        Heavy->SetActorLocation(HF+FVector(0,0,88));Target->SetActorLocation(TF+FVector(0,0,88));
                        for(auto* Man:{Heavy,Target}){Man->Soldier->TickAnimation(.01f,false);Man->Soldier->RefreshBoneTransforms();}
                        const FVector LowMuzzle=HF+FVector(Dir*50.f,0,35.f);
                        const bool LowClear=!B->GetWorld()->LineTraceTestByChannel(LowMuzzle,TF+FVector(0,0,135.f),ECC_Visibility,Q);
                        if(LowClear&&B->HasSight(Heavy,Target)){ClearSite=true;Chosen=FVector(Middle,Lane,0);}
                    }
            }
            Check(TEXT("v8216_review_tankgewehr_site_has_actual_low_and_observation_sight"),ClearSite);
            auto* Bearer=Heavy?MakeMan(9821510,0,EFrontlineRole::AntiTankRifle,1,Heavy->Feet()+FVector(-Dir*45.f,130.f,0)):nullptr;
            if(Heavy&&Bearer)
            {
                FFrontlineSquad Crew;Crew.Id=9821510;Crew.Team=0;Crew.Role=EFrontlineRole::AntiTankRifle;
                Crew.Trench=INDEX_NONE;Crew.Destination=B->GetHomeTrenchIndex(1);Crew.bAdvancing=true;Crew.Members={Heavy,Bearer};B->Squads.Add(Crew);
            }
            Check(TEXT("v8216_review_tankgewehr_actual_two_man_advancing_section"),Heavy&&Bearer&&B->FindSquad(9821510));
            FFrontlineReviewV32::Camera(PC,Chosen.X,Chosen.Y,3600,125);R->SceneAt=Now;Delay=.03;break;
        }
        case 7:
        {
            auto* U=R->Heavy.Get();auto* Target=R->HeavyTarget.Get();
            const bool Fired=U&&U->ShotsDischarged>0;
            if(!Fired&&Now-R->SceneAt<10.){R->Next=Now+.01;return true;}
            auto D=MakeShared<FJsonObject>();D->SetStringField(TEXT("phase"),TEXT("tankgewehr_actual_tick_diagnostic"));
            if(U)
            {
                D->SetNumberField(TEXT("shots"),U->ShotsDischarged);D->SetStringField(TEXT("feet"),U->Feet().ToString());D->SetStringField(TEXT("muzzle"),U->GetMuzzleLocation().ToString());
                D->SetBoolField(TEXT("moving"),U->bMoving);D->SetNumberField(TEXT("weapon_phase"),int32(U->WeaponPhase));
                D->SetNumberField(TEXT("fire_range"),FrontlineWeapons::ProfileFor(U).FireRangeCm*B->EncounterVisibilityScale());
                D->SetBoolField(TEXT("acquired_target"),B->AcquireTarget(U)==Target);D->SetBoolField(TEXT("observed_sight"),Target&&B->HasSight(U,Target));
                if(auto* G=FrontlineMachinegunsV30::Get(U))
                {D->SetNumberField(TEXT("support_state"),int32(G->State));D->SetBoolField(TEXT("support_assets"),G->HasAssets());D->SetBoolField(TEXT("support_can_fire"),G->CanFire());D->SetBoolField(TEXT("muzzle_sight"),Target&&G->HasInfantryFiringLine(Target));}
                if(Target)
                {
                    D->SetStringField(TEXT("target_feet"),Target->Feet().ToString());D->SetBoolField(TEXT("target_alive"),Target->IsAlive());D->SetNumberField(TEXT("range"),FVector::Dist2D(U->Feet(),Target->Feet()));
                    FCollisionQueryParams Q(SCENE_QUERY_STAT(V8215HeavyReviewBlocker),true);for(AWarfareFrontlineSoldier* Man:B->Units)if(Man)Q.AddIgnoredActor(Man);
                    FHitResult Hit;const FVector Aim=Target->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5.f);
                    if(B->GetWorld()->LineTraceSingleByChannel(Hit,U->GetMuzzleLocation(),Aim,ECC_Visibility,Q))
                    {D->SetStringField(TEXT("blocking_actor"),GetNameSafe(Hit.GetActor()));D->SetStringField(TEXT("blocking_point"),Hit.ImpactPoint.ToString());}
                }
            }
            R->Phases.Add(MakeShared<FJsonValueObject>(D));
            if(Fired)Shot(TEXT("03-tankgewehr-real-discharge-recoil"));
            Check(TEXT("v8216_review_tankgewehr_engages_infantry"),Fired);Delay=.10;break;
        }
        case 8:Shot(TEXT("04-tankgewehr-committed-impact"));Delay=.3;break;
        case 9:
        {
            RemoveActors();const FVector At(0,350,AWarfareFrontlineGround::HeightAt(0,350)+8);
            R->Mine=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);
            if(auto* M=R->Mine.Get()){M->ConfigureMineV8214(600);M->SetCasualtyDustV8211(4);Check(TEXT("v8216_review_blood_mixed_mine_profile_ready"),M->IsVisualReady());}
            FFrontlineReviewV32::Camera(PC,0,350,4100,150);R->SceneAt=Now;Delay=.055;break;
        }
        case 10:Shot(TEXT("05-mine-flash-and-dust"));Delay=.25;break;
        case 11:Shot(TEXT("06-mine-blood-mixed-ballistic-soil"));Delay=.65;break;
        case 12:Shot(TEXT("07-mine-rolling-dust-volume"));Delay=.5;break;
        case 13:
            Clear();B->Phase=EFrontlinePhase::Briefing;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);S.Technology=true;S.TechnologyPage=2;Delay=1.;break;
        case 14:Shot(TEXT("08-armour-school-nine-cards-en"));R->AfterCapture=[](){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);};break;
        case 15:Shot(TEXT("09-armour-school-nine-cards-zh"));break;
        case 16:Clear();Check(TEXT("v8216_review_medal_display_seeded_only_in_disposable_account"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,true,3));FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);S.AchievementsV76=true;MedalWallV8215::View(PC).Wall=true;MedalWallV8215::View(PC).Nation=1;Delay=1.;break;
        case 17:Shot(TEXT("10-medal-wall-german-en"));R->AfterCapture=[](){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);};break;
        case 18:Shot(TEXT("11-medal-wall-german-zh"));break;
        case 19:Clear();FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);UltimateArchiveUIV8215::Open(PC,B);Delay=1.;break;
        case 20:Shot(TEXT("12-ultimate-archive-en"));R->AfterCapture=[](){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);};break;
        case 21:Shot(TEXT("13-ultimate-archive-zh"));break;

        case 22:
        {
            Clear();RemoveActors();B->Phase=EFrontlinePhase::Briefing;UltimateArchiveUIV8215::Open(PC,B);
            HUD->NotifyHitBoxClick(TEXT("V8215ArchiveFaction0"));
            HUD->NotifyHitBoxClick(TEXT("V8215ArchiveMission11"));
            HUD->NotifyHitBoxClick(TEXT("V8215ArchiveDifficulty2"));
            const auto& A=UltimateArchiveUIV8215::View(PC);
            Check(TEXT("v8216_archive_actual_HUD_clicks_select_faction_mission_difficulty"),A.Faction==EFrontlineFaction::British&&A.Mission==11&&A.Difficulty==2);
            HUD->NotifyHitBoxClick(TEXT("V8215ArchiveBegin"));
            Check(TEXT("v8216_archive_actual_prepare_click_enters_full_armory_deployment"),FrontlineUltimateArchiveV8215::Active(B)&&S.Deployment&&B->GetCurrentCampaign()&&B->GetCurrentCampaign()->SelectedMission==11);
            HUD->NotifyHitBoxClick(TEXT("V63DeploymentResearch"));
            const int32 Points=B->GetResearchPoints();
            HUD->NotifyHitBoxClick(TEXT("V63ArmourBuy0"));
            Check(TEXT("v8216_archive_armour_option_can_be_removed_no_cost"),!FrontlineArmourV63::Has(B,0)&&B->GetResearchPoints()==Points);
            HUD->NotifyHitBoxClick(TEXT("V63ArmourBuy0"));
            Check(TEXT("v8216_archive_armour_option_can_be_re_equipped"),FrontlineArmourV63::Has(B,0));
            HUD->NotifyHitBoxClick(TEXT("TechBuy0"));
            Check(TEXT("v8216_archive_company_option_can_be_removed_no_cost"),!B->HasTechnology(0)&&B->GetResearchPoints()==Points);
            HUD->NotifyHitBoxClick(TEXT("TechBuy0"));
            Check(TEXT("v8216_archive_company_option_can_be_re_equipped"),B->HasTechnology(0));
            FrontlineUltimateArchiveV8215::Leave(B);Clear();S.AchievementsV76=true;MedalWallV8215::View(PC).Wall=true;
            MedalWallV8215::View(PC).Nation=0;MedalWallV8215::View(PC).Focus=2;
            FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);Delay=.8;break;
        }
        case 23:Shot(TEXT("14-medal-wall-britain-en"));R->AfterCapture=[](){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);};break;
        case 24:Shot(TEXT("15-medal-wall-britain-zh"));R->AfterCapture=[PC](){MedalWallV8215::View(PC).Nation=2;MedalWallV8215::View(PC).Focus=6;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);};break;
        case 25:Shot(TEXT("16-medal-wall-france-en"));R->AfterCapture=[](){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);};break;
        case 26:Shot(TEXT("17-medal-wall-france-zh"));R->AfterCapture=[PC](){MedalWallV8215::View(PC).Nation=3;MedalWallV8215::View(PC).Focus=31;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);};break;
        case 27:Shot(TEXT("18-medal-wall-russia-en"));R->AfterCapture=[](){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);};break;
        case 28:Shot(TEXT("19-medal-wall-russia-zh"));R->AfterCapture=[PC](){auto& V=MedalWallV8215::View(PC);V.Nation=1;V.Focus=3;V.PreviewGrade=2;V.Inspect=true;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);};break;
        case 29:
            Check(TEXT("v8216_inspector_contains_real_Iron_Cross_I_variant"),MedalWallV8215::View(PC).PreviewGrade==2);
            Shot(TEXT("20-medal-inspector-iron-cross-I-en"));R->AfterCapture=[](){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);};break;
        case 30:Shot(TEXT("21-medal-inspector-iron-cross-I-zh"));R->AfterCapture=[PC,Check](){PC->MenuForwardV61();Check(TEXT("v8216_inspector_side_forward_keeps_visible_modal"),MedalWallV8215::View(PC).Inspect);PC->MenuBackV61();Check(TEXT("v8216_inspector_side_back_closes_only_modal"),!MedalWallV8215::View(PC).Inspect&&MedalWallV8215::View(PC).Wall);};Delay=.3;break;
        case 31:
        {
            Clear();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=180;
            const auto Earned=FrontlineMedalsV8215::NationIndices(EFRRUNationV81(int32(B->GetPlayerFaction())));
            int32 I=INDEX_NONE;for(int32 N:Earned)if(MedalMountV8216::DefaultGrade(N)>0){I=N;break;}
            Check(TEXT("v8216_mount_fixture_has_actual_earned_medal"),I>=0);
            HUD->NotifyHitBoxClick(TEXT("V8216MedalMount"));
            const int32 G=MedalMountV8216::DefaultGrade(I);HUD->NotifyHitBoxClick(FName(*FString::Printf(TEXT("V8216MountSelect%d_%d"),I,G)));
            Check(TEXT("v8216_mount_actual_click_sets_only_current_army_earned_medal"),MedalMountV8216::Chosen(MedalMountV8216::View(PC),B)==I);
            Check(TEXT("v8216_mount_click_preserves_explicit_real_grade"),MedalMountV8216::SelectedGrade(MedalMountV8216::View(PC))==G);
            HUD->NotifyHitBoxClick(TEXT("V8216MedalMount"));const int32 Men=B->Units.Num();const int32 Phase=int32(B->Phase);PC->ToggleSettings();PC->ToggleTechnology();PC->ToggleHelp();PC->ToggleAchievementsV76();PC->Restart();PC->RecruitRifle();PC->ToggleArtillery();PC->ToggleSmoke();PC->ToggleMortar();PC->ToggleRally();
            Check(TEXT("v8216_picker_blocks_hidden_overlay_and_battle_shortcuts"),MedalMountV8216::View(PC).Picker&&!S.Settings&&!S.Technology&&!S.Help&&!S.AchievementsV76&&B->Units.Num()==Men&&int32(B->Phase)==Phase&&PC->SupportTargetMode==0);PC->MenuForwardV61();Check(TEXT("v8216_picker_side_forward_keeps_visible_modal"),MedalMountV8216::View(PC).Picker);
            HUD->NotifyHitBoxClick(TEXT("V8216MountSelect3_2"));Check(TEXT("v8216_picker_rejects_pin_IronCross_I_without_losing_choice"),MedalMountV8216::View(PC).Picker&&MedalMountV8216::Chosen(MedalMountV8216::View(PC),B)==I&&MedalMountV8216::SelectedGrade(MedalMountV8216::View(PC))==G);
            PC->MenuBackV61();Check(TEXT("v8216_picker_side_back_does_not_leave_hidden_click_blocker"),!MedalMountV8216::View(PC).Picker);
            MedalMountV8216::View(PC).Picker=true;B->Phase=EFrontlinePhase::Victory;PC->ToggleSettings();Check(TEXT("v8216_ended_battle_keyboard_clears_hidden_picker_before_routing"),!MedalMountV8216::View(PC).Picker&&S.Settings);PC->ToggleSettings();B->Phase=EFrontlinePhase::Fighting;

            FFrontlineReviewV32::Camera(PC,0,350,4100,150);Delay=.65;break;
        }
        case 32:Shot(TEXT("22-battle-hanging-medal"));R->AfterCapture=[HUD](){HUD->NotifyHitBoxClick(TEXT("V8216MedalMount"));};break;
        case 33:{bool PickerArtReady=true;const auto& Picker=MedalMountV8216::View(PC);for(int32 I:FrontlineMedalsV8215::NationIndices(EFRRUNationV81(Picker.Nation)))for(int32 G:MedalMountV8216::HangableGrades(I))if(G<=FrontlineMedalsV8215::Tier(I))PickerArtReady&=FrontlineTextureReady(MedalWallV8215::Art(I,G));Check(TEXT("v8216_first_picker_has_every_earned_hanging_class_texture_ready"),PickerArtReady);}Shot(TEXT("23-earned-medal-picker-zh"));R->AfterCapture=[HUD,PC](){HUD->NotifyHitBoxClick(TEXT("V8216MountClose"));FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);};break;
        case 34:
        {
            RemoveActors();const FVector At(0,350,AWarfareFrontlineGround::HeightAt(0,350));
            for(int32 K=0;K<5;++K){auto* U=MakeMan(9821600+K,0,EFrontlineRole::Rifle,K,At+FVector(-140+K*60,(K%2)*90-30,0));R->GasMen.Add(U);}
            FFrontlineStrike Strike;Strike.Team=1;Strike.Target=At;Strike.Origin=At+FVector(-200,0,1800);Strike.Radius=600;
            B->ResolveFieldStrikeV32(Strike);int32 Fatal=0;for(const auto& W:R->GasMen)if(auto* U=W.Get())Fatal+=!U->IsAlive();
            Check(TEXT("v8216_visual_actual_blast_fixture_has_real_casualties"),Fatal>0);
            for(TActorIterator<AWarfareFrontlineImpact> A(B->GetWorld());A;++A)if(A->GetActorLocation().Equals(At,.1f))R->Mine=*A;
            Check(TEXT("v8216_visual_actual_blast_has_incident_casualty_sources"),R->Mine.IsValid());
            R->SceneAt=Now;Delay=.07;break;
        }
        case 35:Shot(TEXT("24-actual-blast-firelight-and-incidental-blood"));Delay=.22;break;
        case 36:Shot(TEXT("25-actual-blast-blood-inside-volume-dust"));Delay=.6;break;
        case 37:Shot(TEXT("26-actual-blast-rolling-ground-plume"));Delay=.2;break;
        case 38:
        {
            Clear();RemoveActors();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
            if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld()))Fire->Clear();
            const float Gap=B->Trenches.Num()>1?(B->Trenches[0].X+B->Trenches[1].X)*.5f:0.f;
            auto* U=MakeMan(9821638,0,EFrontlineRole::Rifle,0,FVector(Gap,350,0));R->Burning=U;
            if(U)U->ReceiveHit(1.f,.9f,1,FVector(1,.25f,.1f),EFrontlineDamageKind::Fire,.15f);
            Check(TEXT("v8216_capture_real_fire_casualty"),U&&!U->IsAlive()&&U->GetDeathState().Kind==EFrontlineDamageKind::Fire);
            R->BurnAtHost=B->Elapsed;R->SceneAt=Now;FFrontlineReviewV32::Camera(PC,Gap,350,3300,150);Delay=.01;break;
        }
        case 39:
        {
            if(B->Elapsed-R->BurnAtHost<.85f){R->Next=Now+.01;return true;}
            const auto V=AFrontlineFuelFireV33::Get(B->GetWorld())->VisualStateV8214();
            Check(TEXT("v8216_actual_fire_casualty_produces_clothing_flames"),V.Clothing>0&&V.SourceRibbonsV8216>0);
            Shot(TEXT("28-real-burning-soldier"));Delay=.2;break;
        }
        case 40:
        {
            RemoveActors();if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld()))Fire->Clear();
            const float Gap=B->Trenches.Num()>1?(B->Trenches[0].X+B->Trenches[1].X)*.5f:0.f;
            const FVector P(Gap,350,AWarfareFrontlineGround::HeightAt(Gap,350));auto* V=B->GetWorld()->SpawnActor<AFrontlineTank>(P,FRotator::ZeroRotator);R->Wreck=V;
            const bool Ready=V&&V->Configure(B,EFrontlineVehicle::A7V,0,9821640);Check(TEXT("v8216_capture_actual_tank_assets_ready"),Ready);
            if(Ready){V->Simulate(.01f);V->ReceiveModuleHit(EFrontlineTankModule::Hull,V->GetHullIntegrity()+1.f);}
            Check(TEXT("v8216_capture_actual_hull_damage_causes_wreck"),Ready&&!V->IsOperational()&&V->GetHullIntegrity()==0.f);
            R->WreckAtHost=B->Elapsed;R->SceneAt=Now;const FVector At=V?V->GetActorLocation():P;FFrontlineReviewV32::Camera(PC,At.X,At.Y,3600,150);Delay=.01;break;
        }
        case 41:
        {
            if(B->Elapsed-R->WreckAtHost<2.f){R->Next=Now+.01;return true;}
            const auto V=AFrontlineFuelFireV33::Get(B->GetWorld())->VisualStateV8214();Check(TEXT("v8216_actual_wreck_produces_multi_source_flame_and_volume_soot"),V.Wreck>0&&V.SourceRibbonsV8216>0&&V.VolumeSootV8216>0);
            Shot(TEXT("29-real-armour-wreck-multi-source-fire"));Delay=.2;break;
        }
        default:return Finish();
        }
        R->Next=Now+Delay;++R->Stage;return true;
    }),0.f);
}
}
