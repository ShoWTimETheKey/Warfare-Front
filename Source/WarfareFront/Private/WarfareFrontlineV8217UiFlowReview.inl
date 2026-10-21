// Narrow UI checks and capture fixtures. No entry point exists on a normal launch.
namespace FrontlineUiFlowV8217
{
bool Allowed(AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,AWarfareFrontlineHUD* HUD=nullptr)
{
    FString Dir;
    return B&&PC&&FrontlineFor(PC)==B&&(!HUD||HUD==PC->GetHUD())&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8217Review"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir);
}
void Run(AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,AWarfareFrontlineHUD* HUD,
    TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=Allowed(B,PC,HUD);Check(TEXT("v8217_ui_flow_exact_disposable_scope"),Safe);if(!Safe)return;
    FrontlineAwardMotionV8217::RunChecks(Check);
    const auto Language=FrontlineLocalizationV35::GetLanguage();const float Ready=B->ArtilleryReadyAt[0];
    const auto Encounter=B->GetEncounterId();const int32 Roster=B->Units.Num(),Squads=B->Squads.Num();
    const auto Campaign=B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();
    ON_SCOPE_EXIT {B->ArtilleryReadyAt[0]=Ready;FrontlineLocalizationV35::SetLanguage(Language);};
    B->ArtilleryReadyAt[0]=B->Elapsed+20.f;
    const auto Arrival=B->FireSupportDelayRange(EFrontlineSupport::Artillery,FVector::ZeroVector);
    for(const auto L:{FrontlineLocalizationV35::ELanguage::English,FrontlineLocalizationV35::ELanguage::Chinese})
    {
        FrontlineLocalizationV35::SetLanguage(L);const bool ZH=L==FrontlineLocalizationV35::ELanguage::Chinese;
        const FString Prefix=ZH?TEXT("v8217_ui_zh_"):TEXT("v8217_ui_en_");
        Check(Prefix+TEXT("loading_uses_full_army_names"),FrontlineLoadingPageV82::ArmyNameV8217(EFRRUNationV81::British)==(ZH?TEXT("英国远征军"):TEXT("BRITISH EXPEDITIONARY FORCE"))
            &&FrontlineLoadingPageV82::ArmyNameV8217(EFRRUNationV81::German)==(ZH?TEXT("德意志帝国陆军"):TEXT("IMPERIAL GERMAN ARMY")));
        const FString Delay=FrontlineStatusTextV8217::SupportArrival(Arrival);
        Check(Prefix+TEXT("arrival_range_remains_available_during_actual_support_cooldown"),B->SupportCooldown(EFrontlineSupport::Artillery)>19.f
            &&!Delay.IsEmpty()&&Arrival.X>0&&Arrival.Y>=Arrival.X
            &&Delay.Contains(FString::Printf(TEXT("%.0f"),Arrival.X))&&Delay.Contains(FString::Printf(TEXT("%.0f"),Arrival.Y)));
    }
    // Hidden, unregistered squad members exercise the same alive/member-index
    // input as the live tooltip. No casualty or reward event is manufactured.
    TArray<AWarfareFrontlineSoldier*> Members;
    FFrontlineSquad Section;Section.Team=B->GetPlayerFaction()==EFrontlineFaction::British?0:1;Section.Role=EFrontlineRole::Marksman;
    for(int32 I=0;I<2;++I)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(FVector(0,0,-10000),FRotator::ZeroRotator);
        if(!U)continue;U->SetActorTickEnabled(false);U->SetActorHiddenInGame(true);U->Battle=B;U->Team=Section.Team;U->Role=Section.Role;U->MemberIndex=I;
        Members.Add(U);Section.Members.Add(U);
    }
    const auto PrimaryAlive=[&](){for(const auto& W:Section.Members)if(const auto* U=W.Get();U&&U->IsActiveCombatant()&&U->MemberIndex==0)return true;return false;};
    if(Members.Num()==2)
    {
        FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);
        const FString Before=FrontlineStatusTextV8217::Shooter(B,Section,PrimaryAlive());Members[0]->Health=0;
        const FString After=FrontlineStatusTextV8217::Shooter(B,Section,PrimaryAlive());
        Check(TEXT("v8217_sniper_tooltip_changes_with_actual_primary_member_survival"),Before==TEXT("Primary shooter operational")&&After==TEXT("Primary shooter killed · observer remains")
            &&Section.LivingCount()==1&&Members[1]->IsActiveCombatant());
        FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);
        Check(TEXT("v8217_sniper_primary_loss_has_chinese_status"),FrontlineStatusTextV8217::Shooter(B,Section,PrimaryAlive())==TEXT("主射手阵亡 · 仅剩观察手"));
        Section.Role=EFrontlineRole::Rifle;
        Check(TEXT("v8217_primary_shooter_note_does_not_leak_into_regular_rifle_tooltip"),FrontlineStatusTextV8217::Shooter(B,Section,false).IsEmpty());
    }
    else Check(TEXT("v8217_sniper_actual_member_fixture_available"),false);
    for(auto* U:Members)U->Destroy();
    Check(TEXT("v8217_ui_checks_preserve_campaign_encounter_and_live_roster"),Encounter==B->GetEncounterId()&&B->Units.Num()==Roster&&B->Squads.Num()==Squads
        &&(!B->GetCurrentCampaign()? !Campaign.IsValid():B->GetCurrentCampaign()->Id==Campaign));
}
// This replaces only the disposable screenshot campaign, after world checks.
bool ReviewReportFixture(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B,EFrontlineFaction Faction)
{
    if(!Allowed(B,PC))return false;B->AbandonBattleToOperations();
    auto& S=ScreenState(PC);S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=false;
    S.Dossier=1917;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.PreviousPageV35=6;
    return FrontlineAchievementsV76::SeedNewUnlockReviewV8215(B,Faction,0,false)&&HasEnded(B)&&!FrontlineNewUnlockV8215::Pending(B);
}
bool ReviewBannerFixture(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B,bool Medal,double Age)
{
    if(!Allowed(B,PC)||Age<0||Age>=6.5)return false;auto& S=ScreenState(PC);
    UIMotionReviewV83::BannerAge=-1;
    if(Medal)
    {
        if(S.MedalToastV8217==INDEX_NONE)
        {
            S.AchievementToastV81=INDEX_NONE;
            if(!FrontlineAchievementsV76::SeedMedalNotificationReviewV8217(B,B->GetPlayerFaction(),false)
                ||!BeginNextAwardBannerV8217(S,FPlatformTime::Seconds())||S.MedalToastV8217==INDEX_NONE)return false;
        }
    }
    else {S.MedalToastV8217=INDEX_NONE;S.AchievementToastV81=3;}
    // Capture setup backdates the start; the production draw still advances on
    // real time. Achievement index 3 is a layout fixture, medal is a saved event.
    S.AchievementToastAtV81=FPlatformTime::Seconds()-Age;return true;
}
double BannerAge(AWarfareFrontlineController* PC)
{return PC?FPlatformTime::Seconds()-ScreenState(PC).AchievementToastAtV81:-1.;}
}
void RunFrontlineUiFlowChecksV8217(AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,AWarfareFrontlineHUD* HUD,
    TFunctionRef<void(const FString&,bool)> Check)
{FrontlineUiFlowV8217::Run(B,PC,HUD,Check);}
