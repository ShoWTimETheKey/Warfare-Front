#include "WarfareFrontlineLockV39.h"
// Included after the production canvas/status helpers in Presentation.cpp.
// All mutations below require a disposable profile; no earlier review suite runs.
namespace
{
void StartFrontlineV39Review(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV39Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    FString UserDir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
    struct FRun
    {
        int32 Stage=0,Shots=0,OwnSquad=INDEX_NONE,Original=0,Remaining=0;bool Passed=true,HDRSupported=false;
        double Next=FPlatformTime::Seconds()+3,Start=Next;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
        TWeakObjectPtr<AWarfareFrontlineSoldier> OwnMan,EnemyMan;
        TWeakObjectPtr<AFrontlineTank> OwnTank,EnemyTank;
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V39/Review"));
    IFileManager::Get().MakeDirectory(*R->Directory,true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    V27ImageAuditActive=V36TypographyAuditActive=true;V36TypographyAudit.Empty();V28ReviewPointer=FVector2D(1780,110);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
    {
        const double Now=FPlatformTime::Seconds();auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;if(!B||!HUD)return false;if(Now<R->Next)return true;
        auto& S=ScreenState(PC);auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const TCHAR* Name){++R->Shots;FScreenshotRequest::RequestScreenshot(R->Directory/(FString(Name)+TEXT(".png")),true,false);};
        auto Clear=[&]()
        {
            UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.UnitIntroduction=S.Preparing=S.Deployment=S.HistoricalDispatch=false;
            S.bPausedByOverlay=S.bPausedForAuxOverlay=false;PC->CancelTargeting();V28ReviewPointer=FVector2D(1780,110);
        };
        auto Select=[&](EFrontlineFaction F,int32 Mission)
        {
            Clear();B->AbandonBattleToOperations();B->StartNewCampaign(F,EFrontlineDifficulty::Veteran);
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))Run->CompletedMissions=Mission;
            Check(TEXT("selected_operation_")+FString::FromInt(R->Stage),B->SelectCampaignMission(Mission));
            S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;PC->ResetCamera();B->SetActorTickEnabled(false);
        };
        auto Deploy=[&]()
        {
            const auto L=FrontlineCampaignV32::GetLoadout(B);S.Deployment=true;
            // This early-operation screen has genuine locked entries. Optional
            // unselection is a later capacity mechanic, not a forced fixture flag.
            Check(TEXT("deployment_has_available_and_locked_roles_")+FString::FromInt(R->Stage),
                (L.EligibleRoles&(1u<<int32(EFrontlineRole::Lewis)))&&(L.SelectedRoles&(1u<<int32(EFrontlineRole::Lewis)))&&!B->IsRoleUnlocked(EFrontlineRole::Marksman));
            Check(TEXT("deployment_has_available_and_locked_support_")+FString::FromInt(R->Stage),
                (L.EligibleSupports&(1u<<int32(EFrontlineSupport::Smoke)))&&(L.SelectedSupports&(1u<<int32(EFrontlineSupport::Smoke)))&&!B->IsSupportUnlocked(EFrontlineSupport::Gas));
        };
        auto Begin=[&]()
        {
            Clear();if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){Run->RoleSelectionMask=0;Run->SupportSelectionMask=0;}
            B->BeginBattle();B->Elapsed=130;B->SetActorTickEnabled(false);
            B->ArtilleryReadyAt[0]=B->Elapsed+35;B->SmokeReadyAt[0]=B->MortarReadyAt[0]=0;
            FFrontlineReviewV32::Camera(PC,0,0,5400,180);
            Check(TEXT("battle_has_cooling_and_ready_support_")+FString::FromInt(R->Stage),B->ArtilleryCooldown()>0&&B->SmokeCooldown()==0&&B->IsSupportUnlocked(EFrontlineSupport::Smoke));
        };
        auto PointAt=[&](const FVector& World)
        {
            FVector2D P;int32 W=0,H=0;PC->GetViewportSize(W,H);
            if(!PC->ProjectWorldLocationToScreen(World,P)||W<=0||H<=0){V28ReviewPointer=FVector2D(-1000,-1000);return false;}
            const float Scale=FMath::Min(W/1920.f,H/1080.f);
            P=FVector2D((P.X-(W-1920.f*Scale)*.5f)/Scale,(P.Y-(H-1080.f*Scale)*.5f)/Scale);
            V28ReviewPointer=P;return P.X>0&&P.X<1920&&P.Y>75&&P.Y<830;
        };
        auto Readback=[&](const TCHAR* Key)
        {
            auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("visible"),V39LastStatus.Visible);J->SetBoolField(TEXT("tank"),V39LastStatus.Tank);
            J->SetNumberField(TEXT("team"),V39LastStatus.Team);J->SetNumberField(TEXT("current"),V39LastStatus.Current);J->SetNumberField(TEXT("maximum"),V39LastStatus.Maximum);J->SetNumberField(TEXT("fraction"),V39LastStatus.Fraction);
            R->Report->SetObjectField(Key,J);
        };
        if(Now-R->Start>85){Check(TEXT("bounded_review_timeout"),false);R->Stage=99;}
        double Delay=.65;
        switch(R->Stage)
        {
        case 0:
        {
            Check(TEXT("isolated_volatile_profile"),B->IsProgressionVolatile());if(!R->Passed){R->Stage=99;break;}
            const auto Coverage=FrontlineLockV39::RunCoverageChecks(B->GetWorld());R->Report->SetObjectField(TEXT("lock_coverage"),Coverage);Check(TEXT("actual_native_lock_SDR_and_RGBA16F_coverage"),Coverage->GetBoolField(TEXT("success")));
            FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);
            if(Render){R->HDRSupported=Render->GetHDRStatus().bSupported;Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            Select(EFrontlineFaction::British,2);Deploy();Delay=1;break;
        }
        case 1:Shot(TEXT("01-british-deployment-locks-and-available"));break;
        case 2:Begin();if(Render&&R->HDRSupported)Check(TEXT("HDR_enable_request_accepted"),Render->SetHDREnabled(true,false));Delay=2;break;
        case 3:
            if(Render){const auto H=Render->GetHDRStatus();R->Report->SetBoolField(TEXT("HDR_supported"),R->HDRSupported);R->Report->SetBoolField(TEXT("HDR_output_active"),H.bOutputActive);Check(TEXT("supported_HDR_output_is_active_for_lock_capture"),!R->HDRSupported||H.bEnabled&&H.bOutputActive);}
            Shot(R->HDRSupported?TEXT("02-british-HDR-HUD-square-support-locks"):TEXT("02-british-SDR-HUD-HDR-unavailable"));break;
        case 4:if(Render)Render->SetHDREnabled(false,false);Select(EFrontlineFaction::German,2);Deploy();break;
        case 5:Shot(TEXT("03-german-deployment-locks-and-available"));break;
        case 6:Begin();Delay=1;break;
        case 7:Shot(TEXT("04-german-SDR-HUD-square-support-locks"));break;
        case 8:
        {
            // Keep Germany as the player: team zero must remain own army when
            // the campaign starts from the right side of the battlefield.
            Select(EFrontlineFaction::German,4);Begin();
            for(int32 Team=0;Team<2;++Team)FFrontlineReviewV32::Spawn(B,EFrontlineRole::Rifle,Team,B->GetHomeTrenchIndex(Team));
            for(auto& Squad:B->Squads)
            {
                const float X=Squad.Team?750.f:-750.f;int32 Index=0;
                for(const auto& Ref:Squad.Members)if(auto* U=Ref.Get())
                {
                    const FVector Feet(X+(Index%3)*70,-20+(Index/3)*130,0);
                    U->SetActorLocation(FVector(Feet.X,Feet.Y,AWarfareFrontlineGround::HeightAt(Feet.X,Feet.Y)+88));
                    U->bFieldEntered=true;U->SetActorHiddenInGame(false);U->SetActorTickEnabled(false);U->ReturnToIdle();
                    if(Index==0){if(Squad.Team)R->EnemyMan=U;else R->OwnMan=U;}
                    if(Squad.Team==0&&Index>=3)U->ReceiveHit(1.f,0,1);
                    ++Index;
                }
                if(Squad.Team==0)
                {
                    R->OwnSquad=Squad.Id;R->Original=Squad.Members.Num();R->Remaining=Squad.LivingCount();
                    const auto Initial=SquadStrengthV39(Squad);
                    if(R->OwnMan.IsValid())R->OwnMan->bFieldEntered=false;
                    const auto Entering=SquadStrengthV39(Squad);if(R->OwnMan.IsValid())R->OwnMan->bFieldEntered=true;
                    Check(TEXT("living_count_does_not_remove_alive_reinforcements"),Entering.Remaining==Initial.Remaining&&Entering.Original==Initial.Original);
                    for(const auto& Ref:Squad.Members)if(auto* U=Ref.Get()){U->Suppression=1.f;U->bRecoveredMachineGun=true;}
                    auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());const uint32 Mask=Run?Run->Research.TechnologyMask:0;
                    if(Run)Run->Research.TechnologyMask=0xffffffffu;
                    const auto Pressured=SquadStrengthV39(Squad);if(Run)Run->Research.TechnologyMask=Mask;
                    Check(TEXT("infantry_count_ignores_suppression_takeover_and_later_research"),Initial.Remaining==Pressured.Remaining&&Initial.Original==Pressured.Original&&Initial.Fraction==Pressured.Fraction);
                    for(const auto& Ref:Squad.Members)if(auto* U=Ref.Get())if(!U->IsAlive()){U->Destroy();break;}
                    const auto Expired=SquadStrengthV39(Squad);
                    Check(TEXT("dead_and_expired_roster_entries_keep_original_complement"),Expired.Original==R->Original&&Expired.Remaining==R->Remaining&&Expired.Remaining==3&&Expired.Original>3);
                    FFrontlineSquad Empty;Check(TEXT("empty_roster_has_zero_finite_strength"),SquadStrengthV39(Empty).Fraction==0);
                }
            }
            if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(int32 Team=0;Team<2;++Team)
            {
                Vehicles->ResetReadinessForDebug(Team);const auto Roster=Vehicles->GetRoster(Team);const int32 Prior=Vehicles->GetVehicles().Num();
                Check(TEXT("actual_tank_recruited_")+FString::FromInt(Team),Roster.Num()>0&&Vehicles->Recruit(Roster[0].Type,Team));
                if(Vehicles->GetVehicles().Num()>Prior)
                {
                    AFrontlineTank* T=Vehicles->GetVehicles().Last();T->SetActorLocation(FVector(Team?1650:-1650,-600,0));T->Simulate(.001f);
                    if(Team)R->EnemyTank=T;else{R->OwnTank=T;T->ReceiveModuleHit(EFrontlineTankModule::Hull,83.f);}
                }
            }
            Check(TEXT("own_enemy_hover_fixture_assets_ready"),R->OwnMan.IsValid()&&R->EnemyMan.IsValid()&&R->OwnTank.IsValid()&&R->EnemyTank.IsValid()&&R->OwnTank->HasEnteredField()&&R->EnemyTank->HasEnteredField());
            FFrontlineReviewV32::Camera(PC,0,0,5900,180);Delay=1;break;
        }
        case 9:UGameplayStatics::SetGamePaused(PC,true);Check(TEXT("own_infantry_pointer_in_playfield"),R->OwnMan.IsValid()&&PointAt(R->OwnMan->Feet()+FVector(0,0,88)));break;
        case 10:
            Readback(TEXT("own_infantry"));Check(TEXT("actual_draw_own_German_infantry_uses_remaining_men"),V39LastStatus.Visible&&!V39LastStatus.Tank&&V39LastStatus.Team==0&&V39LastStatus.SquadId==R->OwnSquad&&V39LastStatus.Current==R->Remaining&&V39LastStatus.Maximum==R->Original&&FMath::IsNearlyEqual(V39LastStatus.Fraction,float(R->Remaining)/FMath::Max(1,R->Original)));
            Shot(TEXT("05-own-infantry-current-living-count"));break;
        case 11:Check(TEXT("own_tank_pointer_in_playfield"),R->OwnTank.IsValid()&&PointAt(R->OwnTank->AimPoint()));break;
        case 12:
            Readback(TEXT("own_tank"));Check(TEXT("actual_draw_own_tank_reads_real_hull_and_spawned_maximum"),R->OwnTank.IsValid()&&V39LastStatus.Visible&&V39LastStatus.Tank&&V39LastStatus.Team==0&&FMath::IsNearlyEqual(V39LastStatus.Current,R->OwnTank->GetHullIntegrity())&&FMath::IsNearlyEqual(V39LastStatus.Maximum,R->OwnTank->GetMaximumHullIntegrity())&&V39LastStatus.Current<V39LastStatus.Maximum);
            Shot(TEXT("06-own-tank-actual-armor"));break;
        case 13:Check(TEXT("enemy_infantry_pointer_in_playfield"),R->EnemyMan.IsValid()&&PointAt(R->EnemyMan->Feet()+FVector(0,0,88)));break;
        case 14:Readback(TEXT("enemy_infantry"));Check(TEXT("actual_draw_enemy_infantry_has_no_status_panel"),!V39LastStatus.Visible);Shot(TEXT("07-enemy-infantry-no-status-panel"));break;
        case 15:Check(TEXT("enemy_tank_pointer_in_playfield"),R->EnemyTank.IsValid()&&PointAt(R->EnemyTank->AimPoint()));break;
        case 16:Readback(TEXT("enemy_tank"));Check(TEXT("actual_draw_enemy_tank_has_no_status_panel"),!V39LastStatus.Visible);Shot(TEXT("08-enemy-tank-no-status-panel"));break;
        case 17:V28ReviewPointer=FVector2D(-900,-900);break;
        default:
        {
            Check(TEXT("outside_viewport_clears_status_readback"),!V39LastStatus.Visible);Check(TEXT("eight_focused_screens_requested"),R->Shots==8);
            TArray<TSharedPtr<FJsonValue>> Typography;int32 Bad=0,Truncated=0;
            for(const auto& E:V36TypographyAudit){Typography.Add(MakeShared<FJsonValueObject>(E.Value));Bad+=!E.Value->GetBoolField(TEXT("fits"));Truncated+=E.Value->GetBoolField(TEXT("truncated"));}
            Check(TEXT("affected_visible_text_fits"),Bad==0&&Typography.Num()>20);Check(TEXT("no_paragraph_ellipsis"),Truncated==0);
            Check(TEXT("visible_localization_coverage"),FrontlineLocalizationV35::WriteCoverageReport());
            R->Report->SetArrayField(TEXT("typography"),Typography);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetBoolField(TEXT("success"),R->Passed);
            R->Report->SetNumberField(TEXT("screenshots_requested"),R->Shots);R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetStringField(TEXT("scope"),TEXT("Eight actual-renderer views: national deployment and HUD locks, ready/cooling square support frames, one HDR path when supported, own infantry and tank status, and absence of enemy status. Disposable real fixtures; no former FX/motion suite, player save mutation or performance claim. HDR unavailable is recorded rather than represented as tested."));
            FString Text;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Directory/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            V28ReviewPointer.Reset();V27ImageAuditActive=V36TypographyAuditActive=false;FPlatformMisc::RequestExit(false);return false;
        }
        }
        ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
    }));
}
}
