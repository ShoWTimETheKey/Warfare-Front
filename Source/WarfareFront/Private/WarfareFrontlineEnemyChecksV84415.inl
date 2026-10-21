#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "HAL/PlatformTime.h"

struct FFrontlineEnemyChecksV84415
{
    static void RunScoped(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        FString UserDir;
        const bool Isolated=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV84415Focused"))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&FrontlineUltimateArchiveV8215::ReviewUserDir(UserDir);
        Check(TEXT("v84415_enemy_review_requires_explicit_disposable_profile"),Isolated);if(!Isolated)return;
        Check(TEXT("v84415_enemy_review_starts_without_live_company"),B->Units.IsEmpty()&&B->Squads.IsEmpty());if(!B->Units.IsEmpty()||!B->Squads.IsEmpty())return;
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());Check(TEXT("v84415_enemy_review_has_actual_vehicle_subsystem"),Fleet!=nullptr);if(!Fleet)return;
        const auto Runs=B->CampaignRuns;const auto Active=B->ActiveCampaignId;const auto Faction=B->PlayerFaction;
        const auto Phase=B->Phase;const bool Ticking=B->IsActorTickEnabled();B->SetActorTickEnabled(false);
        ON_SCOPE_EXIT
        {
            FrontlineUltimateArchiveV8215::Leave(B);B->AbandonBattleToOperations();B->CampaignRuns=Runs;B->ActiveCampaignId=Active;B->PlayerFaction=Faction;
            B->ResetEncounter();B->Phase=Phase;B->SetActorTickEnabled(Ticking);
        };
        auto Select=[&](EFrontlineFaction F,int32 Operation,EFrontlineDifficulty D)
        {
            B->AbandonBattleToOperations();FFrontlineCampaignProgress Run;Run.Id=FGuid::NewGuid();Run.Faction=F;Run.Difficulty=D;
            Run.SelectedMission=Run.CompletedMissions=Operation;Run.Research.TechnologyMask=0;Run.ArmourResearchMaskV63=0;
            B->CampaignRuns={Run};B->ActiveCampaignId=Run.Id;B->PlayerFaction=F;B->ResetEncounter();B->BindCampaignEncounter();
            B->Phase=EFrontlinePhase::Fighting;B->Elapsed=1000;
            B->EnemyRecruitAt=0;B->EnemyAssaultAt=B->EnemyFireAssessAt=MAX_flt;B->EnemyCounterattackTrench=INDEX_NONE;
            B->bEnemyRoleCommitted=B->bEnemyFirePending=false;B->EnemyWaveRecruitCount=0;
            const auto* M=B->GetEncounterCampaignMission();B->EnemyRecruitStep=M?M->EnemyRoleOrder.Num()*10:40;
            B->ExtendedSupportThinkAt=MAX_flt;
            for(int32 Team=0;Team<2;++Team)
            {
                B->ArtilleryReadyAt[Team]=B->MortarReadyAt[Team]=B->SmokeReadyAt[Team]=MAX_flt;
                for(int32 Id=0;Id<3;++Id)B->ExtendedSupportReadyAt[Team][Id]=MAX_flt;
                for(int32 R=0;R<B->RoleCount;++R)B->RecruitReadyAt[Team][R]=0;
            }
            FrontlineDifficultyV30::Reset(B);FirePlansV84415.Remove(B);
        };
        auto Make=[&](int32 Team,EFrontlineRole Role,int32 Id,const FVector& Point,int32 Member=0)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->Suppression=0;
            U->ShotClock=U->GrenadeClock=MAX_flt;U->SetActorLocation(Point+FVector(0,0,88));B->Units.Add(U);
            FFrontlineSquad S;S.Team=Team;S.Role=Role;S.Id=Id;S.Members.Add(U);B->Squads.Add(S);return U;
        };
        TArray<TSharedPtr<FJsonValue>> Coverage,Actions;
        for(int32 Archive=0;Archive<2;++Archive)for(int32 F=0;F<2;++F)
        {
            const auto Player=EFrontlineFaction(F),Enemy=EFrontlineFaction(1-F);
            Select(Player,0,EFrontlineDifficulty::Rookie);B->Phase=EFrontlinePhase::Briefing;
            const bool SessionReady=!Archive||FrontlineUltimateArchiveV8215::Begin(B,Player,0,EFrontlineDifficulty::Rookie);
            Check(FString::Printf(TEXT("v84415_catalogue_f%d_archive%d_session_ready"),F,Archive),SessionReady);if(!SessionReady)return;
            for(int32 Operation=0;Operation<20;++Operation)for(int32 D=0;D<3;++D)
            {
                const auto Difficulty=EFrontlineDifficulty(D);auto* Run=B->CampaignRuns.FindByPredicate([&](const auto& R){return R.Id==B->ActiveCampaignId;});
                if(!Run){Check(TEXT("v84415_catalogue_real_company_present"),false);return;}
                Run->SelectedMission=Operation;Run->CompletedMissions=Archive?20:Operation;Run->Difficulty=Difficulty;
                Run->WonOperationMaskV52=(1u<<Operation)-1u;B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;
                const auto& Missions=B->GetCampaignMissionsForFaction(Player);if(!Missions.IsValidIndex(Operation)){Check(TEXT("v84415_catalogue_twenty_operation_routes"),false);return;}
                const auto& M=Missions[Operation];const auto& Nodes=B->GetTechnologyNodesForFaction(Enemy);
                const uint32 Research=Archive?(1u<<int32(EFrontlineTechnology::Count))-1u:FrontlineEnemyPolicyV84415::ResearchMask(Enemy,Operation,Difficulty);
                uint32 Roles=0,Supports=0,Vehicles=0;bool Legal=true,Prerequisites=true,Dates=true;int32 Points=0,TechCount=0;
                for(int32 R=0;R<B->RoleCount;++R)
                {
                    const bool Available=FrontlineGameplayV33::EnemyRoleAvailable(B,EFrontlineRole(R));
                    const bool Expected=FrontlineCampaignV41::RoleAllowed(Enemy,EFrontlineRole(R))
                        &&(Archive||(Operation>=B->RoleUnlockMission(EFrontlineRole(R),1)&&M.EnemyRoleOrder.Contains(EFrontlineRole(R))));
                    Legal&=Available==Expected;if(Available)Roles|=1u<<R;
                }
                for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)if(B->IsSupportUnlocked(EFrontlineSupport(I),1))Supports|=1u<<I;
                for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)if(Fleet->IsUnlocked(EFrontlineVehicle(I),1))Vehicles|=1u<<I;
                for(const auto& N:Nodes)
                {
                    Legal&=B->HasTechnology(N.Id,1)==((Research&(1u<<N.Id))!=0);
                    if(!(Research&(1u<<N.Id)))continue;
                    ++TechCount;Points+=N.Cost;
                    Prerequisites&=(N.Prerequisite<0||(Research&(1u<<N.Prerequisite)))&&(N.SecondaryPrerequisite<0||(Research&(1u<<N.SecondaryPrerequisite)));
                    if(!Archive)Legal&=Operation>=FrontlineEnemyPolicyV84415::EarliestResearch(Enemy,N.Id);
                    if(!Archive&&Enemy==EFrontlineFaction::German&&(N.Id==12||N.Id==14||N.Id==15))Dates&=M.CalendarDateV41>=19180321;
                }
                const int32 Budget=D==0?0:2+Operation*(D==2?3:2);if(!Archive)Legal&=Points<=Budget;
                if(!Archive&&Operation>0){const uint32 Previous=FrontlineEnemyPolicyV84415::ResearchMask(Enemy,Operation-1,Difficulty);Legal&=(Research&Previous)==Previous;}
                for(const auto R:M.EnemyOpeningRoles)Legal&=(Roles&(1u<<int32(R)))!=0;
                FrontlineEnemyPolicyV84415::FRecruitSituation Input;Input.Faction=Enemy;Input.Difficulty=Difficulty;Input.AvailableRoles=Roles;Input.Archive=Archive!=0;
                Input.OwnLiving=12;Input.SeenRifles=18;const auto Counter=FrontlineEnemyPolicyV84415::CounterRole(Input);
                Legal&=Counter==EFrontlineRole::Count||(Roles&(1u<<int32(Counter)))!=0;
                const FString Key=FString::Printf(TEXT("v84415_%s_op%02d_d%d_%s"),F?TEXT("de"):TEXT("uk"),Operation+1,D,Archive?TEXT("archive"):TEXT("campaign"));
                Check(Key+TEXT("_legal_roles_research_prerequisites_and_dates"),Legal&&Prerequisites&&Dates);
                auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("case"),Key);Row->SetNumberField(TEXT("researchMask"),Research);
                Row->SetNumberField(TEXT("researchCount"),TechCount);Row->SetNumberField(TEXT("researchCost"),Points);Row->SetNumberField(TEXT("researchBudget"),Archive?-1:Budget);
                Row->SetNumberField(TEXT("availableRoles"),Roles);Row->SetNumberField(TEXT("actualAvailableSupports"),Supports);Row->SetNumberField(TEXT("actualAvailableArmour"),Vehicles);
                Row->SetNumberField(TEXT("date"),M.CalendarDateV41);Row->SetNumberField(TEXT("normalDisabledSupportMask"),M.DisabledSupportMask);
                                const auto& Doctrine=FrontlineDifficultyV30::Doctrine(Difficulty);
                Row->SetNumberField(TEXT("enemyRifleReadinessSeconds"),B->EffectiveRecruitSeconds(EFrontlineRole::Rifle,1)*M.EnemyRecruitScale*Doctrine.RecruitScale);
                Row->SetNumberField(TEXT("enemyGunReadinessSeconds"),B->EffectiveRecruitSeconds(EFrontlineRole::Lewis,1)*M.EnemyRecruitScale*Doctrine.RecruitScale);
                Row->SetNumberField(TEXT("enemyATRReadinessSeconds"),B->EffectiveRecruitSeconds(Enemy==EFrontlineFaction::German?EFrontlineRole::AntiTankRifle:EFrontlineRole::RifleGrenadier,1)*M.EnemyRecruitScale*Doctrine.RecruitScale);
                Row->SetNumberField(TEXT("missionRecruitScale"),M.EnemyRecruitScale);Row->SetNumberField(TEXT("visibility"),M.VisibilityScale);Row->SetNumberField(TEXT("authoredDepth"),M.DepthScaleV41);
                Row->SetNumberField(TEXT("observedAssessmentMin"),Doctrine.AssessMin);Row->SetNumberField(TEXT("observedAssessmentMax"),Doctrine.AssessMax);
                Row->SetNumberField(TEXT("reactionMin"),Doctrine.ReactionMin);Row->SetNumberField(TEXT("reactionMax"),Doctrine.ReactionMax);
                Row->SetNumberField(TEXT("waveRestThreatScale"),FrontlineEnemyPolicyV84415::WaveRestScale(Difficulty,Operation,true));
                Row->SetBoolField(TEXT("archiveFullArmory"),Archive!=0);Row->SetBoolField(TEXT("success"),Legal&&Prerequisites&&Dates);Coverage.Add(MakeShared<FJsonValueObject>(Row));
            }
            B->Phase=EFrontlinePhase::Briefing;if(Archive)FrontlineUltimateArchiveV8215::Leave(B);
        }
        Check(TEXT("v84415_all_240_catalogue_cases_covered"),Coverage.Num()==240);
        Check(TEXT("v84415_empty_unguarded_forward_bay_releases_final_section"),!FrontlineEnemyPolicyV84415::RetainAnchor(EFrontlineDifficulty::Survivor,6,6,0,0,false,false));
        Check(TEXT("v84415_observed_defenders_preserve_covering_reserve"),FrontlineEnemyPolicyV84415::RetainAnchor(EFrontlineDifficulty::Survivor,9,3,4,4,true,false));
        Check(TEXT("v84415_rear_gun_relief_does_not_keep_obsolete_anchor"),!FrontlineEnemyPolicyV84415::RetainAnchor(EFrontlineDifficulty::Veteran,9,3,0,0,false,true));
        Check(TEXT("v84415_introductory_wave_rest_is_unchanged"),FrontlineEnemyPolicyV84415::WaveRestScale(EFrontlineDifficulty::Survivor,2,true)==1.f
            &&FrontlineEnemyPolicyV84415::WaveRestScale(EFrontlineDifficulty::Rookie,19,true)==1.f);
        Check(TEXT("v84415_high_difficulty_reinforces_observed_pressure_without_firearm_multiplier"),FrontlineEnemyPolicyV84415::WaveRestScale(EFrontlineDifficulty::Survivor,14,true)
            <FrontlineEnemyPolicyV84415::WaveRestScale(EFrontlineDifficulty::Veteran,14,true));
        int32 Serial=8441500;
        for(int32 F=0;F<2;++F)for(int32 D=1;D<3;++D)for(int32 Case=0;Case<4;++Case)
        {
            Select(EFrontlineFaction(F),14,EFrontlineDifficulty(D));const int32 Direction=B->GetTeamDirection(1);const FVector Observer(0,0,10000);
            auto* Gun=Make(1,EFrontlineRole::Lewis,Serial++,Observer);AActor* Wall=nullptr;
            bool Set=true;int32 Seen=0;
            if(Case==0)
            {
                for(int32 N=0;N<18;++N)Set&=Make(0,EFrontlineRole::Rifle,Serial++,Observer+FVector(Direction*2200,(N-9)*45,0))!=nullptr;
            }
            else if(Case==1||Case==3)
            {
                Set&=Make(0,EFrontlineRole::Marksman,Serial++,Observer+FVector(Direction*2000,-60,0))!=nullptr;
                Set&=Make(0,EFrontlineRole::Marksman,Serial++,Observer+FVector(Direction*2000,60,0),1)!=nullptr;
                if(Case==3)
                {
                    Wall=B->GetWorld()->SpawnActor<AActor>();if(Wall)
                    {
                        auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Wall->AddInstanceComponent(Box);
                        Box->SetBoxExtent(FVector(35,700,400));Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);
                        Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Box->RegisterComponent();Wall->SetActorLocation(Observer+FVector(Direction*1000,0,150));
                    }
                    else Set=false;
                }
            }
            else
            {
                Fleet->ResetReadinessForDebug(0);auto* Tank=Fleet->Spawn(F==0?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V,0,false);
                if(Tank){Tank->SetActorLocation(Observer+FVector(Direction*2200,0,0));Set&=Tank->HasEnteredField()&&ObserveArmourV84415(B,Tank);}else Set=false;
            }
            for(const AWarfareFrontlineSoldier* U:B->Units)if(FrontlineDifficultyV30::CanObserve(B,U))++Seen;
            const int32 Before=B->Squads.Num();B->ThinkEnemy();
            const EFrontlineRole Wanted=Case==0?EFrontlineRole::Lewis:Case==1?FrontlineChoiceTargetsV78::PrecisionCounter(B->GetFactionForTeam(1)):
                Case==2?(F==0?EFrontlineRole::AntiTankRifle:EFrontlineRole::RifleGrenadier):EFrontlineRole::Rifle;
            const bool Accepted=Set&&Gun&&B->Squads.Num()==Before+1&&B->Squads.Last().Team==1&&B->Squads.Last().Role==Wanted&&B->RecruitCooldown(Wanted,1)>0;
            const auto Name=FString::Printf(TEXT("v84415_actual_observed_recruit_f%d_d%d_case%d"),F,D,Case);Check(Name,Accepted);
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("case"),Name);Row->SetNumberField(TEXT("observedInfantry"),Seen);
            Row->SetNumberField(TEXT("recruitedRole"),B->Squads.Num()>Before?int32(B->Squads.Last().Role):-1);Row->SetBoolField(TEXT("success"),Accepted);Actions.Add(MakeShared<FJsonValueObject>(Row));
            if(Wall){Wall->SetActorEnableCollision(false);Wall->Destroy();}
        }
        for(int32 F=0;F<2;++F)for(int32 D=1;D<3;++D)
        {
            Select(EFrontlineFaction(F),14,EFrontlineDifficulty(D));B->EnemyRecruitAt=MAX_flt;
            const int32 Dir=B->GetTeamDirection(1);const FVector Observer(0,0,10000);
            bool Ready=Make(1,EFrontlineRole::Rifle,Serial++,Observer)!=nullptr;
            auto* Gun=Make(0,EFrontlineRole::Lewis,Serial++,Observer+FVector(Dir*3500,0,0));if(Gun)Gun->ShotsDischarged=1;else Ready=false;
            for(int32 N=0;N<3;++N)Ready&=Make(0,EFrontlineRole::Rifle,Serial++,Observer+FVector(Dir*3500,75.f*(N+1),0))!=nullptr;
            B->MortarReadyAt[1]=0;B->EnemyFireAssessAt=0;FirePlansV84415.Remove(B);
            const int32 Before=B->Strikes.Num();B->ThinkEnemy();const auto Planned=FirePlanV84415(B);
            const bool Delayed=Ready&&Planned.Pending&&Planned.Type==EFrontlineSupport::Mortar&&Planned.ExecuteAt>B->Elapsed&&B->Strikes.Num()==Before;
                        B->EnemyThinkAt=MAX_flt;B->ExtendedSupportThinkAt=0;B->Tick(.05f);
            int32 WaitingRounds=0;for(const auto& Strike:B->Strikes)WaitingRounds+=Strike.Team==1;
            Check(FString::Printf(TEXT("v84415_ordinary_tick_does_not_spend_pending_fire_f%d_d%d"),F,D),Delayed&&WaitingRounds==0&&FirePlanV84415(B).Pending);
            B->ArtilleryReadyAt[0]=0;const bool PlayerCalled=B->RequestArtillery(FVector(-Dir*FrontlineBattlefield::GoalHalfLength*.85f,0,0),0);
            B->Elapsed=Planned.ExecuteAt+.01f;B->ThinkEnemy();int32 EnemyRounds=0;for(const auto& Strike:B->Strikes)EnemyRounds+=Strike.Team==1;
            const bool Fired=Delayed&&EnemyRounds>0&&FrontlineDifficultyV30::State(B).PlannedFireMissions==1&&B->MortarReadyAt[1]>B->Elapsed;
            Check(FString::Printf(TEXT("v84415_actual_observation_transmission_mortar_f%d_d%d"),F,D),Fired);
            Check(FString::Printf(TEXT("v84415_committed_point_survives_actual_player_artillery_call_f%d_d%d"),F,D),PlayerCalled
                &&FirePlanV84415(B).Point.Equals(Planned.Point,.001f)&&!FirePlanV84415(B).Pending);
        }
        for(int32 F=0;F<2;++F)
        {
            Select(EFrontlineFaction(F),14,EFrontlineDifficulty::Survivor);B->EnemyRecruitAt=MAX_flt;B->EnemyAssaultAt=0;
            const int32 Home=B->GetHomeTrenchIndex(1);const bool Spawned=B->SpawnSquad(1,EFrontlineRole::Rifle,Home);
            const int32 Id=Spawned?B->Squads.Last().Id:INDEX_NONE;
            if(Spawned)for(const auto& W:B->Squads.Last().Members)if(auto* U=W.Get())
            {U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->SetActorLocation(B->SoldierDestination(U)+FVector(0,0,88));}
            B->ThinkEnemy();const auto* Section=B->FindSquad(Id);
            Check(FString::Printf(TEXT("v84415_actual_final_full_section_advances_toward_empty_bay_f%d"),F),Spawned&&Section&&Section->bAdvancing&&Section->Trench==INDEX_NONE);
        }
        // Same prepared ordinary terrain, twelve bounded real combat samples.
        // These are rifle-advance pressure probes, never estimated win rates.
        TArray<TSharedPtr<FJsonValue>> Pressure;const double ProbeStart=FPlatformTime::Seconds();
        for(int32 F=0;F<2;++F)for(int32 D=0;D<3;++D)for(int32 Archive=0;Archive<2;++Archive)
        {
            Select(EFrontlineFaction(F),14,EFrontlineDifficulty(D));B->Phase=EFrontlinePhase::Briefing;
            if(Archive&&!FrontlineUltimateArchiveV8215::Begin(B,EFrontlineFaction(F),14,EFrontlineDifficulty(D)))
            {Check(TEXT("v84415_pressure_archive_session_can_open"),false);continue;}
            auto* R=B->CampaignRuns.FindByPredicate([&](const auto& Run){return Run.Id==B->ActiveCampaignId;});
            if(R&&!Archive){R->Research.TechnologyMask=FrontlineEnemyPolicyV84415::ResearchMask(EFrontlineFaction(F),14,EFrontlineDifficulty::Survivor);R->RoleSelectionMask=1u;}
            B->ResetEncounter();B->EnemyDecisionRandom.Initialize(8441500+F*101+D*17);B->Random.Initialize(8441500+F*101);
            B->BeginBattle();B->SetActorTickEnabled(false);
            const bool Started=B->Phase==EFrontlinePhase::Fighting&&B->AssetFailure.IsEmpty();
            Check(FString::Printf(TEXT("v84415_pressure_actual_battle_f%d_d%d_a%d"),F,D,Archive),Started);
            const double CaseStart=FPlatformTime::Seconds();float OrdersAt=0;int32 PlayerRecruits=0,EnemyRecruits=0,SeenEnemyArmour=0;
            TSet<int32> CountedSquads;TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,int32> Shots;int32 ActualShots[2]={};
            while(Started&&B->Elapsed<120.f&&B->Phase==EFrontlinePhase::Fighting&&B->BattleOutroWinner==INDEX_NONE
                &&FPlatformTime::Seconds()-CaseStart<3.0&&FPlatformTime::Seconds()-ProbeStart<50.0)
            {
                if(B->Elapsed>=OrdersAt)
                {
                    OrdersAt=B->Elapsed+1.f;if(B->RecruitBlockReason(EFrontlineRole::Rifle,0).IsEmpty())B->Recruit(EFrontlineRole::Rifle,0);
                    TArray<int32> Orders;for(const auto& S:B->Squads)if(S.Team==0&&S.LivingCount()>0&&S.Trench>=0)Orders.Add(S.Id);
                    for(int32 Id:Orders)if(const auto* Section=B->FindSquad(Id))B->AdvanceSquad(Section->Trench,Id,0);
                }
                B->Tick(.1f);
                const auto Units=B->Units;for(AWarfareFrontlineSoldier* U:Units)if(IsValid(U))
                {
                    U->SetActorTickEnabled(false);U->Tick(.1f);auto& Previous=Shots.FindOrAdd(U);
                    ActualShots[U->Team]+=FMath::Max(0,U->ShotsDischarged-Previous);Previous=U->ShotsDischarged;
                }
                Fleet->Tick(.1f);
                for(const auto& S:B->Squads)if(!CountedSquads.Contains(S.Id)){CountedSquads.Add(S.Id);S.Team==0?++PlayerRecruits:++EnemyRecruits;}
                int32 LivingArmour=0;for(AFrontlineTank* V:Fleet->GetVehicles())LivingArmour+=IsValid(V)&&V->Team==1&&V->IsOperational();SeenEnemyArmour=FMath::Max(SeenEnemyArmour,LivingArmour);
            }
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("faction"),F);Row->SetNumberField(TEXT("difficulty"),D);Row->SetBoolField(TEXT("archive"),Archive!=0);
            Row->SetNumberField(TEXT("simulatedSeconds"),B->Elapsed);Row->SetNumberField(TEXT("wallSeconds"),FPlatformTime::Seconds()-CaseStart);
            Row->SetNumberField(TEXT("playerRecruited"),PlayerRecruits);Row->SetNumberField(TEXT("enemyRecruited"),EnemyRecruits);
            Row->SetNumberField(TEXT("playerShots"),ActualShots[0]);Row->SetNumberField(TEXT("enemyShots"),ActualShots[1]);
            Row->SetNumberField(TEXT("playerLosses"),B->Casualties[0]);Row->SetNumberField(TEXT("enemyLosses"),B->Casualties[1]);
            Row->SetNumberField(TEXT("enemyFireMissions"),FrontlineDifficultyV30::State(B).PlannedFireMissions);Row->SetNumberField(TEXT("maxEnemyArmour"),SeenEnemyArmour);
            Row->SetNumberField(TEXT("playerMorale"),B->Morale[0]);Row->SetNumberField(TEXT("enemyMorale"),B->Morale[1]);
                        const bool GenuineEnd=B->BattleOutroWinner!=INDEX_NONE;
            Check(FString::Printf(TEXT("v84415_pressure_has_meaningful_ordinary_combat_f%d_d%d_a%d"),F,D,Archive),Started
                &&(B->Elapsed>=60.f||GenuineEnd)&&ActualShots[1]>0);
            Row->SetNumberField(TEXT("outcome"),B->BattleOutroWinner);Row->SetBoolField(TEXT("hitSimulationLimit"),B->Elapsed>=120.f);
            Row->SetBoolField(TEXT("genuineBattleOutcome"),GenuineEnd);
            Row->SetBoolField(TEXT("cpuBoundedBeforeMeaningfulWindow"),B->Elapsed<60.f&&!GenuineEnd);
            Row->SetStringField(TEXT("policy"),TEXT("Ordinary player rifle recruitment/shared cooldown and advance commands, real counterfire; no forced hits or invulnerable units. One prepared terrain only."));
            Pressure.Add(MakeShared<FJsonValueObject>(Row));
            B->AbandonBattleToOperations();if(Archive)FrontlineUltimateArchiveV8215::Leave(B);
        }
        auto Receipt=MakeShared<FJsonObject>();Receipt->SetArrayField(TEXT("catalogue240"),Coverage);Receipt->SetArrayField(TEXT("actualRecruits"),Actions);Receipt->SetArrayField(TEXT("boundedCombatPressure"),Pressure);
        Receipt->SetStringField(TEXT("scope"),TEXT("240 actual bound role/research/support/armour catalogue cases; 16 real ThinkEnemy recruits, observed mortar dispatch, and 12 CPU-bounded ordinary rifle-advance combat probes on one prepared terrain. Probes stop at 120 simulated seconds, three wall seconds per case or 50 total. These are not full campaign win rates."));
        const auto Directory=FPaths::Combine(UserDir,TEXT("Saved/EnemyV84415"));IFileManager::Get().MakeDirectory(*Directory,true);
        FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Receipt,Writer);
        Check(TEXT("v84415_enemy_receipt_written"),FFileHelper::SaveStringToFile(Json,*FPaths::Combine(Directory,TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    }
};
void RunFrontlineEnemyChecksV84415(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString Dir;const bool Safe=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV84415Focused"))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir);
    Check(TEXT("v84415_enemy_account_scope_requires_strict_disposable_profile"),Safe);if(!Safe)return;
    const FString Primary=FPaths::Combine(Dir,TEXT("AccountReview/achievements-v2.json")),Backup=Primary+TEXT(".bak");
    TArray<uint8> BeforePrimary,BeforeBackup;const bool HadPrimary=IFileManager::Get().FileExists(*Primary),HadBackup=IFileManager::Get().FileExists(*Backup);
    if((HadPrimary&&!FFileHelper::LoadFileToArray(BeforePrimary,*Primary))||(HadBackup&&!FFileHelper::LoadFileToArray(BeforeBackup,*Backup)))
    {Check(TEXT("v84415_enemy_disposable_account_snapshot"),false);return;}
    ON_SCOPE_EXIT
    {
        if(HadPrimary)FFileHelper::SaveArrayToFile(BeforePrimary,*Primary);else IFileManager::Get().Delete(*Primary);
        if(HadBackup)FFileHelper::SaveArrayToFile(BeforeBackup,*Backup);else IFileManager::Get().Delete(*Backup);
    };
    FrontlineAchievementsV76::WithScopedCombatLedgerV84412(B,[&]()
    {
        const bool Seeded=FrontlineAchievementsV76::SeedMedalReviewV8215(B,true,0);
        Check(TEXT("v84415_enemy_archive_routes_seeded_in_disposable_account_only"),Seeded);if(Seeded)FFrontlineEnemyChecksV84415::RunScoped(B,Check);
    });
}
