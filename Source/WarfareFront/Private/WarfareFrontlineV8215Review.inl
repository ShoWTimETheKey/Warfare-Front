#include "WarfareFrontlineSupportV28.h"
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

namespace
{
// One disposable production-world run. Account mutation is allowed only under
// the same explicit archive-review boundary used by the account implementation.
void StartFrontlineV8215Review(AWarfareFrontlineController* Controller)
{
    FString User;
    if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8215Review"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV8215ArchiveReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||FPaths::IsRelative(User))return;
    User=FPaths::ConvertRelativePathToFull(User);FPaths::NormalizeDirectoryName(User);
    const FString Allowed=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8215/ArchiveReview/Run"));
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
            J->SetStringField(TEXT("scope"),TEXT("Isolated German mission 8 production preparation; focused support, mine, gas/action and account/archive checks; 4K SDR High screenshots. Gas casualties use actual single-dose exposure. Mine captures are cosmetic blood/dust profile phases. No natural campaign, HDR or performance certification."));
            if(B)if(auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))J->SetObjectField(TEXT("render"),Settings->GetDiagnostics());
            FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Directory/TEXT("checks.json")));
            FPlatformMisc::RequestExit(false);return false;
        };
        if(!B||!HUD){Check(TEXT("v8215_review_world_and_hud_present"),false);return Finish();}
        if(!B->IsProgressionVolatile()){Check(TEXT("v8215_review_never_operates_on_player_profile"),false);return Finish();}
        if(Now-R->Start>=118.){Check(TEXT("v8215_review_bounded_120_second_run"),false);return Finish();}
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
                if(U->IsAlive()){FrontlineMotionV40::TickAction(U,DT);FrontlineMotionV40::TickExposure(U,DT);}
                else U->TickDeath(DT);
            }
        }
        if(R->Stage>=7&&R->Stage<=8)
        {
            B->Elapsed+=DT;
            if(auto* U=R->Heavy.Get())U->Tick(DT);
            FrontlineSpecialistsV41::Tick(B);
            if(auto* U=R->HeavyTarget.Get();U&&!U->IsAlive())U->TickDeath(DT);
        }
        if(FScreenshotRequest::IsScreenshotRequested())return true;
        if(R->AfterCapture){auto Action=MoveTemp(R->AfterCapture);Action();}
        if(Now<R->Next)return true;
        auto Clear=[&]()
        {
            UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.UnitIntroduction=S.Preparing=S.Deployment=S.HistoricalDispatch=S.CampaignEndingV51=S.AchievementsV76=false;
            S.bPausedByOverlay=S.bPausedForAuxOverlay=false;MedalWallV8215::View(PC).Wall=false;UltimateArchiveUIV8215::Close(PC);PC->CancelTargeting();
        };
        auto RemoveActors=[&]()
        {
            for(const auto& WeakUnit:B->Units)if(auto* U=WeakUnit.Get())U->Destroy();B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();
            FrontlineMotionV40::Reset(B);FrontlineSpecialistsV41::Reset(B);
            for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)if(IsValid(*I))I->Destroy();
            if(auto* Gas=R->Gas.Get())Gas->Destroy();R->Gas.Reset();R->GasMen.Empty();R->Heavy.Reset();R->HeavyTarget.Reset();
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
                const auto G=FrontlineMotionV40::GasExposureV8215(U);auto M=MakeShared<FJsonObject>();M->SetNumberField(TEXT("member"),U->MemberIndex);M->SetBoolField(TEXT("alive"),U->IsAlive());M->SetBoolField(TEXT("sealed"),G.Sealed);M->SetNumberField(TEXT("dose"),G.Dose);M->SetNumberField(TEXT("threshold"),G.Threshold);M->SetStringField(TEXT("incoming_clip"),U->GetDeathState().ActualEntryClip);M->SetStringField(TEXT("authored_death_clip"),GetPathNameSafe(U->GetDeathState().AuthoredAnimation.Get()));Men.Add(MakeShared<FJsonValueObject>(M));Deaths+=!U->IsAlive();
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
            Check(TEXT("v8215_review_starts_in_empty_briefing"),B->Phase==EFrontlinePhase::Briefing&&B->Units.IsEmpty());
            if(!R->Passed)return Finish();
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
            {Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);}
            FrontlineAchievementsV76::RunMedalChecksV8215(B,Check);
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
            Check(TEXT("v8215_review_german_campaign_created"),B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran));
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))
            {Run->CompletedMissions=14;Run->WonOperationMaskV52=(1u<<14)-1u;Run->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1u;Run->Research.ResearchPoints=30;Run->ArmourResearchMaskV63=511;}
            Check(TEXT("v8215_review_german_mission_8_selected"),B->SelectCampaignMission(7));
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
            Check(TEXT("v8215_review_production_preparation_ready"),Ready);if(!Ready)return Finish();
            int32 W=0,H=0;PC->GetViewportSize(W,H);Check(TEXT("v8215_review_actual_4k_viewport"),W==3840&&H==2160);
            B->SetActorTickEnabled(false);Clear();
            RunFrontlineSupportRulesChecksV8215(B,Check);FrontlineFieldV32::RunMineRulesChecksV8215(B,Check);FrontlineMotionV40::RunFocusedChecksV8215(B,Check);
            RemoveActors();B->Elapsed=180.f;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
            FFrontlineReviewV32::Camera(PC,0,200,4200,180);Delay=1.;break;
        }
        case 2:
        {
            R->GasMen.Add(MakeMan(9821501,0,EFrontlineRole::Rifle,0,FVector(-180,100,0)));
            R->GasMen.Add(MakeMan(9821502,0,EFrontlineRole::Rifle,1,FVector(0,260,0)));
            R->GasMen.Add(MakeMan(9821503,0,EFrontlineRole::Rifle,2,FVector(170,100,0)));
            R->GasMen.Add(MakeMan(9821504,0,EFrontlineRole::Mills,0,FVector(40,450,0)));
            const FVector At(0,200,AWarfareFrontlineGround::HeightAt(0,200));auto* Gas=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
            const bool Configured=Gas&&Gas->Configure(B,EFrontlineSupport::Gas,At,1);R->Gas=Gas;if(Gas)Gas->SetActorTickEnabled(false);
            Check(TEXT("v8215_review_real_gas_support_configured"),Configured);R->SceneAt=Now;Delay=.1;break;
        }
        case 3:
            if(auto* Gas=R->Gas.Get();Gas&&Gas->RoundsFired>0){R->SceneAt=Now;Delay=9.2;break;}
            if(Now-R->SceneAt<35.){R->Next=Now+.1;return true;}Check(TEXT("v8215_review_gas_arrived_in_bound"),false);return Finish();
        case 4:
            Shot(TEXT("01-gas-six-clouds-and-choking"));
            if(auto* Gas=R->Gas.Get()){TArray<UStaticMeshComponent*> C;Gas->GetComponents<UStaticMeshComponent>(C);int32 Visible=0;for(auto* V:C)Visible+=V&&V->IsVisible();Check(TEXT("v8215_review_all_six_persistent_gas_volumes_visible"),Visible==6);}
            Delay=2.4;break;
        case 5:
        {
            Shot(TEXT("02-gas-authored-collapse"));int32 Fatal=0;bool GasDeath=true;
            for(int32 I=0;I<3;++I)if(auto* U=R->GasMen[I].Get()){Fatal+=!U->IsAlive();GasDeath&=!U->IsAlive()&&U->GetDeathState().Kind==EFrontlineDamageKind::Gas&&!U->GetDeathState().ActualEntryClip.IsEmpty();}
            Check(TEXT("v8215_review_unmasked_gas_collapse"),Fatal==3&&GasDeath);
            if(auto* U=R->GasMen[3].Get())Check(TEXT("v8215_review_sealed_mask_postpones_lethal_dose"),U->IsAlive()&&FrontlineMotionV40::MaskSealed(U));
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
            Check(TEXT("v8215_review_tankgewehr_site_has_actual_low_and_observation_sight"),ClearSite);
            auto* Bearer=Heavy?MakeMan(9821510,0,EFrontlineRole::AntiTankRifle,1,Heavy->Feet()+FVector(-Dir*45.f,130.f,0)):nullptr;
            if(Heavy&&Bearer)
            {
                FFrontlineSquad Crew;Crew.Id=9821510;Crew.Team=0;Crew.Role=EFrontlineRole::AntiTankRifle;
                Crew.Trench=INDEX_NONE;Crew.Destination=B->GetHomeTrenchIndex(1);Crew.bAdvancing=true;Crew.Members={Heavy,Bearer};B->Squads.Add(Crew);
            }
            Check(TEXT("v8215_review_tankgewehr_actual_two_man_advancing_section"),Heavy&&Bearer&&B->FindSquad(9821510));
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
            Check(TEXT("v8215_review_tankgewehr_engages_infantry"),Fired);Delay=.10;break;
        }
        case 8:Shot(TEXT("04-tankgewehr-committed-impact"));Delay=.3;break;
        case 9:
        {
            RemoveActors();const FVector At(0,350,AWarfareFrontlineGround::HeightAt(0,350)+8);
            R->Mine=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);
            if(auto* M=R->Mine.Get()){M->ConfigureMineV8214(600);M->SetCasualtyDustV8211(4);Check(TEXT("v8215_review_blood_mixed_mine_profile_ready"),M->IsVisualReady());}
            FFrontlineReviewV32::Camera(PC,0,350,4100,150);R->SceneAt=Now;Delay=.055;break;
        }
        case 10:Shot(TEXT("05-mine-flash-and-dust"));Delay=.25;break;
        case 11:Shot(TEXT("06-mine-blood-mixed-ballistic-soil"));Delay=.65;break;
        case 12:Shot(TEXT("07-mine-rolling-dust-volume"));Delay=.5;break;
        case 13:
            Clear();B->Phase=EFrontlinePhase::Briefing;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);S.Technology=true;S.TechnologyPage=2;Delay=1.;break;
        case 14:Shot(TEXT("08-armour-school-nine-cards-en"));R->AfterCapture=[](){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);};break;
        case 15:Shot(TEXT("09-armour-school-nine-cards-zh"));break;
        case 16:Clear();Check(TEXT("v8215_review_medal_display_seeded_only_in_disposable_account"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,true,3));FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);S.AchievementsV76=true;MedalWallV8215::View(PC).Wall=true;MedalWallV8215::View(PC).Nation=1;Delay=1.;break;
        case 17:Shot(TEXT("10-medal-wall-german-en"));R->AfterCapture=[](){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);};break;
        case 18:Shot(TEXT("11-medal-wall-german-zh"));break;
        case 19:Clear();FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);UltimateArchiveUIV8215::Open(PC,B);Delay=1.;break;
        case 20:Shot(TEXT("12-ultimate-archive-en"));R->AfterCapture=[](){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);};break;
        case 21:Shot(TEXT("13-ultimate-archive-zh"));break;
        case 22:Clear();RemoveActors();B->RestartBattle();Check(TEXT("v8215_review_post_victory_assignment_seed"),FrontlineAchievementsV76::SeedNewUnlockReviewV8215(B,EFrontlineFaction::British,0));FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);Delay=1.;break;
        case 23:Shot(TEXT("14-next-assignment-en"));R->AfterCapture=[](){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);};break;
        case 24:Shot(TEXT("15-next-assignment-zh"));Delay=.4;break;
        default:return Finish();
        }
        R->Next=Now+Delay;++R->Stage;return true;
    }),0.f);
}
}
