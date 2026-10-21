#include "WarfareFrontlineReviewV50.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineEnvironmentV42.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBalanceV50.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineGameplayV49.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlineMoraleV36.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "RHI.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
bool Allowed50(const AWarfareFrontlineGameMode* B)
{
    FString D;return B&&B->GetWorld()&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV50Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),D)&&!D.IsEmpty()&&!FPaths::IsRelative(D);
}
}

struct FFrontlineReviewV50
{
    static bool Ready(AWarfareFrontlineGameMode* B)
    {return B&&B->HasActorBegunPlay()&&B->LoadedSoldier&&B->IdleAnimation&&B->AssetFailure.IsEmpty();}
    static void Pose(AWarfareFrontlineSoldier* U,float DT)
    {
        U->Soldier->HandleExistingParallelEvaluationTask(true,true);
        U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();
    }
    static void Quiet(AWarfareFrontlineSoldier* U)
    {U->SetActorTickEnabled(false);U->ReturnToIdle();U->ShotClock=U->GrenadeClock=10000;U->Suppression=0;}
    static void Place(AWarfareFrontlineSoldier* U,float X,float Y)
    {
        const FVector P(X,Y,0);U->SetActorLocation(FVector(X,Y,FrontlineInfantryV45::SurfaceHeight(U->Battle.Get(),P)+88.f));Pose(U,0.f);
    }
    static bool Field(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Mission)
    {
        if(!Allowed50(B)||!FrontlineCampaignV41::PrepareReviewOperation(B,F,Mission))return false;
        B->BeginBattle();B->SetActorTickEnabled(false);UGameplayStatics::SetGamePaused(B,false);
        return B->Phase==EFrontlinePhase::Fighting&&B->AssetFailure.IsEmpty();
    }
    static void Clear(AWarfareFrontlineGameMode* B)
    {
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Empty();B->Squads.Empty();
        if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))V->ResetBattle();FrontlineMobilityV41::Reset(B);
        B->BattleOutroWinner=INDEX_NONE;B->Phase=EFrontlinePhase::Fighting;
    }
    static void Step(AWarfareFrontlineGameMode* B,float DT,bool Skeleton=true)
    {
        B->Elapsed+=DT;
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->IsAlive())
        {U->Tick(DT);if(Skeleton)Pose(U,DT);}
    }
    static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        auto Report=MakeShared<FJsonObject>();bool Good=true;
        const auto Test=[&](const FString& Name,bool Pass){Check(Name,Pass);Good&=Pass;};
        if(!Allowed50(B)){Test(TEXT("v50_isolated_profile_required"),false);Report->SetBoolField(TEXT("success"),false);return Report;}
        const auto Runs=B->CampaignRuns;const auto Id=B->ActiveCampaignId;const auto Faction=B->PlayerFaction;
        ON_SCOPE_EXIT{Clear(B);B->AbandonBattleToOperations();B->CampaignRuns=Runs;B->ActiveCampaignId=Id;B->PlayerFaction=Faction;};
        TArray<TSharedPtr<FJsonValue>> Ingress,Edges,CrewRows;
        // The geometry contract is exercised directly without twenty expensive
        // world reloads. Both faction home indices and authored bank remaps must
        // remain valid for every irregular 2 / 3 / 4-line operation.
        bool Geometry=true,DistinctInnerPositions=true;float MinPitch=MAX_flt;TSet<int32> Counts;
        TSet<int32> ThreeLineShapes,FourLineShapes;
        for(int32 Operation=0;Operation<20;++Operation)
        {
            FrontlineBattlefield::ConfigureOperationDepth(FrontlineGameplayV33::OperationDepthScale(Operation),Operation);
            using namespace FrontlineBattlefield;Counts.Add(TrenchCount);
            Geometry&=TrenchCount>=2&&TrenchCount<=MaxTrenchCount&&FMath::IsNearlyZero(TrenchX(0)+TrenchX(TrenchCount-1),.05f);
            const float Home=FMath::Abs(TrenchX(0));
            if(TrenchCount==3)
            {
                const float Centre=TrenchX(1)/Home;
                DistinctInnerPositions&=FMath::Abs(Centre)>=.15f&&FMath::Abs(Centre)<=.33f;
                ThreeLineShapes.Add(FMath::RoundToInt(Centre*1000.f));
            }
            else if(TrenchCount==4)
            {
                const float Left=TrenchX(1)/Home,Right=TrenchX(2)/Home;
                DistinctInnerPositions&=Left<-.1f&&Right>.1f&&FMath::Abs(Left+Right)>=.25f;
                FourLineShapes.Add(FMath::RoundToInt(Left*1000.f)*10000+FMath::RoundToInt(Right*1000.f));
            }
            for(int32 I=0;I<TrenchCount;++I)
            {
                Geometry&=ActiveFromAuthoring(AuthoringFromActive(I))==I;
                if(I>0){const float Pitch=TrenchX(I)-TrenchX(I-1);MinPitch=FMath::Min(MinPitch,Pitch);Geometry&=Pitch>=1800.f;}
            }
            for(int32 Team=0;Team<2;++Team)
            {
                const int32 Direction=Team? -1:1,Own=HomeTrench(Team),Other=HomeTrench(1-Team);
                Geometry&=Own>=0&&Own<TrenchCount&&Other>=0&&Other<TrenchCount
                    &&TrenchX(Own)*Direction<0&&TrenchX(Other)*Direction>0
                    &&FrontlineGameplayV49::BreakthroughLine()>TrenchX(Other)*Direction+500.f
                    &&FrontlinePacingV24::CombatHalfLength()>FrontlineGameplayV49::BreakthroughLine()+500.f;
            }
        }
        Test(TEXT("v50_twenty_operations_have_safe_mirrored_homes_goals_and_bank_mapping"),Geometry&&Counts.Num()>=3);
        Test(TEXT("v50_inner_positions_are_distinct_and_bounded_by_operation"),DistinctInnerPositions&&ThreeLineShapes.Num()==12&&FourLineShapes.Num()==2);
        Report->SetNumberField(TEXT("minimum_trench_pitch_cm"),MinPitch);
        for(auto F:{EFrontlineFaction::British,EFrontlineFaction::German})for(int32 Mission:{0,19})
        {
            const FString Prefix=FString::Printf(TEXT("v50_%s_m%02d_"),F==EFrontlineFaction::British?TEXT("british"):TEXT("german"),Mission+1);
            if(!Field(B,F,Mission)){Test(Prefix+TEXT("operation_loaded"),false);continue;}
            const int32 Dir=B->GetTeamDirection(0);const float Boundary=FrontlinePacingV24::CombatHalfLength();
            for(auto Role:{EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Pioneer})
            {
                Clear(B);B->Elapsed=50;
                const bool Spawned=B->SpawnSquad(0,Role);Test(Prefix+FString::Printf(TEXT("role_%d_spawn"),int32(Role)),Spawned);
                if(!Spawned)continue;
                for(AWarfareFrontlineSoldier* U:B->Units)Quiet(U);
                const auto* Lead=B->Units[0].Get();const float Before=Lead->Feet().X;
                const bool Offscreen=(-Dir*Before)>Boundary+800.f&&!Lead->IsActiveCombatant();
                float First=-1;bool CameraIndependent=true,Finite=true;
                for(int32 Frame=0;Frame<100;++Frame)
                {
                    Step(B,.05f,false);
                    for(AWarfareFrontlineSoldier* U:B->Units)
                    {Finite&=!U->Feet().ContainsNaN();if(First<0&&U->IsActiveCombatant())First=(Frame+1)*.05f;}
                }
                for(float Camera:{-9000.f,0.f,9000.f})
                    CameraIndependent&=FrontlinePacingV24::ReinforcementStartX(Dir,Camera,9000,0,.5f)==FrontlinePacingV24::ReinforcementStartX(Dir,0,.5f);
                Test(Prefix+FString::Printf(TEXT("role_%d_enters_in_about_three_seconds"),int32(Role)),Offscreen&&Finite&&First>=2.5f&&First<=3.5f&&CameraIndependent);
                auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("case"),Prefix);Row->SetNumberField(TEXT("role"),int32(Role));
                Row->SetNumberField(TEXT("first_visible_seconds"),First);Row->SetNumberField(TEXT("entry_boundary_cm"),Boundary);
                Row->SetNumberField(TEXT("start_x_cm"),Before);Row->SetBoolField(TEXT("camera_independent"),CameraIndependent);Ingress.Add(MakeShared<FJsonValueObject>(Row));
            }
            Clear(B);B->Elapsed=100;
            auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
            auto* T=Fleet?Fleet->Spawn(F==EFrontlineFaction::British?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V,0,true):nullptr;
            float TankEntry=-1;
            if(T)
            {
                T->SetActorTickEnabled(false);
                for(int32 Frame=0;Frame<90;++Frame){B->Elapsed+=.05f;T->Simulate(.05f);if(TankEntry<0&&T->HasEnteredField())TankEntry=(Frame+1)*.05f;}
            }
            Test(Prefix+TEXT("heavy_hull_enters_in_about_three_seconds"),T&&TankEntry>=2.2f&&TankEntry<=3.6f);
            // Production section and reinforcement actors, with the guard in
            // the exact rear strip that used to be visible but non-combatant.
            Clear(B);B->Elapsed=200;
            if(!B->SpawnSquad(0,EFrontlineRole::Rifle,B->GetHomeTrenchIndex(0)))
            {Test(Prefix+TEXT("edge_attacker_spawn"),false);continue;}
            const int32 AttackId=B->Squads.Last().Id;
            if(!B->SpawnSquad(1,EFrontlineRole::Rifle))
            {Test(Prefix+TEXT("edge_guard_spawn"),false);continue;}
            auto* Attack=B->FindSquad(AttackId);auto* Guard=B->Squads.Last().Members[0].Get();
            for(AWarfareFrontlineSoldier* U:B->Units)Quiet(U);
            for(const auto& W:B->Squads.Last().Members)if(W.Get()!=Guard)W->Health=0;
            Attack->Trench=INDEX_NONE;Attack->Destination=B->GetGoalTrenchIndex(0);Attack->LastPassedTrench=B->GetHomeTrenchIndex(1);
            Attack->bAdvancing=true;Attack->bReinforcement=false;Attack->Slot=1;
            const float Line=FrontlineGameplayV49::BreakthroughLine();
            auto* Scout=Attack->Members[0].Get();
            for(const auto& W:Attack->Members)Place(W.Get(),Dir*(Line+60),W->MemberIndex*110.f-250.f);
            Place(Guard,Dir*(Boundary-70),0);Guard->bFieldEntered=false;
            const bool GuardActiveBeforeOwnTick=Guard->IsActiveCombatant();
            const bool Blocks=FrontlineGameplayV49::EnemyEndDefended(B,0)&&!FrontlineGameplayV43::HasCrossedEnemyEnd(B,*Attack);
            bool Contact=false;
            for(float Lane:{-850.f,-400.f,0.f,400.f,850.f})
            {
                Place(Scout,Dir*(Line+60),Lane);Place(Guard,Dir*(Boundary-70),Lane);
                if(B->AcquireTarget(Scout,true)==Guard){Contact=true;break;}
            }
            // One held defender, with no invulnerability or artificial target
            // type. Suppress firing only to observe whether advancing men halt.
            Guard->SetActorTickEnabled(false);float LastTravel=0;
            for(int32 Frame=0;Frame<50;++Frame)
            {B->Elapsed+=.05f;const FVector Previous=Scout->Feet();Scout->Tick(.05f);Pose(Scout,.05f);if(Frame>=30)LastTravel+=FVector::Dist2D(Previous,Scout->Feet());}
            Test(Prefix+TEXT("visible_rear_reserve_is_targetable_before_own_tick"),GuardActiveBeforeOwnTick&&Contact);
            Test(Prefix+TEXT("visible_end_guard_halts_infantry_and_blocks_victory"),Blocks&&Scout->bEngaging&&LastTravel<1.f&&B->BattleOutroWinner==INDEX_NONE);
            // A man already passed by the attacker is still a contact. Apply
            // the same rule to the defender's own home sector as well.
            Place(Scout,Dir*(Line+220),0);Place(Guard,Dir*(Line-80),0);Guard->bFieldEntered=true;
            bool BehindContact=B->AcquireTarget(Scout,true)==Guard;
            Test(Prefix+TEXT("passed_rear_guard_is_not_ignored"),BehindContact);
            // Keep the passed pair in place: their opposite advance directions
            // put each man behind the other's nominal firing axis. The former
            // fixture swapped them back into a frontal meeting and left five
            // nearer attackers eligible, so correct nearest-target selection
            // could never equal the one arbitrarily named Scout.
            TArray<TPair<AWarfareFrontlineSoldier*,float>> OtherStrength;
            for(const auto& W:Attack->Members)if(auto* U=W.Get();U&&U!=Scout)
            {OtherStrength.Emplace(U,U->Health);U->Health=0;}
            const float GuardRearAlong=(Scout->Feet().X-Guard->Feet().X)*B->GetTeamDirection(Guard->Team);
            AWarfareFrontlineSoldier* HomeRearTarget=B->AcquireTarget(Guard,true);
            for(int32 Frame=0;Frame<20;++Frame){B->Elapsed+=.05f;Guard->Tick(.05f);Pose(Guard,.05f);}
            const FVector HomeAim=(Scout->Feet()-Guard->Feet()).GetSafeNormal2D();
            const bool OwnRearContact=GuardRearAlong< -110.f&&FVector::Dist2D(Scout->Feet(),Guard->Feet())>110.f
                &&FrontlineGameplayV49::InEndSector(B,0,Guard->Feet())&&B->HasSight(Guard,Scout)&&HomeRearTarget==Scout;
            Test(Prefix+TEXT("new_reserve_turns_toward_intruder_behind_its_home_entry"),OwnRearContact
                &&Guard->bEngaging&&Guard->GetMotionSpeed()<1.f&&FVector::DotProduct(Guard->GetActorForwardVector(),HomeAim)>.8f);
            for(const auto& Entry:OtherStrength)Entry.Key->Health=Entry.Value;
            Place(Guard,Dir*(Boundary+250),0);Guard->bFieldEntered=false;
            Test(Prefix+TEXT("truly_offscreen_reserve_does_not_prevent_victory"),!Guard->IsActiveCombatant()&&!FrontlineGameplayV49::EnemyEndDefended(B,0));
            Place(Guard,Dir*(Line-80),0);Guard->bFieldEntered=true;Guard->Health=0;
            for(const auto& W:Attack->Members)Place(W.Get(),Dir*(Line+100),W->MemberIndex*110.f-250.f);
            Test(Prefix+TEXT("cleared_sector_still_allows_whole_section_breakthrough"),FrontlineGameplayV43::HasCrossedEnemyEnd(B,*Attack));
            auto Edge=MakeShared<FJsonObject>();Edge->SetStringField(TEXT("case"),Prefix);Edge->SetNumberField(TEXT("first_tank_visible_seconds"),TankEntry);
            Edge->SetBoolField(TEXT("physical_rear_contact"),Contact);Edge->SetNumberField(TEXT("stopped_last_second_travel_cm"),LastTravel);Edges.Add(MakeShared<FJsonValueObject>(Edge));
            Edge->SetNumberField(TEXT("home_guard_target_along_cm"),GuardRearAlong);Edge->SetBoolField(TEXT("home_rear_contact"),OwnRearContact);
            if(Mission==19)ContinueCrew(B,Prefix,Test,CrewRows);
        }
        Report->SetArrayField(TEXT("ingress"),Ingress);Report->SetArrayField(TEXT("rear_edges"),Edges);Report->SetArrayField(TEXT("machinegun_crew"),CrewRows);
        Report->SetBoolField(TEXT("success"),Good&&Ingress.Num()==12&&Edges.Num()==4&&CrewRows.Num()==4);return Report;
    }
    static bool Scene(AWarfareFrontlineGameMode* B)
    {
        if(!Field(B,EFrontlineFaction::British,19))return false;Clear(B);
        B->SpawnSquad(0,EFrontlineRole::Lewis,B->GetHomeTrenchIndex(0));B->SpawnSquad(0,EFrontlineRole::Pioneer,B->GetHomeTrenchIndex(0));
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());if(!PC)return false;
        FrontlineReviewUIV41::Clear(PC);FrontlineReviewUIV41::Camera(PC,B->Trenches[B->GetHomeTrenchIndex(0)].X,400,6000,650);
        return B->Units.Num()==7;
    }
    static bool ArtScene(AWarfareFrontlineGameMode* B,int32 Index)
    {
        constexpr int32 Missions[]={0,1,5,10,16,17};if(Index<0||Index>=6)return false;
        const auto F=Index%2?EFrontlineFaction::German:EFrontlineFaction::British;
        if(!Field(B,F,Missions[Index]))return false;Clear(B);
        const int32 Home=B->GetHomeTrenchIndex(0);
        const bool Men=B->SpawnSquad(0,EFrontlineRole::Rifle,Home)&&B->SpawnSquad(0,EFrontlineRole::Lewis,Home)
            &&B->SpawnSquad(0,EFrontlineRole::Pioneer,Home);
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());if(!PC)return false;
        FrontlineReviewUIV41::Clear(PC);FrontlineReviewUIV41::Camera(PC,0,600,9000,650);
        if(auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
        {Settings->SetHDREnabled(false,false);Settings->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
        return Men;
    }
    static bool TextScene(AWarfareFrontlineGameMode* B,int32 Index)
    {
        if(!Allowed50(B))return false;
        const bool Chinese=Index==7;
        FrontlineLocalizationV35::SetLanguage(Chinese?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
        if(!FrontlineCampaignV41::PrepareReviewOperation(B,Chinese?EFrontlineFaction::German:EFrontlineFaction::British,Chinese?11:8))return false;
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());if(!PC)return false;
        FrontlineReviewUIV41::Dossier(PC);
        if(Chinese)PC->BeginBattle();
        FrontlineReviewUIV41::BeginAudit();return true;
    }
    static TSharedRef<FJsonObject> ArtState(AWarfareFrontlineGameMode* B,int32 Index)
    {
        auto J=MakeShared<FJsonObject>();constexpr int32 Missions[]={0,1,5,10,16,17};
        J->SetNumberField(TEXT("mission"),Missions[Index]);J->SetStringField(TEXT("faction"),Index%2?TEXT("German"):TEXT("British"));
        J->SetNumberField(TEXT("trench_count"),B->Trenches.Num());J->SetNumberField(TEXT("soldiers"),B->Units.Num());
        J->SetObjectField(TEXT("environment"),FrontlineEnvironmentV42::RunChecks(B));
        int32 GroundActors=0,BoundMeshes=0;
        for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
        {++GroundActors;for(UStaticMeshComponent* C:It->BakedTerrainV43)if(C&&C->GetStaticMesh()&&C->IsVisible())++BoundMeshes;}
        J->SetNumberField(TEXT("ground_actors"),GroundActors);J->SetNumberField(TEXT("visible_bound_terrain_tiles"),BoundMeshes);
        return J;
    }
    static void ContinueCrew(AWarfareFrontlineGameMode* B,const FString& Prefix,TFunctionRef<void(const FString&,bool)> Check,TArray<TSharedPtr<FJsonValue>>& Rows)
    {
        Clear(B);B->Elapsed=500;
        if(!B->SpawnSquad(0,EFrontlineRole::Lewis,B->GetHomeTrenchIndex(0)))
        {Check(Prefix+TEXT("mg_crew_spawn"),false);return;}
        auto* S=&B->Squads.Last();S->Trench=INDEX_NONE;S->Destination=B->GetGoalTrenchIndex(0);S->LastPassedTrench=B->GetHomeTrenchIndex(1);
        S->bAdvancing=true;S->bReinforcement=false;S->Slot=1;
        TArray<AWarfareFrontlineSoldier*> Men;for(const auto& W:S->Members){Men.Add(W.Get());Quiet(W.Get());}
        const int32 Dir=B->GetTeamDirection(0);const float X=Dir*(FrontlinePacingV24::CombatHalfLength()-1200.f);
        for(int32 I=0;I<Men.Num();++I)Place(Men[I],X,I*330.f-250.f);
        auto* G=Men[0];auto* Gun=FrontlineMachinegunsV30::Get(G);
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());auto* Tank=Fleet?Fleet->Spawn(EFrontlineVehicle::A7V,1,false):nullptr;
        if(!Gun||!Tank){Check(Prefix+TEXT("mg_contact_assets"),false);return;}
        Tank->bEntered=true;Tank->SetActorTickEnabled(false);Tank->FitGroundV35(FVector(X+Dir*850.f,-250,0),0);
        for(int32 Frame=0;Frame<180&&!Gun->IsSupported();++Frame){B->Elapsed+=.05f;G->Tick(.05f);Pose(G,.05f);}
        if(!Gun->IsSupported()){Check(Prefix+TEXT("mg_actual_deployment"),false);return;}
        for(int32 I=1;I<Men.Num();++I)
        {bool Reform=false;const FVector P=FrontlineMachinegunsV30::Destination(Men[I],B->SoldierDestination(Men[I]),Reform);Place(Men[I],P.X+Dir*140.f,P.Y+3);}
        FrontlineMobilityV41::Reset(B);
        for(int32 Frame=0;Frame<180;++Frame)Step(B,.05f);
        G->ShotClock=G->AimSettleClock=0;const int32 Shots=G->ShotsDischarged;
        float Travel[3]={};int32 Packs[3]={};bool Locomotion=false;
        for(int32 I=1;I<Men.Num();++I)if(auto* C=FrontlineMachinegunsV30::Get(Men[I]))Packs[I]=C->Packs;
        for(int32 Frame=0;Frame<80;++Frame)
        {
            // Gun traverses a physically visible moving hull inside its normal
            // firing sector. Escorts should not chase that few-degree traverse.
            Tank->FitGroundV35(FVector(X+Dir*850.f,-250+70.f*FMath::Sin(Frame*.12f),0),0);
            FVector Before[3];for(int32 I=0;I<Men.Num();++I)Before[I]=Men[I]->Feet();Step(B,.05f);
            for(int32 I=1;I<Men.Num();++I){Travel[I]+=FVector::Dist2D(Before[I],Men[I]->Feet());Locomotion|=Men[I]->bMoving||Men[I]->Playing==Men[I]->GetRunMotion();}
        }
        for(int32 I=1;I<Men.Num();++I)
        {
            auto* C=FrontlineMachinegunsV30::Get(Men[I]);const bool Stable=C&&C->IsSupported()&&C->Packs==Packs[I]&&Travel[I]<1.f&&!Locomotion;
            Check(Prefix+FString::Printf(TEXT("mg_escort_%d_stable_under_traversing_fire"),I),Stable);
            auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("case"),Prefix);R->SetNumberField(TEXT("member"),I);
            R->SetNumberField(TEXT("travel_cm_over_four_seconds"),Travel[I]);R->SetNumberField(TEXT("new_packs"),C?C->Packs-Packs[I]:-1);
            R->SetNumberField(TEXT("gunner_shots"),G->ShotsDischarged-Shots);Rows.Add(MakeShared<FJsonValueObject>(R));
        }
        Check(Prefix+TEXT("mg_gunner_keeps_firing"),G->ShotsDischarged>Shots);
    }
};

