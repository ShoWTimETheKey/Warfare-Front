#include "WarfareFrontlineGameplayV51.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlinePlayerGuideV43.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMotionV40.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFRRUCampaignV82.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

namespace
{
struct FObserver
{
    FGuid Encounter;
    FVector Position=FVector::ZeroVector;
    float Still=0;
    bool EligibleV8219=false;
};
TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FObserver> Observers;
bool CanObserve(const AWarfareFrontlineSoldier* U)
{
    return U&&FrontlineGameplayV52::ObservationTrained(U)&&U->IsActiveCombatant()
        &&!U->bMoving&&U->GetMotionSpeed()<8.f&&U->Suppression<.5f
        &&U->WeaponPhase==EFrontlineWeaponPhase::Ready;
}
bool HasRunner(const AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* Observer)
{
    const auto* S=B->FindSquad(Observer->SquadId);if(!S)return false;
    for(const auto& W:S->Members)if(const auto* U=W.Get();U&&U!=Observer&&U->Team==Observer->Team
        &&U->IsActiveCombatant()&&FVector::DistSquared2D(U->Feet(),Observer->Feet())<=FMath::Square(850.f))return true;
    return false;
}
}

bool FrontlineGameplayV51::MaskEligibleRole(EFrontlineRole Role)
{
    // The research purchase represents a limited supply of checked replacement
    // filters and trained assault detachments, not the invention of gas masks.
    return Role==EFrontlineRole::Mills||Role==EFrontlineRole::Pioneer||Role==EFrontlineRole::Flamethrower;
}

bool FrontlineGameplayV51::PaidPrerequisiteSatisfied(int32 NodeId,int32 Required,uint32 Mask)
{
    if(Required==INDEX_NONE)return true;
    if(Required<0||Required>=32)return false;
    // V50 respirators were bought after node 5. Preserve that paid branch on
    // archive load; new purchases still use the current node 10 prerequisite.
    return (Mask&(1u<<Required))!=0||(NodeId==16&&Required==10&&(Mask&(1u<<5))!=0);
}

void FrontlineGameplayV51::Tick(AWarfareFrontlineGameMode* B,float DT)
{
    if(!B||!FMath::IsFinite(DT)||DT<0)return;
    for(auto I=Observers.CreateIterator();I;++I)
        if(!I.Key().IsValid()||(I.Key()->Battle==B&&I.Value().Encounter!=B->GetEncounterId()))I.RemoveCurrent();
    if(B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)return;
    for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsSectionLeader())
    {
        auto& S=Observers.FindOrAdd(U);
        const bool Same=S.Encounter==B->GetEncounterId();
        const bool Stayed=Same&&FVector::DistSquared2D(S.Position,U->Feet())<FMath::Square(12.f);
        const bool Eligible=CanObserve(U)&&HasRunner(B,U);
        S.Still=Stayed&&S.EligibleV8219&&Eligible?FMath::Min(ObservationSettleSeconds,S.Still+DT):0.f;
        S.Position=U->Feet();S.Encounter=B->GetEncounterId();S.EligibleV8219=Eligible;
    }
}

bool FrontlineGameplayV51::HasObservation(const AWarfareFrontlineGameMode* B,int32 Team,const FVector& Point)
{
    if(!B||Team<0||Team>1||Point.ContainsNaN()||B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)return false;
    for(const AWarfareFrontlineSoldier* U:B->Units)
    {
        if(!CanObserve(U)||U->Team!=Team||!HasRunner(B,U))continue;
        const auto* S=Observers.Find(const_cast<AWarfareFrontlineSoldier*>(U));
        if(!S||S->Encounter!=B->GetEncounterId()||S->Still<ObservationSettleSeconds
            ||FVector::DistSquared2D(S->Position,U->Feet())>=FMath::Square(12.f)
            ||FVector::DistSquared2D(U->Feet(),Point)>FMath::Square(6200.f*B->EncounterVisibilityScale())
            ||B->DirectFireMultiplier(U->Feet(),Point)<.9f)continue;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(V51ForwardObservation),true);
        for(const AWarfareFrontlineSoldier* Other:B->Units)if(Other)Query.AddIgnoredActor(Other);
        if(!B->GetWorld()->LineTraceTestByChannel(U->Feet()+FVector(0,0,155.f),Point+FVector(0,0,100.f),ECC_Visibility,Query))return true;
    }
    return false;
}

