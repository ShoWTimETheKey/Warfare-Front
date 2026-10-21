#include "WarfareFrontlineTrenchMotionV8447Checks.h"
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineBalanceV50.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

namespace {bool LegacyContactRetentionV87=false;}
// False in ordinary play. Only the qualified synchronous paired review below
// binds the preceding retention branch, then restores it before returning.
bool FrontlineUseLegacyContactRetentionV87(){return LegacyContactRetentionV87;}

struct FFrontlineTrenchMotionV8447Review
{
    struct FContext
    {
        AWarfareFrontlineGameMode* B;
        TArray<TObjectPtr<AWarfareFrontlineSoldier>> Units;
        TArray<FFrontlineSquad> Squads;
        TArray<FFrontlineTrench> Trenches;
        TArray<FFrontlineTracer> Tracers;
        TArray<FFrontlineSmokeZone> Smoke;
        EFrontlinePhase Phase;float Elapsed,Peak;int32 Outro,Shots,Advances;
        int32 Casualties[2];float Morale[2],Damage[2];FFrontlineCombatRecordV65 Records[2];
        FString Message;
        struct FOriginal{TWeakObjectPtr<AWarfareFrontlineSoldier> U;bool Hidden,Collision;};
        TArray<FOriginal> Originals;TArray<AWarfareFrontlineSoldier*> Made;
        int32 CaptureTeam=INDEX_NONE;
        bool CaptureMachinegun=false;
        explicit FContext(AWarfareFrontlineGameMode* In):B(In),Units(In->Units),Squads(In->Squads),Trenches(In->Trenches),
            Tracers(In->Tracers),Smoke(In->SmokeZones),Phase(In->Phase),Elapsed(In->Elapsed),Peak(In->PeakSuppression),
            Outro(In->BattleOutroWinner),Shots(In->ShotsFired),Advances(In->AdvanceCommands),Message(In->LastMessage)
        {
            for(int32 I=0;I<2;++I){Casualties[I]=B->Casualties[I];Morale[I]=B->Morale[I];Damage[I]=B->DamageDealt[I];Records[I]=B->CombatRecordV65[I];}
            for(AWarfareFrontlineSoldier* U:Units)if(IsValid(U)){Originals.Add({U,U->IsHidden(),U->GetActorEnableCollision()});U->SetActorHiddenInGame(true);U->SetActorEnableCollision(false);}
            Clear();
        }
        void Clear()
        {
            for(auto* U:Made)if(IsValid(U))U->Destroy();Made.Reset();
            B->Units.Reset();B->Squads.Reset();B->Tracers.Reset();B->SmokeZones.Reset();
            B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=100.f;
            FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
        }
        ~FContext()
        {
            Clear();B->Units=Units;B->Squads=Squads;B->Trenches=Trenches;B->Tracers=Tracers;B->SmokeZones=Smoke;
            B->Phase=Phase;B->Elapsed=Elapsed;B->PeakSuppression=Peak;B->BattleOutroWinner=Outro;B->ShotsFired=Shots;
            B->AdvanceCommands=Advances;B->LastMessage=Message;
            for(int32 I=0;I<2;++I){B->Casualties[I]=Casualties[I];B->Morale[I]=Morale[I];B->DamageDealt[I]=Damage[I];B->CombatRecordV65[I]=Records[I];}
            for(const auto& O:Originals)if(auto* U=O.U.Get()){U->SetActorHiddenInGame(O.Hidden);U->SetActorEnableCollision(O.Collision);}
            FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
        }
    };
    static TUniquePtr<FContext>& Capture(){static TUniquePtr<FContext> C;return C;}
    static bool Allowed(AWarfareFrontlineGameMode* B)
    {
        FString UserDir;
        return B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV8447Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty()&&!FPaths::IsRelative(UserDir);
    }
    static void Pose(AWarfareFrontlineSoldier* U,float DT)
    {U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();}
    static AWarfareFrontlineSoldier* Make(FContext& C,int32 Id,int32 Team,EFrontlineRole Role,int32 Member,FVector P)
    {
        auto* U=C.B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return nullptr;
        U->Initialize(C.B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->ReturnToIdle();U->bFieldEntered=true;
        U->ShotClock=U->GrenadeClock=10000.f;U->DepartureClock=U->ReactionSeconds=0;U->Health=10000.f;
        P.Z=FrontlineInfantryV45::SurfaceHeight(C.B,P);U->SetActorLocation(P+FVector(0,0,88));
        U->SetActorRotation(FRotator(0,C.B->GetTeamDirection(Team)>0?0.f:180.f,0));
        C.B->Units.Add(U);C.Made.Add(U);Pose(U,0);return U;
    }
    static void Tick(AWarfareFrontlineSoldier* U,float DT)
    {U->Tick(DT);Pose(U,DT);if(auto* Gun=FrontlineMachinegunsV30::Get(U))Gun->TickComponent(DT,LEVELTICK_All,nullptr);}
    static TArray<AWarfareFrontlineSoldier*> Held(FContext& C,int32 Id,int32 Team,EFrontlineRole Role)
    {
        auto* B=C.B;const int32 Trench=B->GetHomeTrenchIndex(Team),Dir=B->GetTeamDirection(Team);
        TArray<AWarfareFrontlineSoldier*> Men;if(!B->Trenches.IsValidIndex(Trench))return Men;
        B->Trenches[Trench].Owner=Team;
        FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=Role;Q.Trench=Q.Destination=Q.LastPassedTrench=Trench;Q.Slot=1;Q.bHoldPosition=true;
        const int32 Count=B->GetSquadSize(Role,Team);
        for(int32 Member=0;Member<Count;++Member)if(auto* U=Make(C,Id,Team,Role,Member,FVector(B->Trenches[Trench].X,-800.f+Member*140.f,0)))
        {Men.Add(U);Q.Members.Add(U);}
        B->Squads.Add(Q);
        for(auto* U:Men){FVector P=B->SoldierDestination(U);P.X=FrontlineFieldV32::TrenchCentreX(Trench,P.Y)-Dir*100.f;P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));}
        for(int32 I=0;I<1920;++I){B->Elapsed+=1.f/120.f;for(auto* U:Men)Tick(U,1.f/120.f);}
        return Men;
    }
    static TSharedRef<FJsonObject> ExistingContactV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> External)
    {
        auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
        const auto Check=[&](const FString& N,bool Pass){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),N);R->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(R));Good&=Pass;External(N,Pass);};
        FString UserDir;
        const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
            &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&(Allowed(B)||FrontlineUltimateArchiveV8215::ReviewUserDir(UserDir));
        Check(TEXT("v87_existing_contact_disposable_uk_de_only"),Safe);
        const auto Finish=[&](){J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("cases"),Cases);
            J->SetStringField(TEXT("scope"),TEXT("Three identical-input legacy/corrected pairs through actual Soldier Tick and evaluated skeletal poses. Existing visible contact, then friendly smoke arrival; measured committed travel, destination, end feet and contact state. No historical motion suite, natural campaign or performance acceptance."));return J;};
        if(!Safe)return Finish();
        FContext C(B);const auto OriginalRandom=B->Random;
        ON_SCOPE_EXIT{B->Random=OriginalRandom;};
        // A smoke screen may arrive after the section has already acquired an
        // open-ground contact. Exercise that state change through Soldier Tick;
        // checking only fresh acquisition missed the retained Focus contract.
        for(int32 Scenario=0;Scenario<3;++Scenario)
        {
            struct FSample{bool Valid=false,Engaging=false;float Forward=0,Travel=0;FVector Start=FVector::ZeroVector,End=FVector::ZeroVector,Destination=FVector::ZeroVector;};
            FSample Samples[2];TArray<TSharedPtr<FJsonValue>> Pair;
            for(int32 Comparison=0;Comparison<2;++Comparison)
            {
                C.Clear();const auto OriginalFaction=B->PlayerFaction;
                B->PlayerFaction=Scenario==2?EFrontlineFaction::British:EFrontlineFaction::German;
                auto& Research=B->ActiveResearchMutable();const auto OriginalResearch=Research;
                const bool OriginalLegacy=LegacyContactRetentionV87;
                ON_SCOPE_EXIT{LegacyContactRetentionV87=OriginalLegacy;Research=OriginalResearch;B->PlayerFaction=OriginalFaction;};
                LegacyContactRetentionV87=Comparison==0;B->Random=OriginalRandom;
                Research.TechnologyMask=Scenario==1||Scenario==2?1u<<9:0u;
                const int32 Team=0,Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team);
                const int32 Next=FMath::Clamp(Home+Dir,0,B->Trenches.Num()-1),Id=8701100+Scenario;
                const bool GroundReady=B->Trenches.IsValidIndex(Home)&&B->Trenches.IsValidIndex(Next)&&Next!=Home;
                Check(FString::Printf(TEXT("v87_smoke_s%d_%s_actual_trench_geometry_ready"),Scenario,Comparison==0?TEXT("legacy"):TEXT("corrected")),GroundReady);
                if(!GroundReady)continue;
                const FVector Start((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,0,0);
                auto* U=Make(C,Id,Team,EFrontlineRole::Mills,1,Start);
                if(U)U->MotionRandom.Initialize(Id);
                const float Range=U?FrontlineWeapons::AdvanceContactRangeFor(U)*B->EncounterVisibilityScale():0.f;
                const float Gap=Range*.8f+110.f;
                auto* E=Make(C,Id+100,1-Team,EFrontlineRole::Rifle,1,Start+FVector(Dir*Gap,0,0));
                const FString Prefix=FString::Printf(TEXT("v87_smoke_s%d_%s_"),Scenario,Comparison==0?TEXT("legacy"):TEXT("corrected"));
                Check(Prefix+TEXT("actual_grounded_actors_and_distance_witness"),U&&E&&Range>550.f);
                if(!U||!E||Range<=550.f)continue;
                FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=EFrontlineRole::Mills;Q.Trench=INDEX_NONE;
                Q.Destination=Next;Q.bAdvancing=true;Q.Members.Add(U);B->Squads.Add(Q);
                B->Elapsed+=.05f;Tick(U,.05f);
                const FVector ContactFeet=U->Feet();
                const bool PriorContact=U->bEngaging&&U->FocusTarget.Get()==E&&B->AcquireTarget(U,true)==E&&!U->bMoving;
                Check(Prefix+TEXT("normal_tick_establishes_visible_contact_before_smoke"),PriorContact&&B->HasSight(U,E));
                FFrontlineSmokeZone Smoke;Smoke.Team=Team;Smoke.Centre=U->Feet();Smoke.Radius=300.f;
                Smoke.StartedAt=B->Elapsed-.1f;Smoke.ExpiresAt=B->Elapsed+10.f;B->SmokeZones.Add(Smoke);
                const float Scale=B->AssaultContactScale(U);
                const bool FreshContact=B->AcquireTarget(U,true)==E;
                const FVector Destination=B->SoldierDestination(U);float Travel=0.f;
                for(int32 I=0;I<12;++I){B->Elapsed+=.05f;const FVector Before=U->Feet();Tick(U,.05f);Travel+=FVector::Dist2D(Before,U->Feet());}
                const float Forward=float((U->Feet().X-ContactFeet.X)*Dir);
                const bool Eligibility=B->IsInFriendlySmoke(ContactFeet,Team)
                    &&FMath::IsNearlyEqual(Scale,Scenario==1?.6f:1.f,.001f)&&FreshContact==(Scenario!=1);
                Check(Prefix+TEXT("same_smoke_changes_only_eligible_german_infiltration"),Eligibility);
                const bool Physical=!U->Feet().ContainsNaN()&&FMath::IsFinite(Travel)&&Forward>=-.01f
                    &&B->SoldierDestination(U).Equals(Destination,.001f)&&U->ShotsDischarged==0&&U->GrenadeThrowsStarted==0;
                Check(Prefix+TEXT("physical_tick_keeps_order_and_spends_no_ammunition"),Physical);
                Samples[Comparison]={PriorContact&&Eligibility&&Physical,U->bEngaging,Forward,Travel,ContactFeet,U->Feet(),Destination};
                auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("logic"),Comparison==0?TEXT("preceding_unscaled_retention"):TEXT("corrected_current_smoke_scale"));
                Row->SetNumberField(TEXT("range_cm"),Range);Row->SetNumberField(TEXT("initial_gap_cm"),Gap);
                Row->SetNumberField(TEXT("current_contact_scale"),Scale);Row->SetBoolField(TEXT("fresh_contact_after_smoke"),FreshContact);
                Row->SetNumberField(TEXT("actual_forward_travel_cm"),Forward);Row->SetNumberField(TEXT("actual_path_travel_cm"),Travel);
                Row->SetStringField(TEXT("contact_start_feet"),ContactFeet.ToString());Row->SetStringField(TEXT("end_feet"),U->Feet().ToString());
                Row->SetStringField(TEXT("unchanged_order_destination"),Destination.ToString());Row->SetBoolField(TEXT("engaging_after_smoke"),U->bEngaging);
                Pair.Add(MakeShared<FJsonValueObject>(Row));
            }
            const bool Identical=Samples[0].Valid&&Samples[1].Valid&&Samples[0].Start.Equals(Samples[1].Start,.001f)
                &&Samples[0].Destination.Equals(Samples[1].Destination,.001f);
            const bool Passed=Identical&&Samples[0].Forward<=.01f&&Samples[0].Travel<=.01f&&Samples[0].Engaging
                &&(Scenario==1?Samples[1].Forward>10.f:Samples[1].Forward<=.01f&&Samples[1].Travel<=.01f&&Samples[1].Engaging);
            const TCHAR* Names[]={TEXT("v87_smoke_german_without_research_keeps_existing_contact"),
                TEXT("v87_smoke_german_infiltration_releases_old_contact_and_moves"),TEXT("v87_smoke_british_same_bit_keeps_existing_contact")};
            Check(Names[Scenario],Passed);
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Names[Scenario]);Row->SetBoolField(TEXT("identical_initial_feet_and_order"),Identical);
            Row->SetBoolField(TEXT("passed"),Passed);Row->SetArrayField(TEXT("actual_tick_comparison"),Pair);Cases.Add(MakeShared<FJsonValueObject>(Row));
        }
        return Finish();
    }
    static TSharedRef<FJsonObject> MachinegunContinuityV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> External)
    {
        auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
        const auto Check=[&](const FString& N,bool Pass){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),N);R->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(R));Good&=Pass;External(N,Pass);};
        FString UserDir;const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
            &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(UserDir);
        Check(TEXT("v87_mg_continuity_disposable_uk_de_only"),Safe);
        const auto Finish=[&](){J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("cases"),Cases);
            J->SetStringField(TEXT("scope"),TEXT("Six current-terrain UK/DE real-Tick cases at 30 Hz: owned-trench MG reload, a settled ground helper after contact changes flank, and an unreached ground helper paused by its actual rifle reload. No forged gun state, Focus, motion clip or weapon phase. Station service is sampled before helper Tick; committed movement/pose/state are sampled afterwards. No FRRU, natural full campaign, screenshots or performance claim."));return J;};
        if(!Safe)return Finish();
        FContext C(B);const auto OriginalRandom=B->Random;ON_SCOPE_EXIT{B->Random=OriginalRandom;};
        constexpr float DT=1.f/30.f;
        for(int32 Team:{0,1})for(int32 Scenario=0;Scenario<3;++Scenario)
        {
            C.Clear();B->Random=OriginalRandom;
            const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team),Next=FMath::Clamp(Home+Dir,0,B->Trenches.Num()-1);
            const int32 Id=8702000+Team*100+Scenario;
            const FString Prefix=FString::Printf(TEXT("v87_mg_t%d_s%d_"),Team,Scenario);
            const bool Terrain=B->Trenches.IsValidIndex(Home)&&B->Trenches.IsValidIndex(Next)&&Home!=Next;
            Check(Prefix+TEXT("actual_trench_geometry_ready"),Terrain);if(!Terrain)continue;
            FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=EFrontlineRole::Lewis;Q.Slot=1;
            Q.Trench=Scenario==0?Home:INDEX_NONE;Q.Destination=Scenario==0?Home:Next;Q.LastPassedTrench=Home;
            Q.bAdvancing=Scenario!=0;Q.bHoldPosition=Scenario==0;B->Squads.Add(Q);
            if(Scenario==0)B->Trenches[Home].Owner=Team;
            const FVector Ground((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,0,0);
            const FVector Spawn=Scenario==0?FVector(FrontlineFieldV32::TrenchCentreX(Home,0),0,0):Ground;
            auto* G=Make(C,Id,Team,EFrontlineRole::Lewis,0,Spawn);TArray<AWarfareFrontlineSoldier*> Crew;
            if(G){Crew.Add(G);B->FindSquad(Id)->Members.Add(G);}
            if(Scenario==0)for(int32 Member=1;Member<B->GetSquadSize(EFrontlineRole::Lewis,Team);++Member)
                if(auto* U=Make(C,Id,Team,EFrontlineRole::Lewis,Member,Spawn)){Crew.Add(U);B->FindSquad(Id)->Members.Add(U);}
            if(Scenario==0)for(auto* U:Crew)
            {FVector P=B->SoldierDestination(U);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));Pose(U,0);}
            TArray<AWarfareFrontlineSoldier*> FrontContacts;
            if(G)for(int32 Contact=0;Contact<(Scenario==0?3:1);++Contact)
            {
                const FVector P=Scenario==0?FVector(FrontlineFieldV32::TrenchCentreX(Home,G->Feet().Y+Contact*90.f)+Dir*(1000.f+Contact*90.f),G->Feet().Y+Contact*90.f,0):G->Feet()+FVector(Dir*650.f,0,0);
                if(auto* E=Make(C,Id+1000+Contact,1-Team,EFrontlineRole::Rifle,1,P))FrontContacts.Add(E);
            }
            Check(Prefix+TEXT("actual_role_actors_and_front_contact_created"),G&&Crew.Num()==(Scenario==0?B->GetSquadSize(EFrontlineRole::Lewis,Team):1)&&!FrontContacts.IsEmpty());
            if(!G||FrontContacts.IsEmpty())continue;
            for(int32 I=0;I<180;++I){B->Elapsed+=DT;for(auto* U:Crew)Tick(U,DT);}
            auto* Gun=FrontlineMachinegunsV30::Get(G);
            const bool Prepared=Gun&&Gun->HasAssets()&&Gun->IsSupported()&&Gun->IsTrench()==(Scenario==0)&&G->FocusTarget.IsValid();
            Check(Prefix+TEXT("normal_tick_reaches_intended_supported_context"),Prepared);if(!Prepared)continue;
            AWarfareFrontlineSoldier* A=Scenario==0&&Crew.Num()>1?Crew[1]:nullptr;
            FVector OriginalStation=FVector::ZeroVector;bool InitialReform=false;
            if(Scenario!=0)
            {
                const FVector Front=FRotator(0,Gun->FormationYawV50(),0).Vector(),Right=FVector::CrossProduct(FVector::UpVector,Front);
                A=Make(C,Id,Team,EFrontlineRole::Lewis,1,G->Feet()-Front*206.f+Right*125.f);
                if(A)
                {
                    B->FindSquad(Id)->Members.Add(A);Crew.Add(A);
                    const FVector P=G->Feet()-Front*206.f+Right*(120.f+A->GetMotionPhase()*10.f);
                    A->SetActorLocation(FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P)+88));Pose(A,0);
                    OriginalStation=FrontlineMachinegunsV30::Destination(A,B->SoldierDestination(A),InitialReform);
                }
                Check(Prefix+TEXT("real_helper_receives_unreached_ground_station"),A&&InitialReform&&FVector::Dist2D(A->Feet(),OriginalStation)>64.f);
                if(!A)continue;
                if(Scenario==1)for(int32 I=0;I<150;++I){B->Elapsed+=DT;for(auto* U:Crew)Tick(U,DT);}
                bool Reform=false;OriginalStation=FrontlineMachinegunsV30::Destination(A,B->SoldierDestination(A),Reform);
                const bool Arrival=Scenario==1?!Reform&&FVector::Dist2D(A->Feet(),OriginalStation)<=64.f:Reform&&FVector::Dist2D(A->Feet(),OriginalStation)>64.f;
                Check(Prefix+TEXT("settled_or_transit_condition_is_observed_before_flank_change"),Arrival);
                if(Scenario==2){A->RoundsRemaining=0;A->StartReload();}
            }
            else {G->RoundsRemaining=0;G->StartReload();}
            const FVector GunOrigin=G->Feet(),HelperOrigin=A?A->Feet():FVector::ZeroVector;
            const int32 FirstDeploy=Gun->Deployments,FirstPack=Gun->Packs,FirstReloads=(Scenario==0?G:A)->ReloadsStarted;
            const float InitialFormationYaw=Gun->FormationYawV50();
            AWarfareFrontlineSoldier* Rear=nullptr;
            if(Scenario!=0)
            {
                Rear=Make(C,Id+2000,1-Team,EFrontlineRole::Rifle,1,G->Feet()-FRotator(0,InitialFormationYaw,0).Vector()*650.f);
                for(auto* E:FrontContacts){B->Units.Remove(E);E->Destroy();}FrontContacts.Reset();
            }
            Check(Prefix+TEXT("stimulus_is_one_actual_reload_or_new_visible_rear_actor"),Scenario==0?G->WeaponPhase==EFrontlineWeaponPhase::Reload:
                Rear&&B->HasSight(G,Rear)&&(Scenario!=2||A->WeaponPhase==EFrontlineWeaponPhase::Reload));
            float MaxGoalShift=0,HelperTravel=0,ReverseTravel=0,MaxStep=0,MinGap=MAX_flt,FirstShiftAt=-1,FirstShiftTravel=-1,MinPelvis=MAX_flt,MaxPelvis=-MAX_flt;
            int32 FirstShiftPhase=INDEX_NONE,MovingFlips=0,FirstShiftGunState=INDEX_NONE;bool LastMoving=A&&A->bMoving,Finite=true;
            bool SawSectorPack=false,SawNewSupport=false;TArray<TSharedPtr<FJsonValue>> Frames;
            const float Began=B->Elapsed;const int32 FramesToRun=240;
            for(int32 I=0;I<FramesToRun;++I)
            {
                B->Elapsed+=DT;const FVector BeforeG=G->Feet(),BeforeA=A?A->Feet():FVector::ZeroVector;
                Tick(G,DT);FVector Station=OriginalStation;bool Reform=false;
                if(A){Station=FrontlineMachinegunsV30::Destination(A,B->SoldierDestination(A),Reform);Tick(A,DT);}
                for(auto* U:Crew)if(U!=G&&U!=A)Tick(U,DT);
                const float Shift=A&&Scenario!=0?FVector::Dist2D(Station,OriginalStation):0.f;
                MaxGoalShift=FMath::Max(MaxGoalShift,Shift);MaxStep=FMath::Max(MaxStep,float(FVector::Dist2D(BeforeG,G->Feet())));
                if(A)
                {
                    const FVector Travel=A->Feet()-BeforeA;HelperTravel+=Travel.Size2D();MaxStep=FMath::Max(MaxStep,float(Travel.Size2D()));
                    const FVector OldFront=FRotator(0,InitialFormationYaw,0).Vector();ReverseTravel+=FMath::Max(0.f,-float(FVector::DotProduct(Travel,OldFront)));
                    if(A->bMoving!=LastMoving)++MovingFlips;LastMoving=A->bMoving;MinGap=FMath::Min(MinGap,float(FVector::Dist2D(A->Feet(),G->Feet())));
                    if(Shift>.01f&&FirstShiftAt<0){FirstShiftAt=B->Elapsed-Began;FirstShiftTravel=HelperTravel;FirstShiftPhase=int32(A->WeaponPhase);FirstShiftGunState=int32(Gun->State);}
                }
                const float Pelvis=G->Soldier->GetBoneLocation(TEXT("pelvis")).Z;MinPelvis=FMath::Min(MinPelvis,Pelvis);MaxPelvis=FMath::Max(MaxPelvis,Pelvis);
                SawSectorPack|=Gun->Packs>FirstPack&&Gun->LastPackReasonV8450==TEXT("sector");
                SawNewSupport|=Gun->IsSupported()&&Gun->Deployments>FirstDeploy&&FMath::Abs(FMath::FindDeltaAngleDegrees(InitialFormationYaw,Gun->FormationYawV50()))>120.f;
                Finite&=!G->Feet().ContainsNaN()&&(!A||!A->Feet().ContainsNaN())&&FMath::IsFinite(Pelvis);
                if(I%6==0||Shift>.01f||Gun->State==EFrontlineGunStanceV30::Pack)
                {
                    auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("seconds"),B->Elapsed-Began);R->SetStringField(TEXT("gun_feet"),G->Feet().ToString());
                    R->SetNumberField(TEXT("gun_state"),int32(Gun->State));R->SetNumberField(TEXT("deployments"),Gun->Deployments);R->SetNumberField(TEXT("packs"),Gun->Packs);
                    R->SetStringField(TEXT("pack_reason"),Gun->LastPackReasonV8450);R->SetNumberField(TEXT("formation_yaw"),Gun->FormationYawV50());
                    R->SetNumberField(TEXT("actual_yaw"),G->GetActorRotation().Yaw);R->SetStringField(TEXT("gun_motion"),GetNameSafe(G->Playing));
                    R->SetNumberField(TEXT("gun_weapon_phase"),int32(G->WeaponPhase));R->SetNumberField(TEXT("pelvis_world_z"),Pelvis);
                    R->SetBoolField(TEXT("gun_engaging"),G->bEngaging);R->SetNumberField(TEXT("gun_motion_speed"),G->GetMotionSpeed());
                    if(auto* Target=G->FocusTarget.Get()){R->SetNumberField(TEXT("focus_squad"),Target->SquadId);R->SetBoolField(TEXT("focus_sight"),B->HasSight(G,Target));R->SetBoolField(TEXT("focus_muzzle_clear"),Gun->HasInfantryFiringLine(Target));}
                    if(A){R->SetStringField(TEXT("helper_feet"),A->Feet().ToString());R->SetStringField(TEXT("helper_station"),Station.ToString());R->SetNumberField(TEXT("helper_station_shift_cm"),Shift);
                        R->SetBoolField(TEXT("helper_reform"),Reform);R->SetBoolField(TEXT("helper_moving"),A->bMoving);R->SetNumberField(TEXT("helper_weapon_phase"),int32(A->WeaponPhase));
                        R->SetNumberField(TEXT("helper_motion_speed"),A->GetMotionSpeed());R->SetStringField(TEXT("helper_motion"),GetNameSafe(A->Playing));}
                    Frames.Add(MakeShared<FJsonValueObject>(R));
                }
            }
            const float GunMove=FVector::Dist2D(GunOrigin,G->Feet());const auto* ReloadOwner=Scenario==0?G:A;
            Check(Prefix+TEXT("committed_positions_and_step_bounds_remain_physical"),Finite&&MaxStep<=80.05f&&(!A||MinGap>=55.3f));
            if(Scenario==0)Check(Prefix+TEXT("one_owned_trench_reload_keeps_the_same_placement_without_pack_deploy_cycle"),GunMove<=.01f&&Gun->Packs==FirstPack&&Gun->Deployments==FirstDeploy
                &&Gun->IsSupported()&&Gun->IsTrench()&&ReloadOwner->ReloadsStarted==FirstReloads&&ReloadOwner->ReloadsCompleted>0);
            else
            {
                Check(Prefix+TEXT("normal_tick_performs_one_sector_replacement_without_gun_translation"),GunMove<=.01f&&SawSectorPack&&SawNewSupport&&Gun->Packs==FirstPack+1&&Gun->Deployments==FirstDeploy+1);
                Check(Prefix+TEXT("sector_only_replacement_retains_the_already_accepted_helper_station"),MaxGoalShift<=.01f);
                if(Scenario==1)Check(Prefix+TEXT("settled_helper_stays_planted_during_sector_replacement"),HelperTravel<=1.f&&MovingFlips<=2);
                else Check(Prefix+TEXT("actual_reload_finishes_then_helper_reaches_its_original_station"),ReloadOwner->ReloadsStarted==FirstReloads&&ReloadOwner->ReloadsCompleted>0&&FVector::Dist2D(A->Feet(),OriginalStation)<=64.f);
            }
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("team"),Team);Row->SetNumberField(TEXT("scenario"),Scenario);Row->SetNumberField(TEXT("dt"),DT);
            Row->SetStringField(TEXT("kind"),Scenario==0?TEXT("owned_trench_reload"):Scenario==1?TEXT("settled_helper_sector_turn"):TEXT("reload_paused_transit_helper_sector_turn"));
            Row->SetStringField(TEXT("gun_origin"),GunOrigin.ToString());Row->SetStringField(TEXT("helper_origin"),HelperOrigin.ToString());Row->SetStringField(TEXT("original_helper_station"),OriginalStation.ToString());
            Row->SetNumberField(TEXT("gun_translation_cm"),GunMove);Row->SetNumberField(TEXT("new_packs"),Gun->Packs-FirstPack);Row->SetNumberField(TEXT("new_deployments"),Gun->Deployments-FirstDeploy);
            Row->SetNumberField(TEXT("max_helper_station_shift_cm"),MaxGoalShift);Row->SetNumberField(TEXT("first_station_shift_seconds"),FirstShiftAt);Row->SetNumberField(TEXT("helper_travel_at_first_shift_cm"),FirstShiftTravel);
            Row->SetNumberField(TEXT("helper_phase_at_first_shift"),FirstShiftPhase);Row->SetNumberField(TEXT("gun_state_at_first_shift"),FirstShiftGunState);
            Row->SetNumberField(TEXT("helper_actual_path_cm"),HelperTravel);Row->SetNumberField(TEXT("helper_actual_reverse_cm"),ReverseTravel);Row->SetNumberField(TEXT("helper_moving_flips"),MovingFlips);
            Row->SetNumberField(TEXT("minimum_gun_helper_gap_cm"),A?MinGap:-1.f);Row->SetNumberField(TEXT("max_committed_step_cm"),MaxStep);
            Row->SetNumberField(TEXT("pelvis_world_z_min"),MinPelvis);Row->SetNumberField(TEXT("pelvis_world_z_max"),MaxPelvis);Row->SetArrayField(TEXT("frames"),Frames);Cases.Add(MakeShared<FJsonValueObject>(Row));
        }
        return Finish();
    }
    static TSharedRef<FJsonObject> MachinegunEngagementV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> External)
    {
        auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;int32 QualifiedCases=0,ExecutedCases=0;
        const auto Check=[&](const FString& N,bool Pass){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),N);R->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(R));Good&=Pass;External(N,Pass);};
        FString User;const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
            &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Param(FCommandLine::Get(),TEXT("WFV87MGDeepOnly"))
            &&FrontlineUltimateArchiveV8215::ReviewUserDir(User);
        Check(TEXT("v87_mg_engagement_private_disposable_uk_de_only"),Safe);
        const auto Finish=[&](){J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("cases"),Cases);
            J->SetNumberField(TEXT("pose_specific_6cm_lowering_cases"),QualifiedCases);J->SetNumberField(TEXT("production_contract_cases_executed"),ExecutedCases);
            J->SetStringField(TEXT("scope"),TEXT("Four 30 Hz UK/DE pose-specific H1 diagnostics: two seconds of actual Ready control, then Ammo0/StartReload through real phase completion. One plane lies ahead of every measured muzzle and before the actual target; every interpolation alpha must lie in (0,1). The original 6 cm lowering threshold and 4/2/2 cm visibility-only box are unchanged. If no Reload ray lies more than 6 cm below the entire Ready control envelope, H1 is not reproduced and its production contract test is explicitly NOT executed. No forced Focus/gun state/phase/stance, no assistant/hull or full trench acceptance."));return J;};
        if(!Safe)return Finish();FContext C(B);const auto Random=B->Random;ON_SCOPE_EXIT{B->Random=Random;};
        constexpr float DT=1.f/30.f;
        for(int32 Team:{0,1})for(int32 Handling=0;Handling<2;++Handling)
        {
            C.Clear();B->Random=Random;const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team);
            const int32 Next=FMath::Clamp(Home+Dir,0,B->Trenches.Num()-1),Id=8704000+100*Team+Handling;
            const FString Prefix=FString::Printf(TEXT("v87_mg_engagement_t%d_%s_"),Team,Handling?TEXT("reload"):TEXT("ready"));
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("team"),Team);Row->SetBoolField(TEXT("handling_stimulus"),Handling!=0);Cases.Add(MakeShared<FJsonValueObject>(Row));
            const bool Terrain=B->Trenches.IsValidIndex(Home)&&B->Trenches.IsValidIndex(Next)&&Home!=Next;
            Check(Prefix+TEXT("current_ground_contact_geometry_available"),Terrain);if(!Terrain)continue;
            const FVector Ground((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,0,0);
            FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=EFrontlineRole::Lewis;Q.Slot=1;Q.Trench=INDEX_NONE;Q.Destination=Next;Q.LastPassedTrench=Home;Q.bAdvancing=true;B->Squads.Add(Q);
            auto* G=Make(C,Id,Team,EFrontlineRole::Lewis,0,Ground);if(G)B->FindSquad(Id)->Members.Add(G);
            auto* Front=G?Make(C,Id+1000,1-Team,EFrontlineRole::Rifle,1,G->Feet()+FVector(Dir*1100.f,0,0)):nullptr;
            Check(Prefix+TEXT("actual_actors_and_supported_clips_created"),G&&Front);if(!G||!Front)continue;
            for(int32 I=0;I<180;++I){B->Elapsed+=DT;Tick(G,DT);}
            auto* Gun=FrontlineMachinegunsV30::Get(G);
            const bool Prepared=Gun&&Gun->HasAssets()&&Gun->IsSupported()&&!Gun->IsTrench()
                &&G->WeaponPhase==EFrontlineWeaponPhase::Ready&&G->FocusTarget.Get()==Front&&Gun->HasInfantryFiringLine(Front);
            Check(Prefix+TEXT("real_tick_observes_ready_supported_forward_target"),Prepared);if(!Prepared)continue;
            const FVector OriginalFeet=G->Feet(),TargetPoint=FrontlineMachinegunsV30::SightTarget(Front);const float OriginalYaw=Gun->FormationYawV50();
            struct FSample {FVector Muzzle;float Seconds,Clock,NodeTime;int32 Phase,Ammo,Started,Completed;FString Motion;};
            TArray<FSample> ReadySamples,ReloadSamples;
            const auto Sample=[&](float Seconds){auto* N=G->Soldier->GetSingleNodeInstance();return FSample{G->GetMuzzleLocation(),Seconds,G->FirePoseClock,N?N->GetCurrentTime():-1.f,
                int32(G->WeaponPhase),G->RoundsRemaining,G->ReloadsStarted,G->ReloadsCompleted,GetNameSafe(G->Playing)};};
            for(int32 I=0;I<60;++I){B->Elapsed+=DT;Tick(G,DT);ReadySamples.Add(Sample((I+1)*DT));}
            const int32 BeforeStarted=G->ReloadsStarted,BeforeCompleted=G->ReloadsCompleted;G->RoundsRemaining=0;G->StartReload();
            const bool Started=G->WeaponPhase==EFrontlineWeaponPhase::Reload&&G->RoundsRemaining==0&&G->ReloadsStarted==BeforeStarted+1&&G->FirePoseClock>0;
            Check(Prefix+TEXT("ammo_zero_starts_one_real_reload_with_positive_action_clock"),Started);
            Row->SetNumberField(TEXT("phase_immediately_after_start_reload"),int32(G->WeaponPhase));Row->SetNumberField(TEXT("ammo_immediately_after_start_reload"),G->RoundsRemaining);
            Row->SetNumberField(TEXT("action_clock_immediately_after_start_reload"),G->FirePoseClock);Row->SetBoolField(TEXT("reload_actually_started"),Started);
            int32 ProbeTicks=0;while(G->WeaponPhase==EFrontlineWeaponPhase::Reload&&ProbeTicks<900)
            {B->Elapsed+=DT;Tick(G,DT);ReloadSamples.Add(Sample(++ProbeTicks*DT));}
            const bool Completed=Started&&!ReloadSamples.IsEmpty()&&G->WeaponPhase==EFrontlineWeaponPhase::Ready
                &&G->ReloadsCompleted==BeforeCompleted+1&&G->RoundsRemaining==G->GetMagazineCapacity();
            Check(Prefix+TEXT("real_reload_phase_completes_and_refills_its_actual_magazine"),Completed);Row->SetBoolField(TEXT("reload_actually_completed"),Completed);
            double FurthestMuzzle=-MAX_dbl;for(const auto& S:ReadySamples)FurthestMuzzle=FMath::Max(FurthestMuzzle,S.Muzzle.X*Dir);
            for(const auto& S:ReloadSamples)FurthestMuzzle=FMath::Max(FurthestMuzzle,S.Muzzle.X*Dir);
            const double PlaneX=Dir*(FurthestMuzzle+TargetPoint.X*Dir)*.5;
            const auto Alpha=[&](const FVector& Origin){return (PlaneX-Origin.X)/(TargetPoint.X-Origin.X);};
            const auto AtPlane=[&](const FVector& Origin){return FMath::Lerp(Origin,TargetPoint,Alpha(Origin));};
            const auto Point=[](const FVector& V){auto P=MakeShared<FJsonObject>();P->SetNumberField(TEXT("x"),V.X);P->SetNumberField(TEXT("y"),V.Y);P->SetNumberField(TEXT("z"),V.Z);return P;};
            bool Segments=FMath::IsFinite(PlaneX)&&FurthestMuzzle<TargetPoint.X*Dir;double ReadyMinZ=MAX_dbl,ReadyMaxZ=-MAX_dbl,ReloadMinZ=MAX_dbl,ReloadMaxZ=-MAX_dbl;
            int32 LowestReady=INDEX_NONE,LowestReload=INDEX_NONE;TArray<TSharedPtr<FJsonValue>> ReadyRows,ReloadRows;
            const auto Rows=[&](const TArray<FSample>& Samples,TArray<TSharedPtr<FJsonValue>>& Out,bool IsReload)
            {
                for(int32 I=0;I<Samples.Num();++I)
                {
                    const auto& S=Samples[I];const double A=Alpha(S.Muzzle);const FVector Probe=AtPlane(S.Muzzle);
                    Segments&=FMath::IsFinite(A)&&A>0&&A<1&&!Probe.ContainsNaN();auto F=MakeShared<FJsonObject>();
                    F->SetNumberField(TEXT("seconds"),S.Seconds);F->SetNumberField(TEXT("phase"),S.Phase);F->SetNumberField(TEXT("ammo"),S.Ammo);
                    F->SetNumberField(TEXT("action_clock"),S.Clock);F->SetNumberField(TEXT("node_time"),S.NodeTime);F->SetStringField(TEXT("motion"),S.Motion);
                    F->SetNumberField(TEXT("reloads_started"),S.Started);F->SetNumberField(TEXT("reloads_completed"),S.Completed);
                    F->SetObjectField(TEXT("muzzle"),Point(S.Muzzle));F->SetObjectField(TEXT("segment_probe"),Point(Probe));F->SetNumberField(TEXT("segment_alpha"),A);Out.Add(MakeShared<FJsonValueObject>(F));
                    if(IsReload&&S.Phase==int32(EFrontlineWeaponPhase::Reload))
                    {if(Probe.Z<ReloadMinZ){ReloadMinZ=Probe.Z;LowestReload=I;}ReloadMaxZ=FMath::Max(ReloadMaxZ,Probe.Z);}
                    else if(!IsReload)
                    {if(Probe.Z<ReadyMinZ){ReadyMinZ=Probe.Z;LowestReady=I;}ReadyMaxZ=FMath::Max(ReadyMaxZ,Probe.Z);}
                }
            };
            Rows(ReadySamples,ReadyRows,false);Rows(ReloadSamples,ReloadRows,true);
            Row->SetArrayField(TEXT("ready_pose_timeline"),ReadyRows);Row->SetArrayField(TEXT("reload_pose_timeline"),ReloadRows);
            Row->SetNumberField(TEXT("real_segment_plane_x_cm"),PlaneX);Row->SetObjectField(TEXT("actual_target_point"),Point(TargetPoint));
            Row->SetBoolField(TEXT("all_probe_alphas_inside_actual_ray_segments"),Segments);
            Check(Prefix+TEXT("all_ready_and_reload_probes_lie_strictly_inside_real_muzzle_target_segments"),Segments);
            const bool Measured=Completed&&Segments&&LowestReady!=INDEX_NONE&&LowestReload!=INDEX_NONE;
            Row->SetNumberField(TEXT("ready_probe_z_min_cm"),ReadyMinZ);Row->SetNumberField(TEXT("ready_probe_z_max_cm"),ReadyMaxZ);
            Row->SetNumberField(TEXT("reload_probe_z_min_cm"),ReloadMinZ);Row->SetNumberField(TEXT("reload_probe_z_max_cm"),ReloadMaxZ);
            Row->SetBoolField(TEXT("production_contract_test_executed"),false);if(!Measured)continue;
            const double Lowering=ReadyMinZ-ReloadMinZ;Row->SetNumberField(TEXT("reload_lowering_below_full_ready_envelope_cm"),Lowering);
            const bool Qualified=Lowering>6.;Row->SetBoolField(TEXT("ray_split_qualified"),Qualified);
            Row->SetStringField(TEXT("hypothesis_status"),Qualified?TEXT("pose_specific_6cm_lowering_requires_real_obstacle_test"):
                TEXT("H1_large_lowered_muzzle_not_reproduced_no_obstacle_or_production_test_created"));
            if(!Qualified)continue;
            ++QualifiedCases;
            const FVector ReadyMuzzle=ReadySamples[LowestReady].Muzzle;
            // Completion sets Ready before the displayed reload-to-aim blend
            // settles. The Ready control must evaluate the ordinary aim again.
            for(int32 I=0;I<30;++I){B->Elapsed+=DT;Tick(G,DT);}
            if(Handling)
            {
                G->RoundsRemaining=0;G->StartReload();
                for(int32 I=0;I<=LowestReload&&G->WeaponPhase==EFrontlineWeaponPhase::Reload;++I){B->Elapsed+=DT;Tick(G,DT);}
            }
            const FVector HandlingMuzzle=Handling?G->GetMuzzleLocation():ReloadSamples[LowestReload].Muzzle;
            const FVector ReadyProbe=AtPlane(ReadyMuzzle),HandlingProbe=AtPlane(HandlingMuzzle);
            const bool RepeatSegment=Alpha(HandlingMuzzle)>0&&Alpha(HandlingMuzzle)<1&&ReadyProbe.Z-HandlingProbe.Z>6.;
            Check(Prefix+TEXT("repeat_stimulus_keeps_the_same_6cm_split_inside_actual_ray_segment"),RepeatSegment);if(!RepeatSegment)continue;
            Row->SetStringField(TEXT("ready_muzzle"),ReadyMuzzle.ToString());Row->SetStringField(TEXT("handling_muzzle"),HandlingMuzzle.ToString());
            Row->SetStringField(TEXT("ready_ray_probe"),ReadyProbe.ToString());Row->SetStringField(TEXT("handling_ray_probe"),HandlingProbe.ToString());
            auto* Wall=B->GetWorld()->SpawnActor<AActor>();ON_SCOPE_EXIT{if(IsValid(Wall))Wall->Destroy();};
            auto* Box=Wall?NewObject<UBoxComponent>(Wall):nullptr;
            if(Box){Wall->SetRootComponent(Box);Wall->AddInstanceComponent(Box);Box->SetBoxExtent(FVector(4,2,2));Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                Box->SetCollisionObjectType(ECC_WorldStatic);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
                Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();Wall->SetActorLocation(HandlingProbe);}
            FCollisionQueryParams Sight(SCENE_QUERY_STAT(V87MGHandlingWitness),true);for(AWarfareFrontlineSoldier* U:B->Units)if(U)Sight.AddIgnoredActor(U);
            const auto OldRayClear=[&](){FHitResult Hit;return !B->GetWorld()->LineTraceSingleByChannel(Hit,ReadyMuzzle,FrontlineMachinegunsV30::SightTarget(Front),ECC_Visibility,Sight);};
            const bool SplitWitness=Box&&OldRayClear()&&B->HasSight(G,Front)&&(Handling?!Gun->HasInfantryFiringLine(Front):Gun->HasInfantryFiringLine(Front));
            Check(Prefix+TEXT("same_world_obstacle_blocks_handling_muzzle_but_not_ready_observation"),SplitWitness);if(!SplitWitness)continue;
            auto* Rear=Make(C,Id+2000,1-Team,EFrontlineRole::Rifle,1,OriginalFeet-FRotator(0,OriginalYaw,0).Vector()*1250.f);
            const bool Stimulus=Rear&&Gun->IsSupported()&&G->FocusTarget.Get()==Front&&B->HasSight(G,Front)&&B->HasSight(G,Rear)
                &&Gun->HasInfantryFiringLine(Rear)
                &&FVector::Dist2D(G->Feet(),Rear->Feet())>900.f&&!Gun->CoversYaw((Rear->Feet()-G->Feet()).Rotation().Yaw)
                &&(Handling?G->WeaponPhase==EFrontlineWeaponPhase::Reload:G->WeaponPhase==EFrontlineWeaponPhase::Ready&&Gun->HasInfantryFiringLine(Front));
            Check(Prefix+TEXT("forward_contact_and_nonclose_outside_sector_alternate_are_real"),Stimulus);Row->SetBoolField(TEXT("stimulus_qualified"),Stimulus);if(!Stimulus)continue;
            Row->SetBoolField(TEXT("production_contract_test_executed"),true);
            ++ExecutedCases;
            const int32 FirstPack=Gun->Packs,FirstDeploy=Gun->Deployments,FirstCompleted=G->ReloadsCompleted;
            int32 HandlingFocusChanges=0,TargetChanges=0,ReadyForwardFrames=0,HandlingFrames=0;bool LastFront=true,ForwardObserved=true,Finite=true;
            float Travel=0,MaxStep=0;const float Began=B->Elapsed;TArray<TSharedPtr<FJsonValue>> Frames;
            for(int32 I=0;I<360;++I)
            {
                B->Elapsed+=DT;const FVector Before=G->Feet();Tick(G,DT);const bool IsFront=G->FocusTarget.Get()==Front;
                if(IsFront!=LastFront){++TargetChanges;LastFront=IsFront;}
                if(G->WeaponPhase==EFrontlineWeaponPhase::Reload){++HandlingFrames;HandlingFocusChanges+=!IsFront;}
                if(G->WeaponPhase==EFrontlineWeaponPhase::Ready&&Gun->HasInfantryFiringLine(Front))++ReadyForwardFrames;
                ForwardObserved&=B->HasSight(G,Front)&&OldRayClear()&&FrontlineMachinegunsV30::ValidContactV61(G,Front);
                const float Moved=FVector::Dist2D(Before,G->Feet());Travel+=Moved;MaxStep=FMath::Max(MaxStep,Moved);
                Finite&=!G->Feet().ContainsNaN()&&FMath::IsFinite(G->Soldier->GetBoneLocation(TEXT("pelvis")).Z);
                if(I%3==0||!IsFront||Gun->State!=EFrontlineGunStanceV30::Supported)
                {
                    auto F=Gun->DescribeTrenchFiringSectorV844();F->SetNumberField(TEXT("seconds"),B->Elapsed-Began);F->SetBoolField(TEXT("focus_is_original_forward"),IsFront);
                    F->SetBoolField(TEXT("original_ready_ray_clear"),OldRayClear());F->SetBoolField(TEXT("forward_observation_clear"),B->HasSight(G,Front));
                    F->SetBoolField(TEXT("forward_current_muzzle_clear"),Gun->HasInfantryFiringLine(Front));F->SetBoolField(TEXT("alternate_current_muzzle_clear"),Gun->HasInfantryFiringLine(Rear));
                    F->SetNumberField(TEXT("forward_score"),FrontlineMachinegunsV30::TargetScoreV828(G,Front));F->SetNumberField(TEXT("alternate_score"),FrontlineMachinegunsV30::TargetScoreV828(G,Rear));
                    F->SetNumberField(TEXT("gun_state"),int32(Gun->State));F->SetNumberField(TEXT("packs"),Gun->Packs);F->SetNumberField(TEXT("deployments"),Gun->Deployments);
                    F->SetStringField(TEXT("feet"),G->Feet().ToString());F->SetStringField(TEXT("current_muzzle"),G->GetMuzzleLocation().ToString());Frames.Add(MakeShared<FJsonValueObject>(F));
                }
            }
            Check(Prefix+TEXT("original_forward_contact_remains_observable_and_motion_is_physical"),ForwardObserved&&Finite&&MaxStep<=80.05f);
            if(Handling)Check(Prefix+TEXT("lowered_feed_muzzle_does_not_retarget_or_redeploy_an_observed_working_sector"),HandlingFrames>0&&HandlingFocusChanges==0
                &&G->ReloadsCompleted>FirstCompleted&&Gun->Packs==FirstPack&&Gun->Deployments==FirstDeploy);
            else Check(Prefix+TEXT("ready_control_keeps_its_same_actual_clear_forward_sector"),TargetChanges==0&&ReadyForwardFrames>=300&&Gun->Packs==FirstPack&&Gun->Deployments==FirstDeploy);
            Row->SetNumberField(TEXT("handling_frames_with_original_focus_lost"),HandlingFocusChanges);Row->SetNumberField(TEXT("handling_frames"),HandlingFrames);
            Row->SetNumberField(TEXT("target_switches"),TargetChanges);Row->SetNumberField(TEXT("new_packs"),Gun->Packs-FirstPack);Row->SetNumberField(TEXT("new_deployments"),Gun->Deployments-FirstDeploy);
            Row->SetNumberField(TEXT("actual_path_cm"),Travel);Row->SetNumberField(TEXT("max_committed_step_cm"),MaxStep);Row->SetArrayField(TEXT("frames"),Frames);
        }
        return Finish();
    }
    static TSharedRef<FJsonObject> MachinegunLeaseV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> External)
    {
        auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
        const auto Check=[&](const FString& N,bool Pass){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),N);R->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(R));Good&=Pass;External(N,Pass);};
        const auto Finish=[&](){J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("cases"),Cases);
            J->SetStringField(TEXT("scope"),TEXT("Two Editor/NoSave/disposable diagnostics, one held Lewis section and one same-team Whippet per actual faction direction. Original starting point, natural lane decisions and all physical guards retained. Track the first non-gunner with an actual friendly yield request, then trigger that actor's actual Ammo0/Reload. No observed Reload+lease competition is explicitly not_exercised, not a production pass/failure. Tank passage, lease acquisition/release and Reload overlap are reported separately. Generic/physical goals are service readbacks; live leases are pure map receipts; selected goal is after-Tick lease/support inference, not an internal Tick local. No H1 obstacle, full historical matrix, player writes or broad motion/visual acceptance."));return J;};