void StartFrontlineV50Review(AWarfareFrontlineGameMode* B)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV50Review")))return;
    struct FRun{TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;double Start=FPlatformTime::Seconds();bool Good=true;int32 Stage=0,ArtIndex=0;double Next=0;FString Dir,Shot;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Art;};
    auto R=MakeShared<FRun>();R->Battle=B;R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V50/Review"));IFileManager::Get().MakeDirectory(*R->Dir,true);
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV50TextOnly")))R->ArtIndex=6;
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
    {
        auto* B=R->Battle.Get();const double Now=FPlatformTime::Seconds();
        const auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Good;};
        const auto Finish=[&]()
        {
            R->Report->SetBoolField(TEXT("success"),R->Good);R->Report->SetArrayField(TEXT("checks"),R->Checks);int32 Passed=0;
            for(const auto& C:R->Checks)Passed+=C->AsObject()->GetBoolField(TEXT("passed"));R->Report->SetNumberField(TEXT("passed"),Passed);R->Report->SetNumberField(TEXT("total"),R->Checks.Num());
            R->Report->SetBoolField(TEXT("null_rhi_logic_only"),GUsingNullRHI);R->Report->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-R->Start);
            R->Report->SetBoolField(TEXT("art_only"),FParse::Param(FCommandLine::Get(),TEXT("WFV50Art")));R->Report->SetArrayField(TEXT("art_scenes"),R->Art);
            R->Report->SetStringField(TEXT("capture"),R->Shot);R->Report->SetStringField(TEXT("scope"),TEXT("V50 focused production actor checks: operation-relative infantry/hull ingress, same-frame rear eligibility, end-defense and paired machinegun crew. Private volatile profile only. No performance or universal art claims."));
            FString JSON;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V50_REVIEW_DONE success=%d checks=%d"),R->Good,R->Checks.Num());FPlatformMisc::RequestExit(false);
        };
        if(!B||Now-R->Start>420){Check(TEXT("v50_bounded_world_and_time"),false);Finish();return false;}
        if(!FFrontlineReviewV50::Ready(B))return true;
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV50Art")))
        {
            if(!Allowed50(B)||GUsingNullRHI){Check(TEXT("v50_art_requires_isolated_profile_and_renderer"),false);Finish();return false;}
            if(R->ArtIndex>=6)
            {
                if(R->Stage==0)
                {Check(TEXT("v50_authored_text_page_ready"),FFrontlineReviewV50::TextScene(B,R->ArtIndex));R->Stage=1;R->Next=FPlatformTime::Seconds()+2.;return true;}
                if(Now<R->Next)return true;
                if(R->Stage==1)
                {
                    const auto Audit=FrontlineReviewUIV41::EndAudit();
                    Check(TEXT("v50_authored_text_and_images_fit"),Audit->GetBoolField(TEXT("success")));
                    R->Shot=R->Dir/(R->ArtIndex==6?TEXT("07-v50-warroom-english.png"):TEXT("08-v50-dispatch-chinese.png"));
                    Audit->SetStringField(TEXT("capture"),R->Shot);R->Art.Add(MakeShared<FJsonValueObject>(Audit));
                    FScreenshotRequest::RequestScreenshot(R->Shot,true,false);R->Stage=2;R->Next=Now+1.;return true;
                }
                Check(TEXT("v50_authored_text_capture_written"),IFileManager::Get().FileSize(*R->Shot)>1024);
                if(++R->ArtIndex>=8){Finish();return false;}R->Stage=0;return true;
            }
            if(R->Stage==0)
            {
                const bool Loaded=FFrontlineReviewV50::ArtScene(B,R->ArtIndex);
                Check(FString::Printf(TEXT("v50_art_%d_operation_loaded"),R->ArtIndex),Loaded);if(!Loaded){Finish();return false;}
                R->Stage=1;R->Next=FPlatformTime::Seconds()+3.5;return true;
            }
            B->Elapsed+=FMath::Clamp(DT,0.f,.05f);if(Now<R->Next)return true;
            if(R->Stage==1)
            {
                const auto Scene=FFrontlineReviewV50::ArtState(B,R->ArtIndex);
                Check(FString::Printf(TEXT("v50_art_%d_environment_contract"),R->ArtIndex),Scene->GetObjectField(TEXT("environment"))->GetBoolField(TEXT("success")));
                Check(FString::Printf(TEXT("v50_art_%d_baked_terrain_bound"),R->ArtIndex),Scene->GetNumberField(TEXT("visible_bound_terrain_tiles"))>0);
                R->Shot=R->Dir/FString::Printf(TEXT("%02d-v50-regional-operation.png"),R->ArtIndex+1);
                Scene->SetStringField(TEXT("capture"),R->Shot);R->Art.Add(MakeShared<FJsonValueObject>(Scene));
                FScreenshotRequest::RequestScreenshot(R->Shot,true,false);R->Stage=2;R->Next=Now+1.;return true;
            }
            Check(FString::Printf(TEXT("v50_art_%d_capture_written"),R->ArtIndex),IFileManager::Get().FileSize(*R->Shot)>1024);
            ++R->ArtIndex;R->Stage=0;return true;
        }
        if(R->Stage==0)
        {
            Check(TEXT("v50_isolated_volatile_no_save"),Allowed50(B));if(!Allowed50(B)){Finish();return false;}
            const auto Logic=FFrontlineReviewV50::Run(B,Check);R->Report->SetObjectField(TEXT("combat"),Logic);Check(TEXT("v50_combat_checks_complete"),Logic->GetBoolField(TEXT("success")));
            const bool BalanceField=FFrontlineReviewV50::Field(B,EFrontlineFaction::British,0);
            Check(TEXT("v50_balance_operation_ready"),BalanceField);
            if(BalanceField){FFrontlineReviewV50::Clear(B);FrontlineBalanceV50::RunChecks(B,Check);}
            RunFrontlineWeatherChecksV50(Check);
            if(BalanceField)FrontlineMoraleV36::RunLayoutChecksV50(B,Check);
            FrontlineCampaignV41::RunChecksV50(B,Check);
            if(GUsingNullRHI){Finish();return false;}
            // Show the production end approach with its true ground and camera.
            Check(TEXT("v50_rear_and_crew_scene"),FFrontlineReviewV50::Scene(B));
            R->Stage=1;R->Next=FPlatformTime::Seconds()+4.;return true;
        }
        if(Now<R->Next)return true;
        if(R->Stage==1){R->Shot=R->Dir/TEXT("01-v50-operation-rear-and-crew.png");FScreenshotRequest::RequestScreenshot(R->Shot,true,false);R->Stage=2;R->Next=Now+1.;return true;}
        Check(TEXT("v50_requested_capture_written"),IFileManager::Get().FileSize(*R->Shot)>1024);Finish();return false;
    }));
}