void FrontlineGameplayV51::RunObservationTimeBudgetChecksV8219(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review"))&&!WarfareFRRUCampaignV82::Active(B)
        &&B->GetWorld()->IsGameWorld()&&B->Phase==EFrontlinePhase::Fighting&&!B->Trenches.IsEmpty();
    Check(TEXT("v8219_observation_requires_actual_prepared_disposable_uk_de_world"),Safe);if(!Safe)return;
    const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedSmoke=B->SmokeZones;
    const auto SavedObservers=Observers;const float SavedClock=B->Elapsed;const int32 SavedWinner=B->BattleOutroWinner;
    auto& Research=const_cast<FFrontlineArmyProgress&>(B->GetActiveResearch());const auto SavedResearch=Research;
    TArray<AWarfareFrontlineSoldier*> Spawned;
    ON_SCOPE_EXIT
    {
        for(auto* U:Spawned)if(IsValid(U))U->Destroy();Observers=SavedObservers;
        B->Units=SavedUnits;B->Squads=SavedSquads;B->SmokeZones=SavedSmoke;Research=SavedResearch;
        B->Elapsed=SavedClock;B->BattleOutroWinner=SavedWinner;
    };
    B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();B->BattleOutroWinner=INDEX_NONE;
    Research.TechnologyMask|=1u<<10;
    const int32 Home=B->GetHomeTrenchIndex(0),Direction=B->GetTeamDirection(0),SquadId=82191992;
    const auto Make=[&](int32 Member)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        Spawned.Add(U);U->Initialize(B,SquadId,0,EFrontlineRole::Officer,Member);U->SetActorTickEnabled(false);
        U->bFieldEntered=true;U->bMoving=false;U->Suppression=0;U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->ReturnToIdle();
        const float Y=-560.f+Member*110.f;
        FVector P(FrontlineFieldV32::TrenchCentreX(Home,Y)+Direction*95.f,Y,0);
        P=FrontlineInfantryV45::TrenchSlot(U,Home,P,true);U->SetActorLocation(P+FVector(0,0,88));B->Units.Add(U);return U;
    };
    auto* Observer=Make(0);auto* Runner=Make(1);
    const bool Registered=IsValid(Observer)&&IsValid(Runner)&&Observer->Soldier&&Runner->Soldier
        &&Observer->GetRootComponent()&&Runner->GetRootComponent()
        &&Observer->GetRootComponent()->IsRegistered()&&Runner->GetRootComponent()->IsRegistered()
        &&Observer->Soldier->IsRegistered()&&Runner->Soldier->IsRegistered()
        &&Observer->Soldier->GetSkeletalMeshAsset()&&Runner->Soldier->GetSkeletalMeshAsset()
        &&FMath::IsNearlyEqual(float(Observer->Feet().Z),FrontlineInfantryV45::SurfaceHeight(B,Observer->Feet()),.05f)
        &&FMath::IsNearlyEqual(float(Runner->Feet().Z),FrontlineInfantryV45::SurfaceHeight(B,Runner->Feet()),.05f);
    Check(TEXT("v8219_observation_uses_registered_grounded_officer_and_runner"),Registered);if(!Registered)return;
    FFrontlineSquad Section;Section.Id=SquadId;Section.Team=0;Section.Role=EFrontlineRole::Officer;
    Section.Trench=Section.Destination=Home;Section.Slot=1;Section.Members={Observer,Runner};B->Squads.Add(Section);
    const FVector Feet=Observer->Feet(),RunnerFeet=Runner->Feet();
    float StillAtTwelve[3]={};bool Ready[3]={};const float Frames[]={.05f,.2f,.4f};
    for(int32 Case=0;Case<3;++Case)
    {
        Observers.Remove(Observer);B->Elapsed=1000.f;Tick(B,0.f);
        const float Frame=Frames[Case];const int32 Count=FMath::RoundToInt(2.4f/Frame);
        for(int32 Step=1;Step<=Count;++Step)
        {
            B->Elapsed=1000.f+Step*Frame;Tick(B,Frame);
            if(Step==Count/2)StillAtTwelve[Case]=Observers.FindChecked(Observer).Still;
        }
        Ready[Case]=HasObservation(B,0,Feet);
        Check(FString::Printf(TEXT("v8219_observation_frame_%.2f_completes_on_actual_two_point_four_game_seconds"),Frame),Ready[Case]);
    }
    Check(TEXT("v8219_observation_same_one_point_two_game_seconds_across_frames"),
        FMath::IsNearlyEqual(StillAtTwelve[0],1.2f,.001f)&&FMath::IsNearlyEqual(StillAtTwelve[1],StillAtTwelve[0],.001f)
        &&FMath::IsNearlyEqual(StillAtTwelve[2],StillAtTwelve[0],.001f));
    Observer->SetActorLocation(Feet+FVector(0,120,88));B->Elapsed+=.4f;Tick(B,.4f);
    Check(TEXT("v8219_observation_actual_relocation_resets_settling_without_backfill"),
        Observers.FindChecked(Observer).Still==0&&!HasObservation(B,0,Observer->Feet()));
    Observer->SetActorLocation(Feet+FVector(0,0,88));Tick(B,0.f);Tick(B,2.4f);
    Runner->SetActorLocation(RunnerFeet+FVector(0,1200,88));Tick(B,.4f);
    Check(TEXT("v8219_observation_actual_runner_loss_rejects_and_resets_contact"),
        Observers.FindChecked(Observer).Still==0&&!HasObservation(B,0,Observer->Feet()));
    Runner->SetActorLocation(RunnerFeet+FVector(0,0,88));Tick(B,4.f);
    Check(TEXT("v8219_observation_new_runner_contact_never_replays_unknown_interval"),
        Observers.FindChecked(Observer).Still==0&&!HasObservation(B,0,Observer->Feet()));
}