#if !WITH_EDITOR
        Check(TEXT("v87_mg_lease_editor_only"),false);return Finish();
#else
        FString User;auto* Fleet=B?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;
        const bool Safe=B&&B->GetWorld()&&Fleet&&IsInGameThread()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV87MGLeaseOnly"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FrontlineUltimateArchiveV8215::ReviewUserDir(User);
        Check(TEXT("v87_mg_lease_private_disposable_uk_de_only"),Safe);if(!Safe)return Finish();
        FContext C(B);const auto Random=B->Random;ON_SCOPE_EXIT{B->Random=Random;};
        // The accessor exposes an actual nonconst subsystem-owned array. This
        // synchronous review saves/restores that exact array; no real tank is simulated.
        auto& Live=const_cast<TArray<TObjectPtr<AFrontlineTank>>&>(Fleet->GetVehicles());const auto OriginalVehicles=Live;
        struct FHidden {TWeakObjectPtr<AFrontlineTank> Tank;bool Hidden,Collision;};TArray<FHidden> Hidden;
        for(AFrontlineTank* V:OriginalVehicles)if(IsValid(V)){Hidden.Add({V,V->IsHidden(),V->GetActorEnableCollision()});V->SetActorHiddenInGame(true);V->SetActorEnableCollision(false);}
        Live.Reset();TArray<AFrontlineTank*> MadeTanks;
        const auto ClearTanks=[&](){for(auto* V:MadeTanks)if(IsValid(V))V->Destroy();MadeTanks.Reset();Live.Reset();FrontlineMobilityV41::Reset(B);};
        ON_SCOPE_EXIT{ClearTanks();Live=OriginalVehicles;for(const auto& H:Hidden)if(auto* V=H.Tank.Get()){V->SetActorHiddenInGame(H.Hidden);V->SetActorEnableCollision(H.Collision);}
            Check(TEXT("v87_mg_lease_original_vehicle_roster_restored"),Live==OriginalVehicles);Finish();};
        const auto Point=[](const FVector& V){auto P=MakeShared<FJsonObject>();P->SetNumberField(TEXT("x"),V.X);P->SetNumberField(TEXT("y"),V.Y);P->SetNumberField(TEXT("z"),V.Z);return P;};
        const auto LeasePoint=[](const TSharedPtr<FJsonObject>& L,const FVector& Fallback){if(!L||!L->GetBoolField(TEXT("lease_present")))return Fallback;
            const auto P=L->GetObjectField(TEXT("destination"));return FVector(P->GetNumberField(TEXT("x")),P->GetNumberField(TEXT("y")),P->GetNumberField(TEXT("z")));};
        constexpr float DT=1.f/30.f;
        for(int32 Team:{0,1})
        {
            ClearTanks();C.Clear();B->Random=Random;const int32 Dir=B->GetTeamDirection(Team),Trench=B->Trenches.Num()/2,Id=8705000+Team*100;
            const FString Prefix=FString::Printf(TEXT("v87_mg_lease_t%d_"),Team);auto Row=MakeShared<FJsonObject>();Cases.Add(MakeShared<FJsonValueObject>(Row));
            Row->SetNumberField(TEXT("team"),Team);Row->SetNumberField(TEXT("direction"),Dir);Row->SetStringField(TEXT("faction"),B->GetFactionForTeam(Team)==EFrontlineFaction::British?TEXT("British"):TEXT("German"));
            Check(Prefix+TEXT("actual_shared_trench_and_faction_direction"),B->Trenches.IsValidIndex(Trench)&&(Dir==1||Dir==-1));if(!B->Trenches.IsValidIndex(Trench))continue;
            B->Trenches[Trench].Owner=Team;FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=EFrontlineRole::Lewis;Q.Slot=1;
            Q.Trench=Q.Destination=Q.LastPassedTrench=Trench;Q.bHoldPosition=true;B->Squads.Add(Q);TArray<AWarfareFrontlineSoldier*> Crew;
            for(int32 Member=0;Member<B->GetSquadSize(EFrontlineRole::Lewis,Team);++Member)
                if(auto* U=Make(C,Id,Team,EFrontlineRole::Lewis,Member,FVector(FrontlineFieldV32::TrenchCentreX(Trench,0),0,0)))
                {Crew.Add(U);B->FindSquad(Id)->Members.Add(U);}
            for(auto* U:Crew){FVector P=B->SoldierDestination(U);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));Pose(U,0);}
            auto* G=Crew.Num()>0?Crew[0]:nullptr;auto* A=Crew.Num()>1?Crew[1]:nullptr;
            auto* Enemy=G?Make(C,Id+1000,1-Team,EFrontlineRole::Rifle,1,G->Feet()+FVector(Dir*1100.f,0,0)):nullptr;
            for(int32 I=0;I<180;++I){B->Elapsed+=DT;B->UpdateSquads();for(auto* U:Crew)Tick(U,DT);}
            auto* Gun=FrontlineMachinegunsV30::Get(G);
            const bool Prepared=G&&A&&Enemy&&Crew.Num()==B->GetSquadSize(EFrontlineRole::Lewis,Team)&&Gun&&Gun->HasAssets()&&Gun->IsSupported()&&Gun->IsTrench();
            Check(Prefix+TEXT("actual_held_crew_reaches_trench_supported_state"),Prepared);if(!Prepared)continue;
            bool Reform=false;const FVector GenericStart=B->SoldierDestination(G),ActualStart=FrontlineMachinegunsV30::Destination(G,GenericStart,Reform);
            Row->SetObjectField(TEXT("generic_gunner_bay_at_start"),Point(GenericStart));Row->SetObjectField(TEXT("physical_gunner_bay_at_start"),Point(ActualStart));
            Row->SetNumberField(TEXT("initial_generic_vs_physical_gunner_gap_cm"),FVector::Dist2D(GenericStart,ActualStart));
            const float Lane=A->Feet().Y;const float Bank=FrontlineFieldV32::TrenchCentreX(Trench,Lane);
            const FVector TankStart(Bank-Dir*900.f,Lane,0);auto* V=B->GetWorld()->SpawnActor<AFrontlineTank>(TankStart,FRotator(0,Dir>0?0.f:180.f,0));
            const bool Configured=V&&V->Configure(B,EFrontlineVehicle::Whippet,Team,Id+5000);
            if(V){MadeTanks.Add(V);V->SetActorTickEnabled(false);}
            Check(Prefix+TEXT("actual_same_team_whippet_configures_with_stock_assets"),Configured&&V->AreAssetsReady());if(!Configured)continue;
            V->SetActorTransform(V->GroundPoseForContactV8218(TankStart,Dir>0?0.f:180.f,0));Live.Add(V);
            for(int32 I=0;I<V->GetMachinegunCount();++I)V->ReceiveModuleHit(EFrontlineTankModule::Machinegun,100000.f,I);
            bool GunsDisabled=true;for(int32 I=0;I<V->GetMachinegunCount();++I)GunsDisabled&=V->GetMachinegunIntegrity(I)<=0;
            Check(Prefix+TEXT("actual_module_hits_disable_only_mg_and_preserve_operational_running_gear"),GunsDisabled&&V->IsOperational()&&!V->IsImmobilized());
            const auto InitialReceipt=FrontlineMobilityV41::DescribeArmourYieldLeaseV87(A);
            Check(Prefix+TEXT("pure_lease_receipt_is_enabled_only_in_this_private_mode"),InitialReceipt.IsValid());
            bool ReloadStarted=false,LeaseSeen=false,ReloadLease=false,Finite=true,Placement=true;int32 LeaseAcquisitions=0,LeaseReleases=0,HullContacts=0,NextContacts=0;
            int32 MovingFlips=0,GoalChanges=0;float HelperTravel=0,HelperReverse=0,MaxStep=0,MaxGenericGap=0,FirstReloadAt=-1,FirstLeaseAt=-1,FirstReleaseAt=-1;
            const int32 PacksBefore=Gun->Packs,DeploysBefore=Gun->Deployments;int32 ReloadsBefore=0;AWarfareFrontlineSoldier* Tracked=nullptr;
            bool LastLeased=false,LastMoving=false;FVector LastSelected=FVector::ZeroVector;TArray<TSharedPtr<FJsonValue>> Frames;
            float MaxGoalJump=0,PendingNoProgress=0,MaxPendingNoProgress=0,ReadyNoProgress=0,MaxReadyNoProgress=0;
            const float Began=B->Elapsed;const FVector FirstTank=V->GetActorLocation();int32 ExecutedTicks=0;
            for(int32 I=0;I<900;++I)
            {
                ++ExecutedTicks;B->Elapsed+=DT;B->UpdateSquads();
                if(!Tracked)for(auto* U:Crew)if(!U->IsMachineGunner())
                {
                    FVector RequestedYield=FVector::ZeroVector;if(!V->InfantryYieldPointV8218(U,RequestedYield))continue;
                    Tracked=U;ReloadsBefore=U->ReloadsCompleted;LastMoving=U->bMoving;LastSelected=U->Feet();
                    U->RoundsRemaining=0;U->StartReload();ReloadStarted=U->WeaponPhase==EFrontlineWeaponPhase::Reload;FirstReloadAt=B->Elapsed-Began;
                    Row->SetNumberField(TEXT("tracked_helper_member"),U->MemberIndex);Row->SetObjectField(TEXT("tracked_helper_feet_at_request"),Point(U->Feet()));
                    Row->SetObjectField(TEXT("actual_reload_trigger_yield_request"),Point(RequestedYield));break;
                }
                auto Frame=MakeShared<FJsonObject>();Frame->SetNumberField(TEXT("seconds"),B->Elapsed-Began);Frame->SetObjectField(TEXT("tank_before_infantry"),Point(V->GetActorLocation()));
                Frame->SetNumberField(TEXT("tank_rear_before_infantry_projected_cm"),V->RearProgressV49());TArray<TSharedPtr<FJsonValue>> People;
                for(auto* U:Crew)
                {
                    const FVector Before=U->Feet(),Generic=B->SoldierDestination(U);bool LocalReform=false;
                    const FVector Physical=FrontlineMachinegunsV30::Destination(U,Generic,LocalReform);const auto LeaseBefore=FrontlineMobilityV41::DescribeArmourYieldLeaseV87(U);
                    Tick(U,DT);const auto LeaseAfter=FrontlineMobilityV41::DescribeArmourYieldLeaseV87(U);
                    const FVector CurrentGeneric=B->SoldierDestination(U);bool AfterReform=false;const FVector CurrentPhysical=FrontlineMachinegunsV30::Destination(U,CurrentGeneric,AfterReform);
                    const FVector Selected=LeasePoint(LeaseAfter,CurrentPhysical);const FVector Committed=U->Feet()-Before;const float Step=Committed.Size2D();MaxStep=FMath::Max(MaxStep,Step);
                    const bool Allowed=FrontlineMobilityV41::AllowsSupportedPlacementV8448(U,Before,U->Feet());Placement&=Allowed;Finite&=!U->Feet().ContainsNaN();
                    auto P=MakeShared<FJsonObject>();P->SetNumberField(TEXT("member"),U->MemberIndex);P->SetObjectField(TEXT("feet_before"),Point(Before));P->SetObjectField(TEXT("feet_after"),Point(U->Feet()));
                    P->SetObjectField(TEXT("generic_formation_goal_before"),Point(Generic));P->SetObjectField(TEXT("physical_support_goal_before"),Point(Physical));
                    P->SetObjectField(TEXT("generic_formation_goal_after"),Point(CurrentGeneric));P->SetObjectField(TEXT("physical_support_goal_after"),Point(CurrentPhysical));
                    P->SetObjectField(TEXT("selected_goal_from_after_tick_lease_or_support"),Point(Selected));P->SetStringField(TEXT("selected_goal_scope"),TEXT("Inferred from the actual after-Tick existing lease, otherwise the cached physical support service; not an instrumented internal Soldier Tick local."));
                    if(LeaseBefore.IsValid())P->SetObjectField(TEXT("lease_before_tick"),LeaseBefore.ToSharedRef());if(LeaseAfter.IsValid())P->SetObjectField(TEXT("lease_after_tick"),LeaseAfter.ToSharedRef());
                    P->SetBoolField(TEXT("actual_supported_placement_allowed"),Allowed);P->SetNumberField(TEXT("committed_xy_travel_cm"),Step);P->SetNumberField(TEXT("committed_forward_cm"),Committed.X*Dir);
                    P->SetStringField(TEXT("placement_check_scope"),TEXT("Fresh production placement-gate recheck after the evaluated pose, against the tank before its next Simulate; not the original Tick's local decision."));
                    P->SetNumberField(TEXT("requested_speed_after_tick"),U->RequestedTravelSpeedV43);P->SetNumberField(TEXT("actual_motion_speed"),U->GetMotionSpeed());P->SetBoolField(TEXT("moving"),U->bMoving);
                    P->SetNumberField(TEXT("weapon_phase"),int32(U->WeaponPhase));P->SetNumberField(TEXT("reloads_started"),U->ReloadsStarted);P->SetNumberField(TEXT("reloads_completed"),U->ReloadsCompleted);
                    P->SetStringField(TEXT("motion"),GetNameSafe(U->Playing));P->SetBoolField(TEXT("current_hull_contact_before_tank_step"),V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation(),V->GetActorRotation().Yaw,34.f,0.f,U));
                    P->SetBoolField(TEXT("next_tank_step_clear"),V->IsInfantryYieldNextStepClearV86(U,U->Feet()));People.Add(MakeShared<FJsonValueObject>(P));
                    MaxGenericGap=FMath::Max(MaxGenericGap,float(FVector::Dist2D(CurrentGeneric,CurrentPhysical)));
                    if(U==Tracked)
                    {
                        HelperTravel+=Step;HelperReverse+=FMath::Max(0.f,-float(Committed.X*Dir));if(U->bMoving!=LastMoving)++MovingFlips;LastMoving=U->bMoving;
                        const bool Leased=LeaseAfter.IsValid()&&LeaseAfter->GetBoolField(TEXT("lease_present"));LeaseSeen|=Leased;ReloadLease|=Leased&&U->WeaponPhase==EFrontlineWeaponPhase::Reload;
                        if(Leased&&!LastLeased){++LeaseAcquisitions;if(FirstLeaseAt<0)FirstLeaseAt=B->Elapsed-Began;}
                        if(!Leased&&LastLeased){++LeaseReleases;if(FirstReleaseAt<0)FirstReleaseAt=B->Elapsed-Began;}
                        LastLeased=Leased;const float Jump=FVector::Dist2D(Selected,LastSelected);MaxGoalJump=FMath::Max(MaxGoalJump,Jump);if(Jump>1.f)++GoalChanges;LastSelected=Selected;
                        const bool Pending=FVector::Dist2D(U->Feet(),Selected)>3.f&&Step<.05f;PendingNoProgress=Pending?PendingNoProgress+DT:0.f;
                        ReadyNoProgress=Pending&&U->WeaponPhase==EFrontlineWeaponPhase::Ready?ReadyNoProgress+DT:0.f;
                        MaxPendingNoProgress=FMath::Max(MaxPendingNoProgress,PendingNoProgress);MaxReadyNoProgress=FMath::Max(MaxReadyNoProgress,ReadyNoProgress);
                    }
                }
                Frame->SetArrayField(TEXT("crew"),People);Frame->SetObjectField(TEXT("gun_state"),Gun->DescribeTrenchFiringSectorV844());V->Simulate(DT);
                Frame->SetObjectField(TEXT("tank_after_simulate"),Point(V->GetActorLocation()));Frame->SetObjectField(TEXT("tank_mobility"),V->DescribeMobilityV839());
                for(auto* U:Crew){HullContacts+=V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation(),V->GetActorRotation().Yaw,34.f,0.f,U);NextContacts+=!V->IsInfantryYieldNextStepClearV86(U,U->Feet());}
                Finite&=!V->GetActorTransform().ContainsNaN();Frames.Add(MakeShared<FJsonValueObject>(Frame));
                const bool Returned=!Tracked||(Tracked->WeaponPhase==EFrontlineWeaponPhase::Ready&&!Tracked->bMoving
                    &&FVector::Dist2D(Tracked->Feet(),B->SoldierDestination(Tracked))<=10.f);
                if(I>180&&V->RearProgressV49()>ActualStart.X*Dir+500.f&&!LastLeased&&Returned)break;
            }
            const float TankForward=(V->GetActorLocation().X-FirstTank.X)*Dir;
            Check(Prefix+TEXT("actual_supported_steps_and_current_hull_remain_physical"),Finite&&Placement&&HullContacts==0&&MaxStep<=80.05f);
            const bool TankPassed=TankForward>1000.f&&V->RearProgressV49()>ActualStart.X*Dir+300.f;
            Check(Prefix+TEXT("friendly_hull_actually_passes_the_held_section"),TankPassed);
            const bool Exercised=Tracked&&ReloadStarted&&LeaseSeen&&ReloadLease;
            Row->SetBoolField(TEXT("tank_actual_passed"),TankPassed);Row->SetBoolField(TEXT("lease_acquired"),LeaseSeen);Row->SetBoolField(TEXT("lease_released"),LeaseReleases>0);
            Row->SetBoolField(TEXT("reload_overlap"),ReloadLease);Row->SetBoolField(TEXT("hypothesis_contract_checked"),Exercised);
            Row->SetStringField(TEXT("hypothesis_status"),Exercised?TEXT("actual_helper_reload_and_lease_competition_observed"):
                Tracked?TEXT("not_exercised_actual_request_did_not_produce_reload_lease_overlap"):TEXT("not_exercised_no_nongunner_friendly_yield_request"));
            if(!Tracked)Row->SetNumberField(TEXT("tracked_helper_member"),INDEX_NONE);
            if(Exercised)
            {
                Check(Prefix+TEXT("tracked_helper_actual_reload_completes"),Tracked->ReloadsCompleted>ReloadsBefore);
                Check(Prefix+TEXT("tracked_helper_lease_releases_after_actual_hull_passage"),LeaseReleases>0);
                const FVector ReturnGoal=B->SoldierDestination(Tracked);bool ReturnReform=false;const FVector PhysicalReturn=FrontlineMachinegunsV30::Destination(Tracked,ReturnGoal,ReturnReform);
                Row->SetObjectField(TEXT("tracked_helper_actual_return_goal"),Point(ReturnGoal));Row->SetObjectField(TEXT("tracked_helper_physical_return_goal"),Point(PhysicalReturn));
                Row->SetObjectField(TEXT("tracked_helper_final_feet"),Point(Tracked->Feet()));Row->SetNumberField(TEXT("tracked_helper_return_error_cm"),FVector::Dist2D(Tracked->Feet(),ReturnGoal));
                Check(Prefix+TEXT("tracked_helper_reaches_its_current_real_return_bay"),!Tracked->bMoving&&FVector::Dist2D(Tracked->Feet(),ReturnGoal)<=10.f);
            }
            Row->SetNumberField(TEXT("executed_ticks"),ExecutedTicks);Row->SetNumberField(TEXT("helper_actual_path_cm"),HelperTravel);Row->SetNumberField(TEXT("helper_actual_reverse_cm"),HelperReverse);
            Row->SetNumberField(TEXT("max_committed_step_cm"),MaxStep);Row->SetNumberField(TEXT("helper_moving_flips"),MovingFlips);Row->SetNumberField(TEXT("helper_goal_changes"),GoalChanges);
            Row->SetNumberField(TEXT("helper_lease_acquisitions"),LeaseAcquisitions);Row->SetNumberField(TEXT("helper_lease_releases"),LeaseReleases);Row->SetNumberField(TEXT("first_helper_reload_seconds"),FirstReloadAt);
            Row->SetNumberField(TEXT("helper_max_goal_jump_cm"),MaxGoalJump);Row->SetNumberField(TEXT("helper_max_pending_goal_zero_progress_seconds"),MaxPendingNoProgress);
            Row->SetNumberField(TEXT("helper_max_ready_pending_goal_zero_progress_seconds"),MaxReadyNoProgress);
            Row->SetNumberField(TEXT("first_helper_lease_seconds"),FirstLeaseAt);Row->SetNumberField(TEXT("first_helper_release_seconds"),FirstReleaseAt);Row->SetNumberField(TEXT("maximum_generic_vs_physical_bay_gap_cm"),MaxGenericGap);
            Row->SetNumberField(TEXT("current_hull_contact_frames"),HullContacts);Row->SetNumberField(TEXT("next_tank_step_blocked_frames"),NextContacts);Row->SetNumberField(TEXT("tank_actual_forward_cm"),TankForward);
            Row->SetNumberField(TEXT("gun_new_packs"),Gun->Packs-PacksBefore);Row->SetNumberField(TEXT("gun_new_deployments"),Gun->Deployments-DeploysBefore);Row->SetArrayField(TEXT("frames"),Frames);
        }
        return Finish();
