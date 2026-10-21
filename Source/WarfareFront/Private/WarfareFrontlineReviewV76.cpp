#include "WarfareFrontlineReviewV76.h"
#include "WarfareFrontlineUIMotionV83.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineFrontsV76.h"
#include "WarfareFrontlineChoiceTargetsV78.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineEnvironmentV42.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineTerrainV42.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlinePrecipitation.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "PipelineStateCache.h"
#include "RHI.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#endif

namespace FrontlineReviewUIV41
{
void HonoursInputChecksV76(AWarfareFrontlineController* PC,TFunctionRef<void(const FString&,bool)> Check);
}

namespace
{
bool AllowedV76(AWarfareFrontlineGameMode* B)
{FString Dir;return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty()&&!FPaths::IsRelative(Dir);}
bool CompiledV76()
{
#if WITH_EDITOR
    if(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling()))return false;
#endif
    return !GUsingNullRHI&&PipelineStateCache::GetNumActivePipelinePrecompileTasks()==0;
}
struct FSceneV76{int32 Op;bool Close;const TCHAR* Name;};
const FSceneV76 ScenesV76[]={{5,false,TEXT("op05-wide")},{5,true,TEXT("op05-bank-bags")},{7,false,TEXT("op07-wide")},{7,true,TEXT("op07-timber")},{17,false,TEXT("op17-wide")},{17,true,TEXT("op17-timber-bags")}};
}
struct FFrontlineReviewV76 : TSharedFromThis<FFrontlineReviewV76>
{
    TWeakObjectPtr<AWarfareFrontlineController> Controller;FString Dir,Shot;double Started=0,At=0,Quiet=0,Last=0;int32 Scene=-1,Step=0,LoadedOp=INDEX_NONE;bool Good=true,Logic=false,BadgesCheckedV82=false;
    bool Live=false,MortarOrdered=false,MenEnabled=false;float FirstImpact=0;FVector MortarTarget=FVector::ZeroVector;FString InitialSupportReason;FVector FocusPoint=FVector::ZeroVector;TArray<TSharedPtr<FJsonValue>> Checks,Frames;TArray<double> CPU,GPU,Intervals;TSharedPtr<FJsonObject> Row,FrontsDiagnostics;
    AWarfareFrontlineGameMode* Battle()const{return Controller.IsValid()?Cast<AWarfareFrontlineGameMode>(Controller->GetWorld()->GetAuthGameMode()):nullptr;}
    void Check(const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V76_CHECK %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));}
    void Finish()
    {
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("scenes"),Frames);J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Started);
        const bool LogicOnly=FParse::Param(FCommandLine::Get(),TEXT("WFV76LogicOnly"));J->SetBoolField(TEXT("logic_only"),LogicOnly);if(FrontsDiagnostics)J->SetObjectField(TEXT("fronts_real_actor_diagnostics"),FrontsDiagnostics);
        J->SetStringField(TEXT("scope"),LogicOnly?TEXT("Production morale handlers, front readout with exact real-actor diagnostics, isolated atomic account persistence and cheat guards; production H/O/T/R honours-page mutual exclusion, Escape/Return/mouse-side navigation and pause ownership. No screenshots or rendering/performance checks."):TEXT("Production morale handlers, front readout, isolated atomic account persistence and cheat guards; honours-page keyboard/mouse navigation; six actual 4K field-camera scenes on operations 5, 7 and 17 with held director, FG off and RR on; English/Chinese honours-page layout. Three-second warmed samples are scene diagnostics, not sustained combat performance or universal compatibility certification."));
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV78HonoursOnly")))J->SetStringField(TEXT("scope"),TEXT("Production logic, isolated event/atomic account and unified-list scroll/navigation checks; all28 EN/ZH honours detail pages with readable fixed Chinese type and complete prose, and16 national/cross-faction EN/ZH banner fixtures at4K. Banner fixtures verify presentation, not natural unlock. No full-campaign or battlefield performance acceptance."));
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV83FactionOnly")))J->SetStringField(TEXT("scope"),TEXT("Production isolated event/account/navigation checks; collapsed and four expanded selector compositions in EN/ZH at4K. Every controller frame sampled during page entry and rendered width animation, no outlier exclusion, FG off. Render fixtures do not prove OS/manual input or battlefield performance."));
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV82BadgeReadyOnly")))J->SetStringField(TEXT("scope"),TEXT("Correction check: production event/account/navigation checks, all28 badges after real async asset completion, two unified EN/ZH page captures. Complete page/banner coverage belongs to the full visual run."));
        J->SetBoolField(TEXT("v78_actual_target_and_enemy_recruitment_checks"),FParse::Param(FCommandLine::Get(),TEXT("WFV78TacticsChecks")));
        FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);
    }
    bool Load(int32 Op)
    {
        auto* B=Battle();auto* PC=Controller.Get();if(!FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,Op))return false;
        B->BeginBattle();B->SetActorTickEnabled(false);B->Elapsed=10;FrontlineReviewUIV41::Clear(PC);
        for(int32 Team=0;Team<2;++Team)if(B->SpawnSquad(Team,Team?EFrontlineRole::Rifle:EFrontlineRole::Lewis,B->GetHomeTrenchIndex(Team)))
            for(const auto& U:B->Squads.Last().Members)if(U.IsValid()){U->bFieldEntered=true;U->SetActorTickEnabled(false);}
        LoadedOp=Op;return B->Phase==EFrontlinePhase::Fighting;
    }
    FVector Focus(bool Trees)const
    {
        auto* B=Battle();FVector P(0,-350,0);float Best=MAX_flt;
        for(TActorIterator<AActor> It(B->GetWorld());It;++It)
        {
            TArray<UInstancedStaticMeshComponent*> Parts;It->GetComponents(Parts);
            for(auto* C:Parts)if(C&&C->GetStaticMesh())
            {
                const FString Name=C->GetStaticMesh()->GetName();const bool IsTree=Name.Contains(TEXT("SplinteredTrunk"))||Name.Contains(TEXT("SplitStump"))||Name.Contains(TEXT("FallenTimber"));const bool IsBag=Name.Contains(TEXT("FiringBay"));
                if(!Name.EndsWith(TEXT("V76"))||(Trees?!IsTree:!IsBag))continue;
                for(int32 I=0;I<C->GetInstanceCount();++I){FTransform T;C->GetInstanceTransform(I,T,true);const FVector Q=T.GetLocation();const float Score=FMath::Abs(Q.X)+FMath::Abs(Q.Y)*.3f;if(Score<Best){Best=Score;P=Q;}}
            }
        }
        return P;
    }
    bool Tick()
    {
        auto* PC=Controller.Get();auto* B=Battle();const double Now=FPlatformTime::Seconds();
        const bool HonoursOnly=FParse::Param(FCommandLine::Get(),TEXT("WFV78HonoursOnly"));
        const bool FactionOnly=FParse::Param(FCommandLine::Get(),TEXT("WFV83FactionOnly"));
        if(!B||Now-Started>(HonoursOnly?420:150)){Check(TEXT("v76_review_bounded"),false);Finish();return false;}
        if(!B->HasActorBegunPlay()||!B->LoadedSoldier||!B->IdleAnimation)return true;
        if(!AllowedV76(B)){Check(TEXT("v76_isolated_profile"),false);Finish();return false;}
        if(!B->AssetFailure.IsEmpty()){Check(TEXT("v76_required_assets_ready"),false);Finish();return false;}
        if(!Logic)
        {
            Check(TEXT("v76_operation05_actual_load"),Load(5));auto F=[this](const FString& N,bool P){Check(N,P);};
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV78TacticsChecks")))
            {
                // The visual scene loader creates two squads. Use the real
                // abandonment path to give isolated combat fixtures an empty
                // encounter, then rebuild the ordinary scene below.
                B->AbandonBattleToOperations();
                FrontlineChoiceTargetsV78::RunChecks(B,F);B->AbandonBattleToOperations();
                Check(TEXT("v78_restore_operation05_after_target_checks"),Load(5));
            }
            const auto Units=B->Units;B->Units.Empty();FrontlineUIMotionV83::RunChecks(F);FrontlineFrontsV76::RunChecks(B,F,&FrontsDiagnostics);B->Units=Units;FrontlineMoraleV36::RunChecksV76(B,F);FrontlineAchievementsV76::RunChecks(B,F);
            FrontlineReviewUIV41::HonoursInputChecksV76(PC,F);
            Logic=true;Scene=HonoursOnly||FactionOnly?6:0;Step=0;
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV76LogicOnly"))){Finish();return false;}
        }
        const int32 EndScene=FactionOnly?16:HonoursOnly&&!FParse::Param(FCommandLine::Get(),TEXT("WFV82BadgeReadyOnly"))?6+FrontlineAchievementsV76::Count*2+(FrontlineAchievementsV76::Count-12)*2:8;
        if(Scene>=EndScene){Finish();return false;}
        if(Step==0)
        {
            if(Scene<6)
            {
                const auto& S=ScenesV76[Scene];if(LoadedOp!=S.Op)Check(FString::Printf(TEXT("v76_operation%02d_actual_load"),S.Op),Load(S.Op));
                FrontlineReviewUIV41::Clear(PC);FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);
                int32 VW=0,VH=0;PC->GetViewportSize(VW,VH);Check(TEXT("v76_actual_viewport_is_4k"),VW==3840&&VH==2160);
                const float Width=S.Close?3400.f:6000.f;FVector P=S.Close?Focus(S.Op!=5):FVector(0,0,0);
                const float Limit=FrontlineTerrainV42::CameraVerticalLimitV43(Width,VH>0?float(VW)/VH:16.f/9.f);P.Y=FMath::Clamp(float(P.Y),-Limit,Limit);
                if(!S.Close)P.Z=650;else P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y)+160;
                FocusPoint=P;FrontlineReviewUIV41::Camera(PC,P.X,P.Y,Width,P.Z,true);
                if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){R->SetHDREnabled(false,false);R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);R->SetRayReconstruction(true,false);}
                Live=Scene==4;FirstImpact=0;MortarOrdered=MenEnabled=false;InitialSupportReason.Empty();
                if(Live)
                {
                    for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Empty();B->Squads.Empty();
                    // Both forces share an actual open inter-trench sector.
                    // Invalid holding squads formerly marched towards an enemy
                    // end-line fallback and never settled to discharge a shot.
                    const float Sector=(B->Trenches[0].X+B->Trenches[1].X)*.5f;
                    const float Separation=FMath::Min(900.f,(B->Trenches[1].X-B->Trenches[0].X)*.24f);
                    MortarTarget=FVector(Sector+Separation,100,0);
                    for(int32 Team=0;Team<2;++Team)for(int32 Group=0;Group<3;++Group)if(B->SpawnSquad(Team,Group==0?EFrontlineRole::Lewis:EFrontlineRole::Rifle))
                    {
                        auto& Q=B->Squads.Last();Q.Trench=INDEX_NONE;Q.bAdvancing=true;Q.bReinforcement=false;
                        Q.LastPassedTrench=Team==0?0:1;Q.Destination=Team==0?1:0;
                        for(const auto& U:Q.Members)if(U.IsValid())
                        {
                            FVector Foot(Sector-B->GetTeamDirection(Team)*(Separation+Group*150.f),-180.f+Group*190.f+U->MemberIndex*35.f,0);
                            Foot.Z=FrontlineInfantryV45::SurfaceHeight(B,Foot)+88;U->SetActorLocation(Foot);U->bFieldEntered=true;U->SetActorTickEnabled(false);
                        }
                    }
                    B->EnemyThinkAt=B->EnemyRecruitAt=B->EnemyFireAssessAt=B->EnemyAssaultAt=B->ExtendedSupportThinkAt=MAX_flt;
                    B->SetActorTickEnabled(true);InitialSupportReason=B->MortarBlockReason();
                    Check(TEXT("v76_live_field_mortar_is_actually_equipped"),B->IsSupportUnlocked(EFrontlineSupport::Mortar)&&FrontlineCampaignV32::SupportBlockReason(B,EFrontlineSupport::Mortar).IsEmpty());
                }
                Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("scene"),S.Name);Row->SetNumberField(TEXT("operation"),S.Op);Row->SetNumberField(TEXT("width_cm"),Width);Row->SetStringField(TEXT("focus"),P.ToString());
                Shot=Dir/(FString(S.Name)+TEXT(".png"));
            }
            else if(FactionOnly)
            {
                const int32 Case=Scene-6,Army=Case/2-1;const bool ZH=(Case%2)==1;
                FrontlineLocalizationV35::SetLanguage(ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
                FrontlineReviewUIV41::FactionV83(PC,Army,Army>=0);FrontlineReviewUIV41::BeginAudit();
                Row=MakeShared<FJsonObject>();const FString Name=FString::Printf(TEXT("faction-%d-%s"),Army,ZH?TEXT("zh"):TEXT("en"));
                Row->SetStringField(TEXT("scene"),Name);Shot=Dir/(Name+TEXT(".png"));
            }
            else
            {
                const int32 Case=Scene-6;const bool ZH=(Case%2)==1,Banner=HonoursOnly&&Case>=FrontlineAchievementsV76::Count*2;
                const int32 Focus=HonoursOnly?(Banner?12+(Case-FrontlineAchievementsV76::Count*2)/2:Case/2):0;
                FrontlineLocalizationV35::SetLanguage(ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
                FrontlineReviewUIV41::HonoursV82(PC,Focus,Banner);FrontlineReviewUIV41::BeginAudit();Row=MakeShared<FJsonObject>();
                const FString Name=FString::Printf(TEXT("honours-%s-%s-%s"),Banner?TEXT("banner"):TEXT("page"),FrontlineAchievementsV76::Definition(Focus).Id,ZH?TEXT("zh"):TEXT("en"));
                Row->SetStringField(TEXT("scene"),Name);Row->SetNumberField(TEXT("achievement_index"),Focus);Row->SetBoolField(TEXT("banner_fixture"),Banner);Shot=Dir/(Name+TEXT(".png"));
            }
            At=Now;Quiet=0;Step=1;return true;
        }
        if(Scene<6)if(auto* View=Cast<ACameraActor>(PC->GetViewTarget())){int32 W=0,H=0;PC->GetViewportSize(W,H);FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),View->GetActorLocation(),FocusPoint,View->GetCameraComponent()->OrthoWidth,W);}
        if(Step==1)
        {
            if(Scene==4)
            {
                // Wait on the real starting readiness timer. Do not zero CD,
                // fast-forward simulation, or bypass the production request.
                if(!MortarOrdered)
                {
                    if(B->MortarCooldown()>.01f)return true;
                    const FString Reason=B->MortarBlockReason();const bool Ordered=B->RequestMortar(MortarTarget,0);
                    Row->SetStringField(TEXT("initial_support_block_reason"),InitialSupportReason);Row->SetStringField(TEXT("ready_support_block_reason"),Reason);
                    Row->SetNumberField(TEXT("support_requested_at_simulation_s"),B->Elapsed);Row->SetBoolField(TEXT("infantry_held_during_preparation"),true);
                    Check(TEXT("v76_live_field_actual_mortar_request"),Ordered);
                    if(!Ordered){Row->SetStringField(TEXT("support_request_failure"),B->LastMessage);Check(TEXT("v76_live_field_combines_real_shooting_and_support_impact"),false);Frames.Add(MakeShared<FJsonValueObject>(Row));Finish();return false;}
                    MortarOrdered=true;for(const auto& Strike:B->Strikes)if(Strike.bMortar&&(FirstImpact==0||Strike.ImpactAt<FirstImpact))FirstImpact=Strike.ImpactAt;
                    Check(TEXT("v76_live_field_request_scheduled_real_rounds"),FirstImpact>B->Elapsed);
                }
                if(!MenEnabled&&B->Elapsed>=FirstImpact-3.f)
                {for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->SetActorTickEnabled(true);MenEnabled=true;Row->SetNumberField(TEXT("infantry_live_from_simulation_s"),B->Elapsed);}
                if(!MenEnabled||B->MortarRoundsResolved==0)return true;
            }
            if(!CompiledV76()){Quiet=0;return true;}if(!Quiet)Quiet=Now;if(Now-At<2||Now-Quiet<.5)return true;
            if(Scene>=6&&!FactionOnly&&!BadgesCheckedV82)
            {FrontlineReviewUIV41::HonoursBadgeChecksV82(PC,[this](const FString& Name,bool Pass){Check(Name,Pass);});BadgesCheckedV82=true;}
            Last=At=Now;CPU.Reset();GPU.Reset();Intervals.Reset();Step=2;return true;
        }
        if(Step==2)
        {
            CPU.Add(FApp::GetDeltaTime()*1000);Intervals.Add((Now-Last)*1000);Last=Now;const double GPUSample=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());if(GPUSample>0)GPU.Add(GPUSample);
            if(Now-At<(Scene<6?3.:.4))return true;
            if(Scene>=6)
            {
                auto Audit=FrontlineReviewUIV41::EndAudit();Row->SetObjectField(TEXT("layout"),Audit);
                Check(Row->GetStringField(TEXT("scene"))+TEXT("_text_image_hitboxes_fit"),Audit->GetBoolField(TEXT("success")));
                if(HonoursOnly&&!Row->GetBoolField(TEXT("banner_fixture")))
                {
                    bool Readable=true;
                    for(const auto& V:Audit->GetArrayField(TEXT("typography")))
                    {
                        auto T=V->AsObject();const FString Text=T->GetStringField(TEXT("text"));bool Chinese=false;
                        for(TCHAR C:Text)Chinese|=C>=0x4e00&&C<=0x9fff;
                        const double X=T->GetNumberField(TEXT("x")),Y=T->GetNumberField(TEXT("y")),Size=T->GetNumberField(TEXT("font_size"));
                        if(Chinese&&Y>=208&&Y<936)Readable&=Size>=20;
                        if(Chinese&&X>=1120&&(Y==496||Y==692))Readable&=Size>=24;
                    }
                    Check(Row->GetStringField(TEXT("scene"))+TEXT("_chinese_readable_size"),Readable);
                }
                if(FactionOnly)Row->SetObjectField(TEXT("entry_and_animation_frames"),FrontlineReviewUIV41::FactionFramesV83());
            }
            Row->SetNumberField(TEXT("simulation_elapsed_s"),B->Elapsed);Row->SetBoolField(TEXT("world_paused"),UGameplayStatics::IsGamePaused(B));Row->SetBoolField(TEXT("simulation_frozen"),!Live);Row->SetBoolField(TEXT("ai_director_held"),true);Row->SetNumberField(TEXT("living_infantry"),B->LivingSoldiers(0)+B->LivingSoldiers(1));Row->SetNumberField(TEXT("shots_dispatched"),B->ShotsFired);Row->SetNumberField(TEXT("mortar_rounds_resolved"),B->MortarRoundsResolved);Row->SetNumberField(TEXT("active_support_rounds"),B->Strikes.Num());
            if(Scene==4)Check(TEXT("v76_live_field_combines_real_shooting_and_support_impact"),B->ShotsFired>0&&B->MortarRoundsResolved>0);
            double C=0,G=0,Interval=0;for(double V:CPU)C+=V;for(double V:GPU)G+=V;for(double V:Intervals)Interval+=V;Row->SetNumberField(TEXT("sampling_interval_ms"),Intervals.Num()?Interval/Intervals.Num():0);Row->SetNumberField(TEXT("frame_ms"),CPU.Num()?C/CPU.Num():0);Row->SetNumberField(TEXT("gpu_ms"),GPU.Num()?G/GPU.Num():0);Row->SetNumberField(TEXT("samples"),CPU.Num());
            if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Row->SetObjectField(TEXT("render"),R->GetDiagnostics());Check(TEXT("v76_diagnostics_fg_off_rr_on"),R->GetFrameGeneration()==EFrontlineFrameGeneration::Off&&R->GetRayReconstruction());}
            if(Scene<6)if(auto* Weather=AWarfareFrontlineWeather::Get(B->GetWorld()))if(auto* Rain=Weather->GetPrecipitation())
            {
                Row->SetNumberField(TEXT("active_rain_drops"),Rain->GetActiveDropCount());Row->SetNumberField(TEXT("active_rain_contacts"),Rain->GetActiveSplashCount());Row->SetStringField(TEXT("rain_visible_centre"),Rain->GetVisibleCentreV76().ToString());
                if(auto* View=Cast<ACameraActor>(PC->GetViewTarget()))
                {const FVector Origin=View->GetActorLocation(),Forward=View->GetActorForwardVector();FVector Expected=Origin+Forward*(-Origin.Z/Forward.Z);Expected=Origin+Forward*((AWarfareFrontlineGround::HeightAt(Expected.X,Expected.Y)-Origin.Z)/Forward.Z);Check(TEXT("v76_rain_pool_follows_actual_projected_camera"),FVector::Dist2D(Expected,Rain->GetVisibleCentreV76())<5.f);}
                int32 Samples=0,Wrong=0;for(const auto& Contact:Rain->GetRainContactsV76())if(Contact.bActive&&Contact.bRipple)
                {float Level=0,Depth=0;if(AWarfareFrontlineGround::QueryWaterSurfaceV51(Contact.Position.X,Contact.Position.Y,Level,Depth)){++Samples;Wrong+=FMath::Abs(Contact.Position.Z-Level-1.1f)>2.f;}}
                Row->SetNumberField(TEXT("water_ripple_samples"),Samples);Row->SetNumberField(TEXT("water_ripple_bad_height"),Wrong);if(Samples)Check(TEXT("v76_rain_ripples_land_on_horizontal_water_not_bed"),Wrong==0);
            }
            FScreenshotRequest::RequestScreenshot(Shot,true,false);At=Now;Step=3;return true;
        }
        if(Step==3)
        {if(FScreenshotRequest::IsScreenshotRequested()||Now-At<.4)return true;Check(TEXT("v76_actual_screenshot_saved"),IFileManager::Get().FileSize(*Shot)>100000);Frames.Add(MakeShared<FJsonValueObject>(Row));if(Live){B->SetActorTickEnabled(false);for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);}Live=false;++Scene;Step=0;}
        return true;
    }
};
void StartFrontlineReviewV76(AWarfareFrontlineController* PC)
{
    // WFV76Review supplies only the isolated account codec to this explicit
    // native parity driver; it does not request the legacy operation suite.
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87ParityReview")))return;
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV851Focused"))||FParse::Param(FCommandLine::Get(),TEXT("WFV844Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8450Review")))return;
    if(!PC||!FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV836Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV837Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV838Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV839Review")))return;auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());if(!AllowedV76(B))return;
    auto R=MakeShared<FFrontlineReviewV76>();R->Controller=PC;R->Started=FPlatformTime::Seconds();R->Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V76/Review");IFileManager::Get().MakeDirectory(*R->Dir,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float){return R->Tick();}),.03f);
}