void FrontlineGameplayV51::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B||!B->IsProgressionVolatile()){Check(TEXT("v51_attachment_checks_require_volatile_profile"),false);return;}
    const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedSmoke=B->SmokeZones;
    const auto SavedObservers=Observers;const auto SavedPhase=B->Phase;const int32 SavedOutro=B->BattleOutroWinner;
    const auto SavedStrikes=B->Strikes;const auto SavedTactics=FrontlineDifficultyV30::State(B);
    const int32 SavedArtilleryCommands=B->ArtilleryCommands;const FString SavedMessage=B->LastMessage;
    const float SavedArtilleryReady[2]={B->ArtilleryReadyAt[0],B->ArtilleryReadyAt[1]};
    auto& Research=const_cast<FFrontlineArmyProgress&>(B->GetActiveResearch());const auto SavedResearch=Research;
    TArray<AWarfareFrontlineSoldier*> Spawned;
    ON_SCOPE_EXIT
    {
        for(auto* U:Spawned)if(IsValid(U))U->Destroy();B->Units=SavedUnits;B->Squads=SavedSquads;B->SmokeZones=SavedSmoke;
        Observers=SavedObservers;B->Phase=SavedPhase;B->BattleOutroWinner=SavedOutro;Research=SavedResearch;
        B->Strikes=SavedStrikes;FrontlineDifficultyV30::State(B)=SavedTactics;B->ArtilleryCommands=SavedArtilleryCommands;B->LastMessage=SavedMessage;
        for(int32 Team=0;Team<2;++Team)B->ArtilleryReadyAt[Team]=SavedArtilleryReady[Team];
    };
    B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    auto Make=[&](EFrontlineRole Role,int32 Member,int32 Id,const FVector& P,int32 Team=0)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->SetActorLocation(P+FVector(0,0,88));
        U->bMoving=false;U->Suppression=0;U->WeaponPhase=EFrontlineWeaponPhase::Ready;Spawned.Add(U);B->Units.Add(U);return U;
    };
    const auto Mobile=EFrontlineRole::MobileGun,Signals=EFrontlineRole::Signals;
    Check(TEXT("v51_new_role_ids_append_without_changing_saved_roles"),int32(EFrontlineRole::AntiTankRifle)==8&&int32(Mobile)==9&&int32(Signals)==10);
    Check(TEXT("v51_four_attachment_portraits_are_present"),LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V51/T_AttachmentPortraits_V51.T_AttachmentPortraits_V51"))!=nullptr);
    Check(TEXT("v51_attachments_have_real_sizes_cooldowns_and_chapter_introductions"),B->GetSquadSize(Mobile)==4&&B->GetSquadSize(Signals)==3
        &&FMath::IsNearlyEqual(B->RoleRecruitSeconds(Mobile),B->RoleRecruitSeconds(EFrontlineRole::Lewis))&&B->RoleRecruitSeconds(Signals)>B->RoleRecruitSeconds(EFrontlineRole::Rifle)
        &&FrontlineCampaignV41::RoleArrival(EFrontlineFaction::British,Mobile)==FrontlineCampaignV41::RoleArrival(EFrontlineFaction::British,EFrontlineRole::Lewis)
        &&FrontlineCampaignV41::RoleArrival(EFrontlineFaction::German,Mobile)==FrontlineCampaignV41::RoleArrival(EFrontlineFaction::German,EFrontlineRole::Lewis)
        &&FrontlineCampaignV41::RoleArrival(EFrontlineFaction::British,Signals)>=20);
    auto* Gun=Make(Mobile,0,95101,FVector(-1700,0,2400));
    auto* Bearer=Make(Mobile,1,95101,FVector(-1800,120,2400));
    auto* StaticGun=Make(EFrontlineRole::Lewis,0,95102,FVector(-1700,450,2400));
    const int32 ObserverHome=B->GetHomeTrenchIndex(0);
    FVector ObserverStart(FrontlineFieldV32::TrenchCentreX(ObserverHome,-560.f),-560,0);
    ObserverStart.Z=FrontlineInfantryV45::SurfaceHeight(B,ObserverStart);
    FVector RunnerStart=ObserverStart+FVector(0,110,0);RunnerStart.Z=FrontlineInfantryV45::SurfaceHeight(B,RunnerStart);
    auto* Observer=Make(Signals,0,95103,ObserverStart);
    auto* Runner=Make(Signals,1,95103,RunnerStart);
    auto* Assault=Make(EFrontlineRole::Mills,0,95104,FVector(-1700,800,2400));
    Check(TEXT("v51_new_attachment_actors_exist"),Gun&&Bearer&&StaticGun&&Observer&&Runner&&Assault);if(!Gun||!Bearer||!StaticGun||!Observer||!Runner||!Assault)return;
    FFrontlineSquad MG;MG.Id=95101;MG.Team=0;MG.Role=Mobile;MG.Members={Gun,Bearer};B->Squads.Add(MG);
    FFrontlineSquad FO;FO.Id=95103;FO.Team=0;FO.Role=Signals;FO.Members={Observer,Runner};B->Squads.Add(FO);
    const auto* MobileStance=FrontlineMachinegunsV30::Get(Gun);const auto* FixedStance=FrontlineMachinegunsV30::Get(StaticGun);
    Check(TEXT("v51_mobile_gun_uses_accepted_gun_rig_and_supported_actions"),Gun->IsAutomaticWeapon()&&!Bearer->IsAutomaticWeapon()
        &&Gun->Soldier->GetSkeletalMeshAsset()==StaticGun->Soldier->GetSkeletalMeshAsset()&&MobileStance&&MobileStance->HasAssets());
    Check(TEXT("v51_mobile_gun_trades_range_and_pressure_for_mobility"),FrontlineWeapons::ProfileFor(Gun).FireRangeCm<FrontlineWeapons::ProfileFor(StaticGun).FireRangeCm
        &&FrontlineWeapons::ProfileFor(Gun).DirectPressure<FrontlineWeapons::ProfileFor(StaticGun).DirectPressure
        &&FMath::IsNearlyEqual(FrontlineWeapons::AdvanceContactRangeFor(Bearer),FrontlineWeapons::ProfileFor(Gun).AdvanceContactCm));
    if(MobileStance&&FixedStance)
    {
        auto* A=const_cast<UFrontlineMachinegunV30*>(MobileStance);auto* Z=const_cast<UFrontlineMachinegunV30*>(FixedStance);
        A->State=Z->State=EFrontlineGunStanceV30::Deploy;
        Check(TEXT("v51_mobile_gun_deploys_thirty_percent_faster_without_carried_fire"),FMath::IsNearlyEqual(A->TransitionRate()/Z->TransitionRate(),1.30f)&&!A->CanFire());
    }
    Check(TEXT("v51_observation_officer_has_matched_pistol_and_rifle_escort"),Observer->IsCommissionedOfficer()&&!Observer->IsSectionLeader()&&!Runner->IsCommissionedOfficer()
        &&Observer->Soldier->GetSkeletalMeshAsset()&&Observer->GetMagazineCapacity()<=8&&Runner->GetMagazineCapacity()>=5);
    // Grounded integration: the real operation's trench slots, earthworks and
    // visibility query feed real artillery requests. Replay only the request's
    // random sequence; never bypass collision or manufacture a predicted plan.
    for(int32 Team=0;Team<2;++Team)
    {
        const int32 Home=B->GetHomeTrenchIndex(Team),Id=95103+Team*10,Dir=B->GetTeamDirection(Team);
        auto* O=Team==0?Observer:Make(Signals,0,Id,FVector::ZeroVector,Team);
        auto* R=Team==0?Runner:Make(Signals,1,Id,FVector::ZeroVector,Team);
        const FString Prefix=Team==0?TEXT("v51_british_trench_observer_"):TEXT("v51_german_trench_observer_");
        Check(Prefix+TEXT("actors_and_home_exist"),O&&R&&B->Trenches.IsValidIndex(Home));
        if(!O||!R||!B->Trenches.IsValidIndex(Home))continue;
        if(Team==1){FFrontlineSquad S;S.Id=Id;S.Team=Team;S.Role=Signals;S.Members={O,R};B->Squads.Add(S);}
        auto* S=B->FindSquad(Id);S->Trench=S->Destination=Home;S->Slot=1;S->bAdvancing=false;
        FVector Point;bool Visible=false,Grounded=false;int32 BaysTested=0;
        for(float Y:{-560.f,0.f,560.f})
        {
            ++BaysTested;
            for(auto* U:{O,R})
            {
                const float Lane=Y+(U==R?110.f:0.f);
                FVector P(FrontlineFieldV32::TrenchCentreX(Home,Lane)+Dir*95.f,Lane,0);
                P=FrontlineInfantryV45::TrenchSlot(U,Home,P,true);
                U->SetActorLocation(P+FVector(0,0,88));U->bMoving=false;U->Suppression=0;U->Health=100;
                U->WeaponPhase=EFrontlineWeaponPhase::Ready;
            }
            Point=FVector(Dir*600.f,Y,0);Point.Z=AWarfareFrontlineGround::HeightAt(Point.X,Point.Y);
            Observers.Remove(O);
            Check(Prefix+FString::Printf(TEXT("bay_%d_needs_settling"),BaysTested),!HasObservation(B,Team,Point));
            for(int32 I=0;I<24;++I)Tick(B,.1f);
            Grounded=FMath::IsNearlyEqual(float(O->Feet().Z),FrontlineInfantryV45::SurfaceHeight(B,O->Feet()),.05f)
                &&FMath::IsNearlyEqual(float(R->Feet().Z),FrontlineInfantryV45::SurfaceHeight(B,R->Feet()),.05f)&&B->CoverFor(O)>0;
            Visible=Grounded&&HasObservation(B,Team,Point);
            if(Visible)break; // A parapet may obscure one bay; no elevated fallback.
        }
        UE_LOG(LogTemp,Display,TEXT("WF_V51_GROUNDED_OBSERVER team=%d trench=%d bays=%d grounded=%d visible=%d observer=%s target=%s"),
            Team,Home,BaysTested,Grounded,Visible,*O->Feet().ToCompactString(),*Point.ToCompactString());
        Check(Prefix+TEXT("actual_ground_and_friendly_trench_cover"),Grounded);
        Check(Prefix+TEXT("observes_across_real_earthworks"),Visible);
        if(!Visible)continue;
        const auto Order=[&](TArray<FFrontlineStrike>& Out)
        {
            B->Strikes.Empty();B->ArtilleryReadyAt[Team]=B->Elapsed;B->ArtilleryCommands=7;
            FrontlineDifficultyV30::State(B).SupportSeedSerial=17;
            const FString Block=B->ArtilleryBlockReason(Team);const bool Accepted=B->RequestArtillery(Point,Team);Out=B->Strikes;
            UE_LOG(LogTemp,Display,TEXT("WF_V51_OBSERVED_FIRE_REQUEST team=%d accepted=%d observed=%d rounds=%d first_delay=%.3f block=%s"),
                Team,Accepted,HasObservation(B,Team,Point),Out.Num(),Out.IsEmpty()?-1.f:Out[0].ImpactAt-B->Elapsed,*Block);
            return Accepted;
        };
        const auto Same=[&](const TArray<FFrontlineStrike>& A,const TArray<FFrontlineStrike>& Z)
        {
            if(A.Num()!=Z.Num()||A.IsEmpty())return false;
            for(int32 I=0;I<A.Num();++I)if(!A[I].Target.Equals(Z[I].Target,.01f)
                ||!FMath::IsNearlyEqual(A[I].ImpactAt,Z[I].ImpactAt,.001f)||A[I].SalvoIndex!=Z[I].SalvoIndex)return false;
            return true;
        };
        TArray<FFrontlineStrike> Base,Seen,Lost,Obscured;
        R->Health=0;const bool BaseAccepted=Order(Base);R->Health=100;
        const bool SeenAccepted=Order(Seen);bool First=BaseAccepted&&SeenAccepted&&Base.Num()==Seen.Num()&&!Base.IsEmpty(),Later=First;
        for(int32 I=0;I<FMath::Min(Base.Num(),Seen.Num());++I)
        {
            if(Base[I].SalvoIndex==0)
            {
                const FVector2D A(Base[I].Target.X-Point.X,Base[I].Target.Y-Point.Y),Z(Seen[I].Target.X-Point.X,Seen[I].Target.Y-Point.Y);
                First&=FMath::IsNearlyEqual(Base[I].ImpactAt-Seen[I].ImpactAt,2.5f,.001f)&&Z.Equals(A*.70f,.02f);
            }
            else Later&=Base[I].Target.Equals(Seen[I].Target,.01f)&&FMath::IsNearlyEqual(Base[I].ImpactAt,Seen[I].ImpactAt,.001f);
        }
        Check(Prefix+TEXT("accepted_requests_change_first_salvo_delay_and_scatter"),First);
        Check(Prefix+TEXT("later_salvos_and_round_counts_stay_identical"),Later);
        R->Health=0;const bool LostAccepted=!HasObservation(B,Team,Point)&&Order(Lost);R->Health=100;
        Check(Prefix+TEXT("lost_runner_removes_benefit_from_actual_request"),LostAccepted&&Same(Base,Lost));
        FFrontlineSmokeZone Cloud;Cloud.Centre=(O->Feet()+Point)*.5f;Cloud.Radius=500;
        Cloud.StartedAt=B->Elapsed-1;Cloud.ExpiresAt=B->Elapsed+10;B->SmokeZones.Add(Cloud);
        const bool ObscuredAccepted=!HasObservation(B,Team,Point)&&Order(Obscured);
        Check(Prefix+TEXT("smoke_removes_benefit_from_actual_request"),ObscuredAccepted&&Same(Base,Obscured));B->SmokeZones.Empty();
        O->bMoving=true;Check(Prefix+TEXT("movement_interrupts_observation"),!HasObservation(B,Team,Point));O->bMoving=false;
        O->Suppression=.6f;Check(Prefix+TEXT("suppression_interrupts_observation"),!HasObservation(B,Team,Point));O->Suppression=0;
    }
    Research.TechnologyMask&=~(1u<<16);Check(TEXT("v51_assault_mask_is_never_default"),!FrontlineMotionV40::HasIssuedMask(Assault));
    Research.TechnologyMask|=1u<<16;const uint32 Paid=Research.TechnologyMask;
    Check(TEXT("v51_paid_respirator_bit_retained_but_no_universal_issue"),FrontlineMotionV40::HasIssuedMask(Assault)
        &&!FrontlineMotionV40::HasIssuedMask(Gun)&&!FrontlineMotionV40::HasIssuedMask(Observer)&&!FrontlineMotionV40::HasIssuedMask(Runner)&&Research.TechnologyMask==Paid);
    Check(TEXT("v51_unsealed_mask_still_has_full_gas_danger"),!FrontlineMotionV40::MaskSealed(Assault)&&FrontlineFieldV32::GasHitScale(Assault)==1.f&&SealedGasDoseScale>.20f);
    FFrontlineSmokeZone Gas;Gas.bGas=true;Gas.Team=1;Gas.Centre=Assault->Feet();Gas.Radius=950;Gas.StartedAt=B->Elapsed;Gas.ExpiresAt=B->Elapsed+100;B->SmokeZones.Add(Gas);
    FrontlineMotionV40::TickAction(Assault,.01f);
    Check(TEXT("v51_researched_assault_mask_still_requires_real_don_action"),FrontlineMotionV40::IsBusy(Assault)&&!FrontlineMotionV40::MaskSealed(Assault));
    for(int32 I=0;I<40;++I)FrontlineMotionV40::TickAction(Assault,.1f);
    FrontlineFieldV32::TickUnit(Assault,0);
    TArray<USkeletalMeshComponent*> MaskParts;Assault->GetComponents(MaskParts);bool Visible=false;
    for(auto* Part:MaskParts)if(Part!=Assault->Soldier&&Part->GetSkeletalMeshAsset()&&Part->GetSkeletalMeshAsset()->GetName().Contains(TEXT("Respirator")))Visible|=Part->IsVisible();
    Check(TEXT("v51_actual_sealed_mask_is_visible_and_retains_twenty_two_percent_exposure"),FrontlineMotionV40::MaskSealed(Assault)&&Visible&&FMath::IsNearlyEqual(FrontlineFieldV32::GasHitScale(Assault),.22f));
    B->SmokeZones.Empty();
    bool Ordinary=true;for(int32 Id=0;Id<27;++Id)if(Id!=12&&Id!=14&&Id!=15)
        Ordinary&=FrontlinePlayerGuideV43::TechnologyArrival(EFrontlineFaction::British,Id)==0&&FrontlinePlayerGuideV43::TechnologyArrival(EFrontlineFaction::German,Id)==0;
    Check(TEXT("v51_doctrine_research_has_no_mission_gate_and_MP18_keeps_1918_date"),Ordinary&&FrontlinePlayerGuideV43::TechnologyArrival(EFrontlineFaction::German,12)==11);
    Check(TEXT("v51_fitted_filter_training_costs_four_and_requires_observation"),B->GetTechnologyNodes()[16].Cost==4&&B->GetTechnologyNodes()[16].Prerequisite==10);
    Check(TEXT("v51_archive_keeps_paid_old_respirator_branch_without_granting_new_research"),
        PaidPrerequisiteSatisfied(16,10,(1u<<16)|(1u<<5))&&PaidPrerequisiteSatisfied(16,10,(1u<<16)|(1u<<10))
        &&!PaidPrerequisiteSatisfied(16,10,1u<<16)&&!PaidPrerequisiteSatisfied(15,10,(1u<<15)|(1u<<5)));
    bool Archives=true;for(auto F:{EFrontlineFaction::British,EFrontlineFaction::German})
    {
        const auto ValidArchive=[F](uint32 Mask)
        {
            for(const auto& N:AWarfareFrontlineGameMode::GetTechnologyNodesForFaction(F))if(Mask&(1u<<N.Id))
                for(int32 Required:{N.Prerequisite,N.SecondaryPrerequisite})
                    if(!PaidPrerequisiteSatisfied(N.Id,Required,Mask))return false;
            return true;
        };
        const uint32 OldPaid=(1u<<4)|(1u<<5)|(1u<<16),NewPaid=(1u<<3)|(1u<<10)|(1u<<16);
        Archives&=ValidArchive(OldPaid)&&ValidArchive(NewPaid)&&!ValidArchive(1u<<16)
            &&!ValidArchive((1u<<5)|(1u<<16))&&!(OldPaid&(1u<<10));
    }
    Check(TEXT("v51_both_saved_research_graphs_accept_old_and_new_paid_branches_reject_missing_roots"),Archives);
}