#endif
    }
    static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> External)
    {
        auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases,Reloads;bool Good=true;
        const auto Check=[&](const FString& N,bool Pass){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),N);R->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(R));Good&=Pass;External(N,Pass);};
        Check(TEXT("v8447_motion_disposable_uk_de_only"),Allowed(B));
        if(!Allowed(B)){J->SetBoolField(TEXT("success"),false);J->SetArrayField(TEXT("checks"),Checks);return J;}
        FContext C(B);bool LegacyWitness=false,MountedArrivalOffsetWitness=false;
        J->SetObjectField(TEXT("existing_contact_v87"),ExistingContactV87(B,Check));
        // The previous BackStep branch excludes ground crews while their
        // Watch-facing branch always includes them. Exercise a real 50 cm
        // rear-side station adjustment through normal Tick at three rates.
        for(int32 Team:{0,1})for(float DT:{1.f/120.f,1.f/60.f,.1f})
        {
            C.Clear();const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team),Next=FMath::Clamp(Home+Dir,0,B->Trenches.Num()-1);
            const int32 Id=8447000+Team*100+FMath::RoundToInt(DT*1000);const FVector P((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,0,0);
            auto* G=Make(C,Id,Team,EFrontlineRole::Lewis,0,P);
            auto* E=Make(C,Id+500,1-Team,EFrontlineRole::Rifle,0,P+FVector(Dir*650.f,0,0));
            FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=EFrontlineRole::Lewis;Q.Trench=INDEX_NONE;Q.Destination=Next;Q.bAdvancing=true;Q.Members.Add(G);B->Squads.Add(Q);
            const FString Prefix=FString::Printf(TEXT("v8447_escort_t%d_hz%d_"),Team,FMath::RoundToInt(1.f/DT));
            Check(Prefix+TEXT("real_supported_actors_spawned"),G&&E);if(!G||!E)continue;
            for(int32 I=0;I<120;++I){B->Elapsed+=.05f;Tick(G,.05f);}
            auto* Gun=FrontlineMachinegunsV30::Get(G);Check(Prefix+TEXT("normal_tick_deploys_on_real_ground_contact"),Gun&&Gun->IsSupported()&&!Gun->IsTrench());
            if(!Gun||!Gun->IsSupported()||Gun->IsTrench())continue;
            const FVector Front=FRotator(0,Gun->FormationYawV50(),0).Vector(),Right=FVector::CrossProduct(FVector::UpVector,Front);
            auto* A=Make(C,Id,Team,EFrontlineRole::Lewis,1,G->Feet()+Front*14.f+Right*125.f);if(!A){Check(Prefix+TEXT("assistant_created"),false);continue;}
            // Match the production station's deterministic personal side offset.
            FVector Start=G->Feet()+Front*14.f+Right*(120.f+A->GetMotionPhase()*10.f);Start.Z=FrontlineInfantryV45::SurfaceHeight(B,Start);A->SetActorLocation(Start+FVector(0,0,88));
            B->FindSquad(Id)->Members.Add(A);A->bAtDestinationV43=false;
            bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(A,B->SoldierDestination(A),Reform);
            const bool LegacyBranch=FrontlineMachinegunsV30::HasGroundStationV828(A)&&Reform&&FVector::Dist2D(Start,Goal)>32.f
                &&FVector::Dist2D(Start,Goal)<=64.f&&FVector::DotProduct((Goal-Start).GetSafeNormal2D(),Front)<-.5f;
            float MinFacing=1.f,RearTravel=0,LateTravel=0,MaxIntrusion=0,MinBodyGap=MAX_flt;int32 NegativeRates=0,RunFrames=0,MoveFlips=0;bool WasMoving=false,Finite=true;
            TArray<TSharedPtr<FJsonValue>> Samples;
            const int32 Steps=FMath::CeilToInt(5.f/DT);
            for(int32 I=0;I<Steps;++I)
            {
                B->Elapsed+=DT;const FVector Before=A->Feet();Tick(A,DT);const FVector Travel=A->Feet()-Before;const float Speed=Travel.Size2D()/DT;
                Finite&=!A->Feet().ContainsNaN()&&FMath::IsFinite(Speed)&&FMath::IsFinite(A->GetActorRotation().Yaw);
                if(Speed>8.f){MinFacing=FMath::Min(MinFacing,float(FVector::DotProduct(Travel.GetSafeNormal2D(),A->GetActorForwardVector())));RearTravel+=FMath::Max(0.f,-float(FVector::DotProduct(Travel,Front)));}
                if(auto* Node=A->Soldier->GetSingleNodeInstance())if(Node->GetPlayRate()<0.f)++NegativeRates;
                if(A->Playing==A->GetRunMotion())++RunFrames;
                if(I*DT>=3.f){LateTravel+=Travel.Size2D();if(A->bMoving!=WasMoving)++MoveFlips;}
                WasMoving=A->bMoving;MaxIntrusion=FMath::Max(MaxIntrusion,FrontlineInfantryV45::BankIntrusion(A,A->Feet()));MinBodyGap=FMath::Min(MinBodyGap,float(FVector::Dist2D(A->Feet(),G->Feet())));
                if(I<12||Speed>8.f){auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("time"),(I+1)*DT);R->SetStringField(TEXT("feet"),A->Feet().ToString());R->SetNumberField(TEXT("yaw"),A->GetActorRotation().Yaw);R->SetNumberField(TEXT("motion_speed"),A->GetMotionSpeed());R->SetNumberField(TEXT("weapon_phase"),int32(A->WeaponPhase));Samples.Add(MakeShared<FJsonValueObject>(R));}
            }
            bool FinalReform=false;const FVector FinalGoal=FrontlineMachinegunsV30::Destination(A,B->SoldierDestination(A),FinalReform);
            const float Error=FVector::Dist2D(A->Feet(),FinalGoal);LegacyWitness|=LegacyBranch&&RearTravel>1.f;
            Check(Prefix+TEXT("actual_local_rear_leg_faces_accepted_travel"),Finite&&MinFacing>=.49f&&NegativeRates==0&&RunFrames==0);
            Check(Prefix+TEXT("legal_station_reaches_and_stays_planted"),Error<=64.f&&LateTravel<=1.f&&MoveFlips<=2);
            Check(Prefix+TEXT("same_physical_station_keeps_body_and_bank_clearance"),MinBodyGap>=55.4f&&MaxIntrusion<=8.1f&&FVector::Dist2D(FinalGoal,Goal)<=.01f);
            auto* Far=Make(C,Id,Team,EFrontlineRole::Lewis,2,G->Feet()+Front*600.f-Right*180.f);
            bool FarSafe=Far!=nullptr;float FarReverse=0.f;
            if(Far)
            {
                B->FindSquad(Id)->Members.Add(Far);const FVector FarStart=Far->Feet();
                bool FarReform=false;const FVector FarGoal=FrontlineMachinegunsV30::Destination(Far,B->SoldierDestination(Far),FarReform);
                FarSafe&=FVector::Dist2D(FarStart,FarGoal)<=.01f&&!FarReform;
                for(int32 I=0;I<FMath::CeilToInt(2.f/DT);++I)
                {B->Elapsed+=DT;const FVector Before=Far->Feet();Tick(Far,DT);FarReverse+=FMath::Max(0.f,-float(FVector::DotProduct(Far->Feet()-Before,Front)));FarSafe&=!Far->Feet().ContainsNaN();}
            }
            Check(Prefix+TEXT("assistant_beyond_support_area_never_marches_back_to_old_gun"),FarSafe&&FarReverse<=.01f);
            B->Units.Remove(E);E->Destroy();FrontlineMobilityV41::Reset(B);
            const FVector MarchStart=G->Feet();bool MarchForward=true;float MinimumMarchFacing=1.f;
            for(int32 I=0;I<FMath::CeilToInt(5.f/DT);++I)
            {
                B->Elapsed+=DT;
                for(auto* U:{G,A})
                {
                    const FVector Before=U->Feet();const bool Local=FrontlineMachinegunsV30::HasGroundStationV828(U);Tick(U,DT);const FVector Travel=U->Feet()-Before;
                    if(!Local)MarchForward&=Travel.X*Dir>=-.01f;
                    if(Travel.Size2D()/DT>8.f)MinimumMarchFacing=FMath::Min(MinimumMarchFacing,float(FVector::DotProduct(Travel.GetSafeNormal2D(),U->GetActorForwardVector())));
                }
            }
            Check(Prefix+TEXT("contact_loss_resumes_forward_march_with_forward_stride"),MarchForward&&MinimumMarchFacing>=.49f&&(G->Feet().X-MarchStart.X)*Dir>100.f);
            auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("team"),Team);R->SetNumberField(TEXT("dt"),DT);R->SetStringField(TEXT("start"),Start.ToString());R->SetStringField(TEXT("owned_station"),Goal.ToString());R->SetStringField(TEXT("end"),A->Feet().ToString());R->SetBoolField(TEXT("old_ground_watch_forward_loop_condition"),LegacyBranch);R->SetNumberField(TEXT("actual_rearward_station_travel_cm"),RearTravel);R->SetNumberField(TEXT("minimum_travel_body_facing_dot"),MinFacing);R->SetNumberField(TEXT("negative_clip_rate_frames"),NegativeRates);R->SetNumberField(TEXT("running_clip_frames"),RunFrames);R->SetNumberField(TEXT("last_two_second_travel_cm"),LateTravel);R->SetNumberField(TEXT("final_station_error_cm"),Error);R->SetNumberField(TEXT("far_helper_reverse_travel_cm"),FarReverse);R->SetNumberField(TEXT("minimum_march_body_facing_dot"),MinimumMarchFacing);R->SetNumberField(TEXT("contact_loss_forward_progress_cm"),(G->Feet().X-MarchStart.X)*Dir);R->SetArrayField(TEXT("tick_samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(R));
        }
        Check(TEXT("v8447_actual_ground_station_exercises_old_backslide_branch"),LegacyWitness);
        for(int32 Team:{0,1})for(EFrontlineRole Role:{EFrontlineRole::Rifle,EFrontlineRole::Lewis})
        {
            C.Clear();auto Men=Held(C,8447900+Team*10+int32(Role),Team,Role);const FString Prefix=FString::Printf(TEXT("v8447_reload_t%d_role%d_"),Team,int32(Role));
            Check(Prefix+TEXT("real_held_section_arrived"),!Men.IsEmpty());if(Men.IsEmpty())continue;
            auto* U=Men[0];const FVector Feet=U->Feet();bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);
            TSharedPtr<FJsonObject> NaturalReadyGeometry,ExplicitReadyGeometry,SettledReadyGeometry,FirstReloadGeometry;
            if(Role==EFrontlineRole::Lewis)if(auto* Gun=FrontlineMachinegunsV30::Get(U))NaturalReadyGeometry=Gun->DescribeSupportContactsV8447();
            Check(Prefix+TEXT("owned_trench_bay_is_physically_planted"),FVector::Dist2D(Feet,Goal)<=12.f&&!U->bMoving);
            const float AimCover=B->CoverFor(U);const FVector AimHead=U->Soldier->GetBoneLocation(TEXT("Head"));const int32 BeforeReloads=U->ReloadsCompleted,BeforeShots=U->ShotsDischarged;
            const int32 Dir=B->GetTeamDirection(Team);
            auto* Target=Make(C,U->SquadId+500,1-Team,EFrontlineRole::Rifle,0,Feet+FVector(Dir*650.f,0,0));
            U->RoundsRemaining=1;U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->FirePoseClock=U->AimSettleClock=0;
            U->SetActorRotation(FRotator(0,Dir>0?0.f:180.f,0));U->SetMotion(U->GetAimMotion());Pose(U,.25f);
            if(Role==EFrontlineRole::Lewis)
            {
                auto* Gun=FrontlineMachinegunsV30::Get(U);if(Gun)Gun->TickComponent(0,LEVELTICK_All,nullptr);
                if(Gun)ExplicitReadyGeometry=Gun->DescribeSupportContactsV8447();
                // One bounded animation settling observation distinguishes a
                // handoff-window artifact from an already invalid real mount.
                // It does not move an actor, select another bay or relax contact.
                Pose(U,1.f);if(Gun)Gun->TickComponent(0,LEVELTICK_All,nullptr);
                if(Gun)SettledReadyGeometry=Gun->DescribeSupportContactsV8447();
                FTransform ActualAimMount;
                Check(Prefix+TEXT("last_actual_ready_aim_has_two_contacts_and_cached_mount"),Gun&&Gun->HasBipodContactV36()&&Gun->ReadSupportedWeaponAnchorV8447(ActualAimMount));
                double Gap=0,FootOffset=0;bool Projected=false;
                if(SettledReadyGeometry.IsValid())
                {
                    SettledReadyGeometry->TryGetNumberField(TEXT("feet_to_bay_cm"),Gap);
                    SettledReadyGeometry->TryGetNumberField(TEXT("station_foot_xy_offset_cm"),FootOffset);
                    SettledReadyGeometry->TryGetBoolField(TEXT("actual_station_foot_projection"),Projected);
                }
                const bool PhysicalOffset=Gun&&Gun->HasBipodContactV36()&&Projected&&Gap<=12.f&&FMath::Abs(Gap-FootOffset)<=.01f;
                Check(Prefix+TEXT("actual_near_bay_feet_keep_physical_supported_bipod"),PhysicalOffset);
                MountedArrivalOffsetWitness|=PhysicalOffset&&Gap>1.f;
            }
            if(Target)U->DischargeWeapon(Target);
            const bool RealShot=Target&&U->ShotsDischarged==BeforeShots+1&&U->RoundsRemaining==0;
            int32 FeedSteps=0;while(RealShot&&U->WeaponPhase!=EFrontlineWeaponPhase::Reload&&FeedSteps++<1200)
            {B->Elapsed+=1.f/120.f;Tick(U,1.f/120.f);}
            const bool RealFeed=RealShot&&U->WeaponPhase==EFrontlineWeaponPhase::Reload;
            Check(Prefix+TEXT("actual_last_round_enters_authored_reload_through_tick"),RealFeed);
            // Retain the pose diagnostics after a failed shot admission, while
            // its separate real-fire check remains failed and visible.
            if(!RealFeed){U->RoundsRemaining=0;U->StartReload();}
            const int32 ReloadShotCount=U->ShotsDischarged;
            float PeakBlend=0,PeakLower=0,PeakCover=0,MaxRootTravel=0,MaxMountedWorldError=0;bool Finite=true,MountedContacts=true;int32 MountedSamples=0;TArray<TSharedPtr<FJsonValue>> Samples;
            int32 Step=0;while(U->WeaponPhase==EFrontlineWeaponPhase::Reload&&Step<2400)
            {
                const float Rates[]={1.f/120.f,1.f/60.f,1.f/30.f,.1f};const float DT=Rates[Step++%4];B->Elapsed+=DT;Tick(U,DT);
                if(Role==EFrontlineRole::Lewis&&!FirstReloadGeometry.IsValid())if(auto* Gun=FrontlineMachinegunsV30::Get(U))FirstReloadGeometry=Gun->DescribeSupportContactsV8447();
                auto* Anim=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance());const auto D=Anim?Anim->GetCharacterDiagnostics():FFrontlineAnimationV29Diagnostics();
                const float Blend=FrontlineBalanceV50::TrenchReloadShelterBlendV8447(U);PeakBlend=FMath::Max(PeakBlend,Blend);PeakLower=FMath::Max(PeakLower,D.LastTrenchReloadLowerCmV8447);PeakCover=FMath::Max(PeakCover,B->CoverFor(U));MaxRootTravel=FMath::Max(MaxRootTravel,float(FVector::Dist(Feet,U->Feet())));
                const FVector Head=U->Soldier->GetBoneLocation(TEXT("Head")),Muzzle=U->GetMuzzleLocation();Finite&=!Head.ContainsNaN()&&!Muzzle.ContainsNaN();
                float MountedWorldError=0;bool Bipod=true;
                if(Role==EFrontlineRole::Lewis&&U->WeaponPhase==EFrontlineWeaponPhase::Reload&&Blend>.01f&&D.MountedReloadFramesV8447>0)
                {
                    const FVector Expected=U->Soldier->GetComponentTransform().TransformPosition(D.LastMountedReloadWeaponAnchorCSV8447);
                    MountedWorldError=FVector::Dist(U->Soldier->GetBoneLocation(TEXT("weapon_rifle")),Expected);
                    MaxMountedWorldError=FMath::Max(MaxMountedWorldError,MountedWorldError);
                    const auto* Gun=FrontlineMachinegunsV30::Get(U);Bipod=Gun&&Gun->IsSupported()&&Gun->HasBipodContactV36();
                    MountedContacts&=Bipod;++MountedSamples;
                }
                if(Step%4==0){auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("phase"),U->GetWeaponReloadNormalizedPhase());R->SetNumberField(TEXT("shelter_blend"),Blend);R->SetNumberField(TEXT("cover"),B->CoverFor(U));R->SetStringField(TEXT("head"),Head.ToString());R->SetStringField(TEXT("muzzle"),Muzzle.ToString());R->SetNumberField(TEXT("pelvis_lower_cm"),D.LastTrenchReloadLowerCmV8447);R->SetNumberField(TEXT("rounds"),U->RoundsRemaining);R->SetNumberField(TEXT("mounted_weapon_world_target_error_cm"),MountedWorldError);R->SetBoolField(TEXT("actual_two_bipod_contacts"),Bipod);Samples.Add(MakeShared<FJsonValueObject>(R));}
            }
            auto* Anim=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance());const auto D=Anim?Anim->GetCharacterDiagnostics():FFrontlineAnimationV29Diagnostics();
            Check(Prefix+TEXT("actual_reload_tucks_body_and_restores_cover_in_phase"),Finite&&PeakBlend>.99f&&PeakLower>(Role==EFrontlineRole::Lewis?.1f:15.9f)&&PeakCover>=AimCover+.065f&&D.TrenchReloadFramesV8447>0&&D.TrenchReloadRejectedFramesV8447==0);
            Check(Prefix+TEXT("reload_keeps_authoring_grips_feet_and_leg_lengths"),MaxRootTravel<=.01f&&D.MaximumTrenchReloadFootDriftCmV8447<=.02f&&D.MaximumTrenchReloadGripDriftCmV8447<=.02f&&D.MaximumTrenchReloadLegLengthErrorCmV8447<=.02f);
            if(Role==EFrontlineRole::Lewis)Check(Prefix+TEXT("mounted_reload_retains_real_world_gun_bolt_contacts_and_arm_lengths"),MountedSamples>0&&MountedContacts&&MaxMountedWorldError<=.02f
                &&D.MountedReloadFramesV8447>0&&D.MaximumMountedReloadWeaponAnchorDriftCmV8447<=.02f&&D.MaximumMountedReloadBoltAnchorDriftCmV8447<=.02f&&D.MaximumMountedReloadArmLengthErrorCmV8447<=.02f);
            Check(Prefix+TEXT("full_physical_reload_refills_once_without_shots"),U->ReloadsCompleted==BeforeReloads+1&&U->RoundsRemaining==U->GetMagazineCapacity()&&U->ShotsDischarged==ReloadShotCount&&U->ShotsDuringReload==0);
            auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("team"),Team);R->SetNumberField(TEXT("role"),int32(Role));R->SetStringField(TEXT("aim_head"),AimHead.ToString());R->SetNumberField(TEXT("aim_cover"),AimCover);R->SetBoolField(TEXT("real_last_round_fired"),RealShot);R->SetBoolField(TEXT("real_recoil_enters_reload"),RealFeed);R->SetNumberField(TEXT("reloads_completed"),U->ReloadsCompleted-BeforeReloads);R->SetNumberField(TEXT("peak_reload_cover"),PeakCover);R->SetNumberField(TEXT("peak_pelvis_lower_cm"),PeakLower);R->SetNumberField(TEXT("maximum_actor_travel_cm"),MaxRootTravel);R->SetNumberField(TEXT("maximum_reload_foot_drift_cm"),D.MaximumTrenchReloadFootDriftCmV8447);R->SetNumberField(TEXT("maximum_reload_grip_drift_cm"),D.MaximumTrenchReloadGripDriftCmV8447);R->SetNumberField(TEXT("maximum_reload_leg_length_error_cm"),D.MaximumTrenchReloadLegLengthErrorCmV8447);R->SetNumberField(TEXT("mounted_reload_samples"),MountedSamples);R->SetBoolField(TEXT("all_mounted_samples_keep_two_real_bipod_contacts"),MountedContacts);R->SetNumberField(TEXT("maximum_mounted_weapon_world_target_error_cm"),MaxMountedWorldError);R->SetNumberField(TEXT("maximum_mounted_bolt_anchor_drift_cm"),D.MaximumMountedReloadBoltAnchorDriftCmV8447);R->SetNumberField(TEXT("maximum_mounted_arm_length_error_cm"),D.MaximumMountedReloadArmLengthErrorCmV8447);R->SetNumberField(TEXT("mounted_reload_reduced_frames"),D.MountedReloadReducedFramesV8447);R->SetArrayField(TEXT("phase_samples"),Samples);Reloads.Add(MakeShared<FJsonValueObject>(R));
            if(NaturalReadyGeometry.IsValid())R->SetObjectField(TEXT("natural_held_ready_geometry"),NaturalReadyGeometry);
            if(ExplicitReadyGeometry.IsValid())R->SetObjectField(TEXT("explicit_aim_025s_geometry"),ExplicitReadyGeometry);
            if(SettledReadyGeometry.IsValid())R->SetObjectField(TEXT("settled_aim_125s_geometry"),SettledReadyGeometry);
            if(FirstReloadGeometry.IsValid())R->SetObjectField(TEXT("first_reload_geometry"),FirstReloadGeometry);
        }
        Check(TEXT("v8447_real_centimetre_arrival_offset_exercises_true_bipod_support"),MountedArrivalOffsetWitness);
        J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("ground_escort_trajectories"),Cases);J->SetArrayField(TEXT("actual_reload_trajectories"),Reloads);
        J->SetBoolField(TEXT("real_arrival_offset_support_exercised"),MountedArrivalOffsetWitness);
        J->SetStringField(TEXT("scope"),TEXT("Real UK/DE actors: six ground assistant trajectories at 120/60/10 Hz against actual observed contacts; the old branch witness is computed from the same physical station and executed rearward leg, not an independently simulated old trajectory. Four authored reloads reach completion through production Tick at mixed rates with live bone, support and phase diagnostics. Existing V8444 physical entry checks remain separate. No campaign completion or visual acceptance is implied."));return J;
    }
    static bool Prepare(AWarfareFrontlineGameMode* B,int32 Team,bool Reload,FVector& Focus,TFunctionRef<void(const FString&,bool)> Check,bool Machinegun)
    {
        const bool Guard=Allowed(B)&&(Team==0||Team==1);Check(TEXT("v8447_pose_disposable_guard"),Guard);if(!Guard)return false;
        auto& C=Capture();if(!C||C->B!=B||C->CaptureTeam!=Team||C->CaptureMachinegun!=Machinegun){C.Reset();C=MakeUnique<FContext>(B);C->CaptureTeam=Team;C->CaptureMachinegun=Machinegun;Held(*C,8447990+Team,Team,Machinegun?EFrontlineRole::Lewis:EFrontlineRole::Rifle);}
        if(C->Made.IsEmpty()){Check(TEXT("v8447_pose_real_section_available"),false);return false;}
        Focus=FVector::ZeroVector;
        for(auto* U:C->Made)
        {
            U->Soldier->bPauseAnims=false;U->ReturnToIdle();U->bMoving=false;U->SetMotion(U->GetAimMotion());Pose(U,.25f);
            Focus+=U->Feet();if(Reload){U->RoundsRemaining=0;U->StartReload();}
        }
        Focus/=C->Made.Num();Focus.Z+=100.f;
        if(Reload)
        {
            int32 Steps=0;while(C->Made[0]->GetWeaponReloadNormalizedPhase()<.5f&&Steps++<1200)
            {B->Elapsed+=1.f/120.f;for(auto* U:C->Made)Tick(U,1.f/120.f);}
        }
        bool PoseReady=true;
        for(auto* U:C->Made)
        {
            Pose(U,0);U->Soldier->bPauseAnims=true;auto* A=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance());
            auto* Gun=FrontlineMachinegunsV30::Get(U);
            if(Gun)Gun->TickComponent(0,LEVELTICK_All,nullptr);
            PoseReady&=!U->bMoving&&(!Reload||(U->WeaponPhase==EFrontlineWeaponPhase::Reload&&A&&A->GetCharacterDiagnostics().LastTrenchReloadLowerCmV8447>.1f));
            if(Machinegun&&U->IsMachineGunner())PoseReady&=Gun&&Gun->IsSupported()&&Gun->HasBipodContactV36()
                &&(!Reload||(A&&A->GetCharacterDiagnostics().MountedReloadFramesV8447>0));
        }
        Check(Reload?TEXT("v8447_pose_real_mid_reload_frozen"):TEXT("v8447_pose_real_aim_frozen"),PoseReady);return PoseReady;
    }
};
TSharedRef<FJsonObject> FrontlineTrenchMotionV8447::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check){return FFrontlineTrenchMotionV8447Review::Run(B,Check);}
TSharedRef<FJsonObject> FrontlineTrenchMotionV8447::RunExistingContactChecksV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check){return FFrontlineTrenchMotionV8447Review::ExistingContactV87(B,Check);}
TSharedRef<FJsonObject> FrontlineTrenchMotionV8447::RunMachinegunContinuityChecksV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check){return FFrontlineTrenchMotionV8447Review::MachinegunContinuityV87(B,Check);}
TSharedRef<FJsonObject> FrontlineTrenchMotionV8447::RunMachinegunEngagementChecksV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check){return FFrontlineTrenchMotionV8447Review::MachinegunEngagementV87(B,Check);}
TSharedRef<FJsonObject> FrontlineTrenchMotionV8447::RunMachinegunLeaseChecksV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check){return FFrontlineTrenchMotionV8447Review::MachinegunLeaseV87(B,Check);}
bool FrontlineTrenchMotionV8447::PreparePoseV8447(AWarfareFrontlineGameMode* B,int32 Team,bool Reload,FVector& Focus,TFunctionRef<void(const FString&,bool)> Check,bool Machinegun){return FFrontlineTrenchMotionV8447Review::Prepare(B,Team,Reload,Focus,Check,Machinegun);}
void FrontlineTrenchMotionV8447::CleanupPoseV8447(AWarfareFrontlineGameMode* B){auto& C=FFrontlineTrenchMotionV8447Review::Capture();if(C&&C->B==B)C.Reset();}
