#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontline.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

namespace
{
struct FSlot45
{
    FGuid Encounter;
    int32 Trench=INDEX_NONE;
    FVector Desired=FVector::ZeroVector,Position=FVector::ZeroVector;
    bool FiringValidatedV831=false;
    float SectorReviewAtV844=-1.f,BlockedSinceV844=-1.f;
    TWeakObjectPtr<AWarfareFrontlineSoldier> BlockedContactV844;
};
struct FSlots45 {FSlot45 Carry,Firing;};
TMap<TWeakObjectPtr<const AWarfareFrontlineSoldier>,FSlots45> Slots45;
}

float FrontlineInfantryV45::SurfaceHeight(AWarfareFrontlineGameMode* B,const FVector& P)
{
    float Physical=0;if(WarfareFRRUWorldV82::TryHeight(B,P.X,P.Y,Physical))return Physical;
    float Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y),Bank=0;
    if(FrontlineFieldV32::TryEarthworkSurfaceV36(B,P,Bank))Z=FMath::Max(Z,Bank);
    return Z;
}

float FrontlineInfantryV45::BankIntrusion(const AWarfareFrontlineSoldier* U,const FVector& P)
{
    if(!U||!U->Battle)return 0;
    const float Radius=(U->Capsule?U->Capsule->GetScaledCapsuleRadius():25.f)+15.f;
    const float Floor=AWarfareFrontlineGround::HeightAt(P.X,P.Y);
    float Intrusion=0;
    // The capsule plus shoulder/aim clearance is tested against actual bank
    // triangles, including the curved modules' rotated, irregular inner faces.
    for(int32 I=0;I<8;++I)
    {
        const float Angle=I*PI*.25f;
        const FVector Sample=P+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
        float Bank=0;
        if(FrontlineFieldV32::TryEarthworkSurfaceV36(U->Battle,Sample,Bank))
            Intrusion=FMath::Max(Intrusion,Bank-Floor);
    }
    return Intrusion;
}

FVector FrontlineInfantryV45::TrenchSlot(const AWarfareFrontlineSoldier* U,int32 Trench,const FVector& Desired,bool FiringStep)
{
    if(!U||!U->Battle||!U->Battle->Trenches.IsValidIndex(Trench))return Desired;
    // Cache the fixed slot, never a frame-dependent pose or terrain height.
    // Two entries prevent ordinary/crew destination queries invalidating each other.
    if(Slots45.Num()>256)for(auto It=Slots45.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& Pair=Slots45.FindOrAdd(U);auto& Cache=FiringStep?Pair.Firing:Pair.Carry;
    if(Cache.Encounter==U->Battle->GetEncounterId()&&Cache.Trench==Trench&&Cache.Desired.Equals(Desired,.01f))return Cache.Position;
    const float Centre=FrontlineFieldV32::TrenchCentreX(Trench,Desired.Y);
    FVector Best=Desired;Best.X=FMath::Clamp(float(Best.X),Centre-125.f,Centre+125.f);
    float Error=BankIntrusion(U,Best);
    if(Error>8.f)
    {
        float Score=MAX_flt;
        for(int32 I=-15;I<=15;++I)
        {
            FVector P=Desired;P.X=Centre+I*8.f;
            const float Penetration=BankIntrusion(U,P);
            const float Cost=FMath::Max(0.f,Penetration-8.f)*1000.f+FMath::Abs(float(P.X-Desired.X));
            if(Cost<Score){Score=Cost;Best=P;}
        }
    }
    bool FiringValidated=false;
    if(FiringStep&&!WarfareFRRUCampaignV82::Active(U->Battle)&&!U->IsMachineGunner())
    {
        // A wall-safe floor slot is not necessarily a firing step. Search a
        // small part of this man's own bay for a real, clear eye line before
        // caching it; do not raise his eyes or ignore earthwork collision.
        FCollisionQueryParams Query(SCENE_QUERY_STAT(V8218InfantryFiringBay),true);
        for(const AWarfareFrontlineSoldier* Other:U->Battle->Units)if(Other)Query.AddIgnoredActor(Other);
        const float Dir=U->Battle->GetTeamDirection(U->Team);
        const auto HasFrontage=[&](FVector P)
        {
            P.Z=SurfaceHeight(U->Battle,P);
            FVector End=P+FVector(Dir*650.f,0,0);End.Z=SurfaceHeight(U->Battle,End)+135.f;
            return !U->GetWorld()->LineTraceTestByChannel(P+FVector(0,0,145.f),End,ECC_Visibility,Query);
        };
        const auto Reserved=[&](const FVector& P)
        {
            for(const auto& Slot:Slots45)
            {
                const auto* Other=Slot.Key.Get();
                if(!Other||Other==U||!Other->IsAlive()||Other->Battle.Get()!=U->Battle.Get())continue;
                const auto* Assignment=U->Battle->FindSquad(Other->SquadId);
                // Corpses remain valid actors, and departure does not erase the
                // cached firing point. Only a current living held assignment
                // reserves its bay; the real movement field owns other bodies.
                if(!Assignment||Assignment->bAdvancing||Assignment->Trench!=Trench)continue;
                if(Slot.Value.Firing.Encounter==U->Battle->GetEncounterId()&&Slot.Value.Firing.Trench==Trench
                    &&FVector::DistSquared2D(P,Slot.Value.Firing.Position)<FMath::Square(56.f))return true;
            }
            return false;
        };
        // A neighbour's displaced firing step can occupy this man's otherwise
        // clear nominal slot. Reservation applies to the initial candidate too;
        // accepting it unchecked assigned two physical bodies the same point.
        if(BankIntrusion(U,Best)>8.f||!HasFrontage(Best)||Reserved(Best))
        {
            float Score=MAX_flt;FVector Candidate=Best;
            for(float Y:{0.f,-56.f,56.f,-112.f,112.f})for(float X:{125.f,101.f,77.f,53.f,29.f})
            {
                FVector P=Desired;P.Y+=Y;P.X=FrontlineFieldV32::TrenchCentreX(Trench,P.Y)+Dir*X;
                if(FMath::Abs(P.Y)>FrontlineBattlefield::HalfFrontage-35.f||BankIntrusion(U,P)>8.f||!HasFrontage(P))continue;
                // Nearby cached bays must remain separate even before their
                // assigned men have arrived on the physical floor.
                if(Reserved(P))continue;
                const float Cost=float(FVector::DistSquared2D(P,Desired));
                if(Cost<Score){Score=Cost;Candidate=P;}
            }
            if(Score<MAX_flt)Best=Candidate;
        }
        FiringValidated=BankIntrusion(U,Best)<=8.f&&HasFrontage(Best)&&!Reserved(Best);
    }
    Best.Z=SurfaceHeight(U->Battle,Best);
    Cache.Encounter=U->Battle->GetEncounterId();Cache.Trench=Trench;Cache.Desired=Desired;Cache.Position=Best;Cache.FiringValidatedV831=FiringValidated;
    Cache.SectorReviewAtV844=Cache.BlockedSinceV844=-1.f;Cache.BlockedContactV844.Reset();
    return Best;
}

void FrontlineInfantryV45::ObservedTrenchContactsV8444(const AWarfareFrontlineSoldier* U,TArray<AWarfareFrontlineSoldier*>& Contacts)
{
    Contacts.Reset();
    if(!U||!U->Battle||!U->IsActiveCombatant()||WarfareFRRUCampaignV82::Active(U->Battle))return;
    const auto* B=U->Battle.Get();const auto* S=B->FindSquad(U->SquadId);
    if(!S||S->bAdvancing||!B->Trenches.IsValidIndex(S->Trench)||B->Trenches[S->Trench].Owner!=U->Team)return;
    const float Range=FrontlineWeapons::ProfileFor(U).FireRangeCm*B->EncounterVisibilityScale();
    // At most the three resident sections' current contacts are inspected.
    // Unseen enemies and units merely passing a locked/bypassed trench cannot
    // direct a garrison's movement or grant an omniscient acquisition.
    for(const auto& Occupant:B->Squads)if(Occupant.Team==U->Team&&!Occupant.bAdvancing&&Occupant.Trench==S->Trench)
        for(const auto& Member:Occupant.Members)if(const auto* Observer=Member.Get();Observer&&Observer!=U&&Observer->IsActiveCombatant())
        {
            auto* Contact=Observer->GetObservedInfantryTargetV844();
            if(!Contact||!Contact->IsActiveCombatant()||Contact->Team==U->Team||Contact->Battle!=U->Battle
                ||FrontlineWeapons::DirectFireRangeDistanceCm(U,Contact->Feet())>Range)continue;
            if(!Contacts.Contains(Contact)&&B->HasSight(Observer,Contact))Contacts.Add(Contact);
        }
    Contacts.Sort([&](const AWarfareFrontlineSoldier& A,const AWarfareFrontlineSoldier& C)
        {return FVector::DistSquared2D(U->Feet(),A.Feet())<FVector::DistSquared2D(U->Feet(),C.Feet());});
}

AWarfareFrontlineSoldier* FrontlineInfantryV45::ObservedTrenchContactV844(const AWarfareFrontlineSoldier* U)
{
    TArray<AWarfareFrontlineSoldier*> Contacts;ObservedTrenchContactsV8444(U,Contacts);
    return Contacts.IsEmpty()?nullptr:Contacts[0];
}

void FrontlineInfantryV45::ReviewTrenchFiringSectorV844(AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle||!U->IsActiveCombatant()||U->IsMachineGunner()||FrontlineSpecialistsV41::IsSpecialist(U)
        ||U->IsSectionLeader()||WarfareFRRUCampaignV82::Active(U->Battle)||U->bMoving
        ||U->WeaponPhase!=EFrontlineWeaponPhase::Ready)return;
    auto* B=U->Battle.Get();const auto* S=B->FindSquad(U->SquadId);auto* Pair=Slots45.Find(U);
    if(!S||S->bAdvancing||!Pair||!B->Trenches.IsValidIndex(S->Trench)||B->Trenches[S->Trench].Owner!=U->Team)return;
    auto& C=Pair->Firing;
    if(C.Encounter!=B->GetEncounterId()||C.Trench!=S->Trench||FVector::DistSquared2D(U->Feet(),C.Position)>FMath::Square(12.f))return;
    if(C.SectorReviewAtV844<0.f){C.SectorReviewAtV844=B->Elapsed+.12f+U->GetMotionPhase()*.65f;return;}
    if(B->Elapsed<C.SectorReviewAtV844)return;
    C.SectorReviewAtV844=B->Elapsed+.8f;
    // Keep an already useful bay and its independent weapon cadence. A pause
    // to reload/aim is not a blocked firing sector or a relocation request.
    if(B->AcquireTarget(U))
    {C.BlockedSinceV844=-1.f;C.BlockedContactV844.Reset();return;}
    TArray<AWarfareFrontlineSoldier*> Contacts;ObservedTrenchContactsV8444(U,Contacts);
    auto* Seen=Contacts.IsEmpty()?nullptr:Contacts[0];
    if(!Seen){C.BlockedSinceV844=-1.f;C.BlockedContactV844.Reset();return;}
    if(C.BlockedContactV844!=Seen){C.BlockedContactV844=Seen;C.BlockedSinceV844=B->Elapsed;return;}
    if(C.BlockedSinceV844<0.f){C.BlockedSinceV844=B->Elapsed;return;}
    if(B->Elapsed-C.BlockedSinceV844<1.2f)return;
    C.SectorReviewAtV844=B->Elapsed+2.5f;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(V844InfantryActualSector),true);
    for(const AWarfareFrontlineSoldier* Other:B->Units)if(Other)Q.AddIgnoredActor(Other);
    const FVector EyeOffset=FrontlineMachinegunsV30::SightOrigin(U)-U->Feet();
    const float Dir=B->GetTeamDirection(U->Team);
    const float Range=FrontlineWeapons::ProfileFor(U).FireRangeCm*B->EncounterVisibilityScale();
    const auto Reserved=[&](const FVector& P)
    {
        for(const auto& Slot:Slots45)
        {
            const auto* Other=Slot.Key.Get();if(!Other||Other==U||!Other->IsAlive()||Other->Battle!=U->Battle)continue;
            const auto* Assignment=B->FindSquad(Other->SquadId);
            if(Assignment&&!Assignment->bAdvancing&&Assignment->Trench==S->Trench
                &&Slot.Value.Firing.Encounter==B->GetEncounterId()&&Slot.Value.Firing.Trench==S->Trench
                &&FVector::DistSquared2D(P,Slot.Value.Firing.Position)<FMath::Square(56.f))return true;
        }
        for(const AWarfareFrontlineSoldier* Other:B->Units)
            if(Other&&Other!=U&&Other->IsAlive()&&FVector::DistSquared2D(P,Other->Feet())<FMath::Square(56.f))return true;
        return false;
    };
    FVector Best=C.Position;float Score=MAX_flt;
    // A nearer observed man behind an unpassable traverse cannot monopolise
    // the search. Every candidate is still a current resident's actual contact.
    for(const auto* Contact:Contacts)for(float Y:{0.f,-56.f,56.f,-112.f,112.f})for(float X:{125.f,101.f,77.f,53.f,29.f})
    {
        FVector P=C.Desired;P.Y+=Y;P.X=FrontlineFieldV32::TrenchCentreX(S->Trench,P.Y)+Dir*X;P.Z=SurfaceHeight(B,P);
        if(FMath::Abs(P.Y)>FrontlineBattlefield::HalfFrontage-35.f||FVector::DistSquared2D(P,U->Feet())<FMath::Square(12.f)
            ||BankIntrusion(U,P)>8.f||Reserved(P)||!AllowsTrenchStep(U,U->Feet(),P)
            ||FrontlineWeapons::DirectFireRangeDistanceCm(U,Contact->Feet()+U->Feet()-P)>Range)continue;
        const FVector Eye=P+EyeOffset;
        const FVector Primary=FrontlineMachinegunsV30::SightTarget(Contact);
        const FVector Head=Contact->Soldier?Contact->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5):Contact->Feet()+FVector(0,0,145);
        if(U->GetWorld()->LineTraceTestByChannel(Eye,Primary,ECC_Visibility,Q)
            &&U->GetWorld()->LineTraceTestByChannel(Eye,Head,ECC_Visibility,Q))continue;
        const float Cost=FVector::DistSquared2D(P,C.Position)+.15f*FVector::DistSquared2D(P,C.Desired);
        if(Cost<Score){Best=P;Score=Cost;}
    }
    if(Score==MAX_flt)return;
    // The destination is invalidated once. Ordinary swept movement and body
    // avoidance must reach it; no teleport, raised eye or discarded wall.
    C.Position=Best;C.FiringValidatedV831=true;C.BlockedSinceV844=-1.f;C.BlockedContactV844.Reset();
    C.SectorReviewAtV844=B->Elapsed+4.f;
}

bool FrontlineInfantryV45::HasValidatedFiringStepV831(const AWarfareFrontlineSoldier* U,const FVector& Destination)
{
    if(!U||!U->Battle||WarfareFRRUCampaignV82::Active(U->Battle))return false;
    const auto* Squad=U->Battle->FindSquad(U->SquadId);const auto* Slots=Slots45.Find(U);
    return Squad&&!Squad->bAdvancing&&Slots&&Slots->Firing.FiringValidatedV831
        &&Slots->Firing.Encounter==U->Battle->GetEncounterId()&&Slots->Firing.Trench==Squad->Trench
        // Movement projects its requested heading onto XY; its temporary Z is
        // the current foot height, not the target bank's surface. Slot identity
        // is that fixed floor point. Actual support/body/step tests retain Z.
        &&FVector2D(Slots->Firing.Position.X,Slots->Firing.Position.Y).Equals(FVector2D(Destination.X,Destination.Y),.1f);
}

bool FrontlineInfantryV45::AllowsTrenchStep(const AWarfareFrontlineSoldier* U,const FVector& From,const FVector& To)
{
    if(!U||!U->Battle)return true;
    if(WarfareFRRUCampaignV82::Active(U->Battle))return WarfareFRRUWorldV82::AllowsStep(U,From,To);
    const auto* S=U->Battle->FindSquad(U->SquadId);
    if(!S)return true;
    int32 Trench=S->Trench;
    if(S->bAdvancing)
    {
        // Advancing is an order, not permission for avoidance to push a body
        // sideways through a traverse. Use the trench actually under the man.
        Trench=INDEX_NONE;float Nearest=180.f;
        for(int32 I=0;I<U->Battle->Trenches.Num();++I)
        {
            const float Gap=FMath::Abs(From.X-FrontlineFieldV32::TrenchCentreX(I,From.Y));
            if(Gap<Nearest){Nearest=Gap;Trench=I;}
        }
    }
    if(!U->Battle->Trenches.IsValidIndex(Trench))return true;
    // A man still arriving over the parapet must be allowed to enter. Once in
    // the bay, local avoidance cannot push his shoulders back through its wall.
    if(FMath::Abs(From.X-FrontlineFieldV32::TrenchCentreX(Trench,From.Y))>140.f)return true;
    const float Direction=U->Battle->GetTeamDirection(U->Team);
    const float FromSide=(From.X-FrontlineFieldV32::TrenchCentreX(Trench,From.Y))*Direction;
    const float ToSide=(To.X-FrontlineFieldV32::TrenchCentreX(Trench,To.Y))*Direction;
    const float Floor=AWarfareFrontlineGround::HeightAt(From.X,From.Y);
    // A squad can settle while its rear rank is still ON the rear bank.
    // Its footprint overlaps a high bank relative to the trench floor, but
    // the man stands above that floor. Permit inward/downward entry only;
    // men on the floor, lateral steps and the front wall retain their gate.
    if(From.Z>Floor+8.f&&FromSide<0.f&&ToSide>FromSide+.001f&&ToSide<=40.f)return true;
    const float NewIntrusion=BankIntrusion(U,To);
    if(NewIntrusion<=8.f)return true;
    if(NewIntrusion<=BankIntrusion(U,From)+.05f)return true;
    // A real forward crossing may enter/leave the bank. A near-lateral passing
    // tangent must stay in the floor corridor instead of entering the wall.
    const float Forward=(To.X-From.X)*Direction;
    return S->bAdvancing&&Forward>.001f&&FMath::Abs(To.Y-From.Y)<=Forward*.80f+.10f;
}

// Production actors, operation-fitted bank triangles, ordinary Tick and the
// evaluated skeleton are exercised together; no alternate movement model.
struct FFrontlineInfantryV45Review
{
    static void Quiet(AWarfareFrontlineSoldier* U)
    {
        U->SetActorTickEnabled(false);U->ReturnToIdle();U->ShotClock=U->GrenadeClock=10000;
        U->ReactionSeconds=U->DepartureClock=0;U->Suppression=0;
    }
    static void Pose(AWarfareFrontlineSoldier* U,float DT)
    {
        U->Soldier->HandleExistingParallelEvaluationTask(true,true);
        U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();
    }
    static void Clear(AWarfareFrontlineGameMode* B)
    {
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();
        B->Units.Empty();B->Squads.Empty();FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
    }
    static float PosePenetration(AWarfareFrontlineSoldier* U,int32& Contacts)
    {
        float Worst=0;
        for(const TCHAR* Bone:{TEXT("foot_l"),TEXT("foot_r"),TEXT("pelvis"),TEXT("spine_02"),TEXT("Head")})
        {
            const FVector P=U->Soldier->GetBoneLocation(Bone);float Height=0;
            if(FrontlineFieldV32::TryEarthworkSurfaceV36(U->Battle,P,Height))
            {++Contacts;Worst=FMath::Max(Worst,Height-float(P.Z));}
        }
        return Worst;
    }
    static TSharedRef<FJsonObject> AntiTankV48(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        auto J=MakeShared<FJsonObject>();FString UserDir;
        const bool Allowed=B&&B->GetWorld()&&B->IsProgressionVolatile()
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV48Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
        Check(TEXT("v48_at_stance_fixture_isolated_no_save"),Allowed);
        if(!Allowed){J->SetBoolField(TEXT("success"),false);return J;}
        const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedTracers=B->Tracers;
        const float SavedElapsed=B->Elapsed;const auto SavedPhase=B->Phase;const int32 SavedOutro=B->BattleOutroWinner;
        const int32 SavedShots=B->ShotsFired;const bool SavedTick=B->IsActorTickEnabled();
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        if(!Fleet){Check(TEXT("v48_at_vehicle_subsystem_available"),false);J->SetBoolField(TEXT("success"),false);return J;}
        const auto SavedVehicles=Fleet->Vehicles;const int32 SavedSerial=Fleet->NextSerial;
        const int32 SavedDeployments[2]={Fleet->DeploymentCountV37[0],Fleet->DeploymentCountV37[1]};
        struct FHidden48{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden48> Hidden;
        TArray<TObjectPtr<AFrontlineTank>> Fixtures;TObjectPtr<AActor> Wall=nullptr;
        const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
        for(AWarfareFrontlineSoldier* U:SavedUnits)Hide(U);for(AFrontlineTank* T:SavedVehicles)Hide(T);
        B->Units.Empty();B->Squads.Empty();Fleet->Vehicles.Empty();B->SetActorTickEnabled(false);
        B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=1000;
        ON_SCOPE_EXIT
        {
            Clear(B);for(AFrontlineTank* T:Fixtures)if(IsValid(T))T->Destroy();if(IsValid(Wall))Wall->Destroy();
            Fleet->Vehicles=SavedVehicles;Fleet->NextSerial=SavedSerial;for(int32 I=0;I<2;++I)Fleet->DeploymentCountV37[I]=SavedDeployments[I];
            B->Units=SavedUnits;B->Squads=SavedSquads;B->Tracers=SavedTracers;B->Elapsed=SavedElapsed;B->Phase=SavedPhase;
            B->BattleOutroWinner=SavedOutro;B->ShotsFired=SavedShots;B->SetActorTickEnabled(SavedTick);
            for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
            FrontlineMobilityV41::Reset(B);
        };
        const int32 Team=B->GetFactionForTeam(0)==EFrontlineFaction::German?0:1,Dir=B->GetTeamDirection(Team);
        // Raised isolated contact rig removes incidental map obstacles from this
        // specific posture/LOS regression. Ordinary actors, clips and Tick run;
        // this fixture is not a ground-contact or visual-quality certification.
        const FVector Station(0,0,2400);
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(Station+FVector(0,0,88),FRotator(0,Dir>0?0:180,0));
        if(!U){Check(TEXT("v48_at_actor_created"),false);J->SetBoolField(TEXT("success"),false);return J;}
        U->Initialize(B,948101,Team,EFrontlineRole::AntiTankRifle,0);B->Units.Add(U);Quiet(U);U->bFieldEntered=true;
        FFrontlineSquad Section;Section.Id=U->SquadId;Section.Team=Team;Section.Role=U->Role;Section.Trench=INDEX_NONE;
        Section.Destination=Dir>0?B->Trenches.Num()-1:0;Section.LastPassedTrench=FrontlineBattlefield::CentreTrench;
        Section.bAdvancing=true;Section.bReinforcement=false;Section.Members.Add(U);B->Squads.Add(Section);
        const auto TankAt=[&](const FVector& P)
        {auto* T=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,1-Team,false);if(T){T->SetActorTickEnabled(false);T->bEntered=true;T->SetActorLocation(P);Fixtures.Add(T);}return T;};
        auto* A=TankAt(Station+FVector(Dir*950.f,600,0));auto* Other=TankAt(Station+FVector(Dir*1200.f,-650,0));
        auto* Gun=FrontlineMachinegunsV30::Get(U);Pose(U,0);
        if(!A||!Other||!Gun||!Gun->HasAssets())
        {Check(TEXT("v48_at_actual_assets_and_targets_available"),false);J->SetBoolField(TEXT("success"),false);return J;}
        const auto Step=[&](float DT){B->Elapsed+=DT;U->Tick(DT);Pose(U,DT);Gun->TickComponent(DT,LEVELTICK_All,nullptr);};
        bool Stable=Gun->ObserveArmourV48()==A;
        Other->SetActorLocation(Station+FVector(Dir*720.f,-500,0));
        Stable&=Gun->ObserveArmourV48()==A;
        Other->SetActorEnableCollision(false);Other->Destroy();
        Wall=B->GetWorld()->SpawnActor<AActor>();
        if(!Wall){Check(TEXT("v48_at_occlusion_fixture_created"),false);J->SetBoolField(TEXT("success"),false);return J;}
        auto* Block=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Block);Wall->AddInstanceComponent(Block);
        Block->SetBoxExtent(FVector(25,280,60));Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Block->SetCollisionResponseToAllChannels(ECR_Ignore);Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Block->RegisterComponent();
        const FVector Toward=(A->AimPoint()-Station).GetSafeNormal2D();Wall->SetActorLocation(Station+Toward*280.f+FVector(0,0,65));Wall->SetActorRotation(Toward.Rotation());
        int32 SplitFrames=0,SupportedFrames=0,ReloadFrames=0;bool CorrectClips=true,Observed=true,NoShots=true;float Travel=0;
        for(int32 Frame=0;Frame<320;++Frame)
        {
            if(Frame==100){U->RoundsRemaining=0;U->StartReload();U->ShotClock=10000;}
            // Give the real discharge path one opportunity after the low muzzle
            // is known blocked; maintaining observation must not allow a shot.
            if(Frame==85&&!Gun->HasArmourFiringLineV48(A)){U->ShotClock=0;U->AimSettleClock=0;}
            const FVector Before=U->Feet();Step(.05f);Travel+=FVector::Dist2D(Before,U->Feet());
            Observed&=Gun->ObserveArmourV48()==A;
            if(!Gun->HasArmourFiringLineV48(A))++SplitFrames;
            if(Gun->IsSupported())
            {++SupportedFrames;CorrectClips&=GetNameSafe(U->Playing).StartsWith(TEXT("A_AT_Ground_"));}
            ReloadFrames+=U->WeaponPhase==EFrontlineWeaponPhase::Reload;
            NoShots&=U->ShotsDischarged==0;U->ShotClock=10000;
        }
        const int32 Deployments=Gun->Deployments,Packs=Gun->Packs;
        const bool Settled=Gun->IsSupported()&&Deployments==1&&Packs==0&&Travel<.01f;
        Block->SetBoxExtent(FVector(25,280,300));
        const bool TallWallHidden=Gun->ObserveArmourV48()==nullptr;
        Wall->Destroy();A->SetActorLocation(Station+FVector(Dir*12000.f,0,0));
        const bool RangeHidden=Gun->ObserveArmourV48()==nullptr;A->Destroy();
        const FVector Departure=U->Feet();for(int32 Frame=0;Frame<160;++Frame)Step(.05f);
        const bool Resumed=Gun->ObserveArmourV48()==nullptr&&Gun->Packs==1&&FVector::Dist2D(Departure,U->Feet())>100.f;
        Check(TEXT("v48_at_retains_visible_tank_across_distance_ties"),Stable);
        Check(TEXT("v48_at_observation_survives_deploy_and_reload_muzzle_occlusion"),Observed&&SplitFrames>100&&ReloadFrames>100);
        Check(TEXT("v48_at_one_deployment_no_stationary_pack_loop"),Settled&&SupportedFrames>250&&CorrectClips);
        Check(TEXT("v48_at_observation_does_not_authorize_obstructed_fire"),NoShots&&SplitFrames>100);
        Check(TEXT("v48_at_real_occlusion_and_range_invalidate_observation"),TallWallHidden&&RangeHidden);
        Check(TEXT("v48_at_destroyed_contact_releases_stance_and_resumes_advance"),Resumed);
        J->SetBoolField(TEXT("success"),Stable&&Observed&&SplitFrames>100&&ReloadFrames>100&&Settled&&SupportedFrames>250&&CorrectClips&&NoShots&&TallWallHidden&&RangeHidden&&Resumed);
        J->SetNumberField(TEXT("deployments_before_departure"),Deployments);J->SetNumberField(TEXT("packs_before_departure"),Packs);
        J->SetNumberField(TEXT("observation_without_muzzle_line_frames"),SplitFrames);J->SetNumberField(TEXT("reload_frames"),ReloadFrames);
        J->SetNumberField(TEXT("supported_frames"),SupportedFrames);J->SetNumberField(TEXT("stationary_travel_cm"),Travel);
        J->SetNumberField(TEXT("departure_travel_cm"),FVector::Dist2D(Departure,U->Feet()));
        J->SetStringField(TEXT("scope"),TEXT("Actual German T-Gewehr actor, supported animations and ordinary Tick. An isolated raised rig separates stable section observation from a low muzzle blocker; 16 seconds with a real reload, then full occlusion, range loss and 8 seconds of departure. No campaign/save mutation and no aesthetic certification."));
        return J;
    }
    static TSharedRef<FJsonObject> Crew(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        auto Report=MakeShared<FJsonObject>();FString UserDir;
        const bool Isolated=B&&B->GetWorld()&&B->IsProgressionVolatile()
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV46Review"))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
        Check(TEXT("v46_mg_crew_requires_isolated_volatile_profile"),Isolated);
        if(!Isolated){Report->SetBoolField(TEXT("success"),false);return Report;}
        const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedTracers=B->Tracers;
        const float SavedElapsed=B->Elapsed;const auto SavedPhase=B->Phase;const int32 SavedShots=B->ShotsFired,SavedNext=B->NextSquadId;
        const int32 SavedOutro=B->BattleOutroWinner;const bool SavedTick=B->IsActorTickEnabled();
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        const auto SavedVehicles=Fleet?Fleet->Vehicles:TArray<TObjectPtr<AFrontlineTank>>();
        const int32 SavedSerial=Fleet?Fleet->NextSerial:0,SavedDeployments[2]={Fleet?Fleet->DeploymentCountV37[0]:0,Fleet?Fleet->DeploymentCountV37[1]:0};
        struct FHidden {TWeakObjectPtr<AActor> Unit;bool Hidden,Collision;};TArray<FHidden> Hidden;
        TArray<TObjectPtr<AFrontlineTank>> Tanks;
        for(AWarfareFrontlineSoldier* U:SavedUnits)if(IsValid(U)){Hidden.Add({U,U->IsHidden(),U->GetActorEnableCollision()});U->SetActorHiddenInGame(true);U->SetActorEnableCollision(false);}
        for(AFrontlineTank* T:SavedVehicles)if(IsValid(T)){Hidden.Add({T,T->IsHidden(),T->GetActorEnableCollision()});T->SetActorHiddenInGame(true);T->SetActorEnableCollision(false);}
        if(Fleet)Fleet->Vehicles.Empty();
        B->Units.Empty();B->Squads.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->SetActorTickEnabled(false);
        ON_SCOPE_EXIT
        {
            Clear(B);for(AFrontlineTank* T:Tanks)if(IsValid(T))T->Destroy();
            if(Fleet){Fleet->Vehicles=SavedVehicles;Fleet->NextSerial=SavedSerial;for(int32 I=0;I<2;++I)Fleet->DeploymentCountV37[I]=SavedDeployments[I];}
            B->Units=SavedUnits;B->Squads=SavedSquads;B->Tracers=SavedTracers;B->Elapsed=SavedElapsed;B->Phase=SavedPhase;
            B->ShotsFired=SavedShots;B->NextSquadId=SavedNext;B->BattleOutroWinner=SavedOutro;B->SetActorTickEnabled(SavedTick);
            for(const auto& H:Hidden)if(auto* U=H.Unit.Get()){U->SetActorHiddenInGame(H.Hidden);U->SetActorEnableCollision(H.Collision);}
            FrontlineMobilityV41::Reset(B);
        };
        bool Ready=true,Projected=true,Reformed=true,Stopped=true,Supported=true,Fired=true,Resumed=true,GateStopped=true,Measured=true;
        int32 Sections=0,SupportMembers=0;TArray<TSharedPtr<FJsonValue>> Rows;
        const auto Place=[&](AWarfareFrontlineSoldier* U,const FVector& P){U->SetActorLocation(FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P)+88.f));};
        const auto Weight=[](AWarfareFrontlineSoldier* U){auto* A=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance());return A?A->GetCharacterDiagnostics().LocomotionPoseWeightV46:1.f;};
        const auto Step=[&](AWarfareFrontlineSoldier* U,float DT){U->Tick(DT);Pose(U,DT);if(auto* Gun=FrontlineMachinegunsV30::Get(U))Gun->TickComponent(DT,LEVELTICK_All,nullptr);};
        for(int32 Team=0;Team<2;++Team)
        {
            Clear(B);B->Elapsed=500.f;
            if(!Fleet||B->Trenches.Num()<2||!B->SpawnSquad(Team,EFrontlineRole::Lewis,B->GetHomeTrenchIndex(Team)))
            {Ready=false;continue;}
            auto& S=B->Squads.Last();S.Trench=INDEX_NONE;S.Destination=FrontlineBattlefield::CentreTrench;S.bAdvancing=true;S.bReinforcement=false;
            TArray<AWarfareFrontlineSoldier*> Crew;for(const auto& W:S.Members)if(auto* U=W.Get()){Quiet(U);U->bFieldEntered=true;Crew.Add(U);}
            if(Crew.Num()!=3){Ready=false;continue;}
            auto* G=Crew[0];auto* Gun=FrontlineMachinegunsV30::Get(G);const int32 Dir=B->GetTeamDirection(Team);
            const FVector Station(-Dir*FrontlineBattlefield::TrenchSpacing*.5f,0,0);
            for(int32 I=0;I<Crew.Num();++I){Place(Crew[I],Station+FVector(0,I*350.f,0));Crew[I]->SetActorRotation(FRotator(0,Dir>0?0:180,0));Pose(Crew[I],0);}
            auto* Target=Fleet->Spawn(Team?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V,1-Team,false);
            if(!Gun||!Target){Ready=false;continue;}Tanks.Add(Target);Target->SetActorTickEnabled(false);Target->bEntered=true;
            Target->FitGroundV35(G->Feet()+FVector(Dir*FMath::Min(900.f,FrontlineBattlefield::TrenchSpacing*.25f),0,0),0.f);
            FrontlineMobilityV41::Reset(B);
            for(int32 Frame=0;Frame<120&&!Gun->IsSupported();++Frame){B->Elapsed+=.05f;Step(G,.05f);}
            if(!Gun->IsSupported()||Fleet->AcquireInfantryTarget(G)!=Target){Ready=false;Target->Destroy();continue;}
            ++Sections;
            TArray<FVector> Initial;Initial.Add(G->Feet());
            for(int32 I=1;I<Crew.Num();++I)
            {
                auto* U=Crew[I];bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);
                // Real incoming state: the bearer has passed the gun and has
                // only 3 cm left laterally, but must return 140 cm behind him.
                Place(U,Goal+FVector(Dir*140.f,3.f,0));Initial.Add(U->Feet());
                U->MotionSpeed=U->RequestedTravelSpeedV43=390;U->bMoving=U->bWantedMovement=true;U->bAtDestinationV43=false;
                U->SetMotion(U->GetRunMotion());Pose(U,.05f);FrontlineMobilityV41::Reset(B);
                const FVector ProjectedStep=FrontlineMobilityV41::Step(U,U->Feet()+FVector(-Dir*140.f,3.f,0),19.5f,true)-U->Feet();
                Projected&=FMath::Abs(ProjectedStep.X)<.001f&&ProjectedStep.Size2D()<=3.001f;
                FrontlineMobilityV41::Reset(B);
            }
            G->ShotClock=G->AimSettleClock=0;const int32 ShotsBefore=G->ShotsDischarged;
            float LateTravel[3]={},MaxLateWeight[3]={},MaxLateSpeed[3]={};int32 MovingFrames[3]={};
            for(int32 Frame=0;Frame<160;++Frame)
            {
                B->Elapsed+=.05f;
                for(int32 I=0;I<Crew.Num();++I)
                {
                    auto* U=Crew[I];const FVector Before=U->Feet();Step(U,.05f);const float Travel=FVector::Dist2D(Before,U->Feet());
                    Measured&=FMath::IsNearlyEqual(U->GetMotionSpeed(),Travel/.05f,.03f)||(!U->bMoving&&Travel<.4f);
                    MovingFrames[I]+=U->bMoving;
                    if(Frame>=120){LateTravel[I]+=Travel;MaxLateWeight[I]=FMath::Max(MaxLateWeight[I],Weight(U));MaxLateSpeed[I]=FMath::Max(MaxLateSpeed[I],U->GetMotionSpeed());}
                }
            }
            Fired&=G->ShotsDischarged-ShotsBefore>=4&&Gun->IsSupported();
            for(int32 I=1;I<Crew.Num();++I)
            {
                auto* U=Crew[I];auto* Support=FrontlineMachinegunsV30::Get(U);++SupportMembers;
                const float Backward=(Initial[I].X-U->Feet().X)*Dir;
                Reformed&=Backward>90.f&&MovingFrames[I]>0;
                Stopped&=LateTravel[I]<.10f&&MaxLateSpeed[I]<.1f&&MaxLateWeight[I]<.001f&&!U->bMoving
                    &&U->Playing!=U->GetWalkMotion()&&U->Playing!=U->GetRunMotion();
                Supported&=Support&&Support->IsSupported()&&Support->Deployments==1&&Support->Packs==0;
                auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("team"),Team);Row->SetNumberField(TEXT("member"),I);
                Row->SetStringField(TEXT("role"),I==1?TEXT("assistant"):TEXT("observer"));Row->SetStringField(TEXT("clip"),GetNameSafe(U->Playing));
                Row->SetNumberField(TEXT("backward_reformation_cm"),Backward);Row->SetNumberField(TEXT("moving_frames"),MovingFrames[I]);
                Row->SetNumberField(TEXT("last_two_seconds_travel_cm"),LateTravel[I]);Row->SetNumberField(TEXT("last_two_seconds_max_speed_cm_s"),MaxLateSpeed[I]);
                Row->SetNumberField(TEXT("last_two_seconds_max_locomotion_weight"),MaxLateWeight[I]);Row->SetNumberField(TEXT("gunner_shots"),G->ShotsDischarged-ShotsBefore);
                Row->SetNumberField(TEXT("deployments"),Support?Support->Deployments:0);Row->SetNumberField(TEXT("packs"),Support?Support->Packs:0);Rows.Add(MakeShared<FJsonValueObject>(Row));
            }
            // A support gate can stop before it has a pack/deploy clip. Seed the
            // preceding run snapshot, then require the actual zero-travel tick
            // to select support aim rather than leave that run looping.
            auto* Bearer=Crew[1];Place(Bearer,Bearer->Feet()+FVector(Dir*70.f,0,0));Bearer->bMoving=true;
            Bearer->MotionSpeed=Bearer->RequestedTravelSpeedV43=390;Bearer->SetMotion(Bearer->GetRunMotion());Pose(Bearer,.05f);
            const FVector GateFrom=Bearer->Feet();B->Elapsed+=.05f;Step(Bearer,.05f);Pose(Bearer,.20f);
            GateStopped&=FVector::Dist2D(GateFrom,Bearer->Feet())<.001f&&!Bearer->bMoving&&Bearer->GetMotionSpeed()==0
                &&Bearer->Playing!=Bearer->GetWalkMotion()&&Bearer->Playing!=Bearer->GetRunMotion()&&Weight(Bearer)<.001f;
            Target->Destroy();const FVector Departure[3]={Crew[0]->Feet(),Crew[1]->Feet(),Crew[2]->Feet()};int32 DepartureFrames[3]={};
            for(int32 Frame=0;Frame<160;++Frame)
            {
                B->Elapsed+=.05f;
                for(int32 I=0;I<Crew.Num();++I){Step(Crew[I],.05f);DepartureFrames[I]+=Crew[I]->bMoving&&Weight(Crew[I])>.5f;}
            }
            for(int32 I=0;I<Crew.Num();++I)Resumed&=FVector::Dist2D(Departure[I],Crew[I]->Feet())>100.f&&DepartureFrames[I]>10;
        }
        Check(TEXT("v46_both_armies_use_actual_three_member_machinegun_sections"),Ready&&Sections==2&&SupportMembers==4);
        Check(TEXT("v46_forward_only_step_never_overshoots_projected_lateral_remainder"),Projected&&Sections==2);
        Check(TEXT("v46_assistants_and_observers_can_reform_behind_their_firing_gunner"),Reformed&&SupportMembers==4);
        Check(TEXT("v46_stationary_crew_has_zero_speed_and_evaluated_locomotion_weight"),Stopped&&Measured&&SupportMembers==4);
        Check(TEXT("v46_crew_remains_supported_while_gunner_really_fires"),Supported&&Fired&&SupportMembers==4);
        Check(TEXT("v46_blocked_support_gate_cannot_retain_previous_run_clip"),GateStopped&&Sections==2);
        Check(TEXT("v46_gunner_and_crew_resume_real_locomotion_when_contact_leaves"),Resumed&&Sections==2);
        Report->SetBoolField(TEXT("success"),Ready&&Projected&&Reformed&&Stopped&&Supported&&Fired&&Resumed&&GateStopped&&Measured&&Sections==2&&SupportMembers==4);
        Report->SetStringField(TEXT("method"),TEXT("Two real Lewis/MG08/15 sections; accepted incoming run, rearward support reform, eight seconds of ordinary Tick against an operational tank, evaluated pose weights, then eight seconds of departure. Fixed 0.05 s steps; no alternate movement model."));
        Report->SetArrayField(TEXT("support_members"),Rows);Report->SetNumberField(TEXT("sections"),Sections);return Report;
    }
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        FString UserDir;
        if(!B||!B->IsProgressionVolatile()||!(FParse::Param(FCommandLine::Get(),TEXT("WFV43Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV45Review")))
            ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
        {Check(TEXT("v45_infantry_requires_isolated_volatile_profile"),false);return;}
        const auto SavedRuns=B->CampaignRuns;const auto SavedId=B->ActiveCampaignId;const auto SavedFaction=B->PlayerFaction;
        const bool WasTicking=B->IsActorTickEnabled();
        ON_SCOPE_EXIT
        {
            Clear(B);B->AbandonBattleToOperations();B->CampaignRuns=SavedRuns;B->ActiveCampaignId=SavedId;B->PlayerFaction=SavedFaction;
            B->ResetEncounter();B->SetActorTickEnabled(WasTicking);Slots45.Reset();
        };
        B->AbandonBattleToOperations();FFrontlineCampaignProgress Run;Run.Id=FGuid::NewGuid();Run.Faction=EFrontlineFaction::British;
        Run.Difficulty=EFrontlineDifficulty::Rookie;Run.SelectedMission=Run.CompletedMissions=16;
        B->CampaignRuns={Run};B->ActiveCampaignId=Run.Id;B->PlayerFaction=Run.Faction;B->ResetEncounter();B->BeginBattle();B->SetActorTickEnabled(false);
        auto BankCheck=FrontlineFieldV32::RunBankContactChecksV42(B);
        Check(TEXT("v45_placement_uses_live_operation_fitted_bank_triangles"),BankCheck->GetBoolField(TEXT("success")));
        bool Slots=true,Motion=true,Steady=true;float WorstPose=0;int32 Examined=0,MovingFrames=0,Contacts=0;
        const int32 ActiveTrenches=B->Trenches.Num();
        for(int32 Team=0;Team<2;++Team)for(int32 Trench=0;Trench<ActiveTrenches;++Trench)for(int32 Slot=0;Slot<FrontlineBattlefield::TrenchCapacity;++Slot)
        {
            Clear(B);B->Trenches[Trench].Owner=Team;
            if(!B->SpawnSquad(Team,EFrontlineRole::Rifle,Trench)){Slots=false;continue;}
            auto& S=B->Squads.Last();S.Slot=Slot;
            for(const auto& W:S.Members)if(auto* U=W.Get())
            {
                Quiet(U);const FVector Goal=B->SoldierDestination(U);++Examined;
                Slots&=FrontlineInfantryV45::BankIntrusion(U,Goal)<=8.1f;
                // The ordinary movement solver must correct an offset within
                // the curved bay without walking into the bank or orbiting.
                FVector P=Goal;P.Y-=60;P.X=FrontlineFieldV32::TrenchCentreX(Trench,P.Y)+(Goal.X-FrontlineFieldV32::TrenchCentreX(Trench,Goal.Y));
                P.Z=FrontlineInfantryV45::SurfaceHeight(B,P)+88;U->SetActorLocation(P);Pose(U,0);
            }
            for(int32 Frame=0;Frame<30;++Frame)
            {
                B->Elapsed+=.05f;
                for(const auto& W:S.Members)if(auto* U=W.Get())
                {
                    const FVector Before=U->Feet();U->Tick(.05f);Pose(U,.05f);
                    Motion&=FMath::IsNearlyEqual(U->GetMotionSpeed(),float(FVector::Dist2D(Before,U->Feet())/.05f),.2f)||!U->bMoving;
                    MovingFrames+=U->bMoving;WorstPose=FMath::Max(WorstPose,PosePenetration(U,Contacts));
                    if(Frame==29)Steady&=!U->bMoving;
                }
            }
        }
        Check(TEXT("v45_all_active_trenches_both_sides_three_slots_clear_real_banks"),Slots&&ActiveTrenches>=3&&Examined>=ActiveTrenches*30);
        Check(TEXT("v45_in_bay_movement_uses_actual_stride_and_settles"),Motion&&Steady&&MovingFrames>40);
        Check(TEXT("v45_evaluated_moving_and_idle_body_bones_stay_outside_banks"),WorstPose<8.f);
        bool GunSteps=true,GunSupported=true,GunContacts=true;float WorstGun=0;int32 GunCount=0;
        for(int32 Team=0;Team<2;++Team)
        {
            Clear(B);const int32 Trench=FrontlineBattlefield::CentreTrench;B->Trenches[Trench].Owner=Team;
            if(!B->SpawnSquad(Team,EFrontlineRole::Lewis,Trench)){GunSteps=false;continue;}
            for(const auto& W:B->Squads.Last().Members)if(auto* U=W.Get())Quiet(U);
            auto* G=B->Squads.Last().Members[0].Get();if(!G){GunSteps=false;continue;}++GunCount;
            bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(G,B->SoldierDestination(G),Reform);
            GunSteps&=FrontlineInfantryV45::BankIntrusion(G,Goal)<=8.1f;
            G->SetActorLocation(Goal+FVector(0,0,88));
            for(int32 Frame=0;Frame<100;++Frame)
            {B->Elapsed+=.05f;G->Tick(.05f);Pose(G,.05f);WorstGun=FMath::Max(WorstGun,PosePenetration(G,Contacts));}
            auto* Gun=FrontlineMachinegunsV30::Get(G);GunSupported&=Gun&&Gun->IsSupported()&&!G->bMoving;
            if(Gun)Gun->TickComponent(.05f,LEVELTICK_All,nullptr);
            GunContacts&=Gun&&Gun->HasBipodContactV36();
        }
        Check(TEXT("v45_both_machinegun_firing_steps_fit_actual_inner_bank"),GunSteps&&GunSupported&&GunCount==2&&WorstGun<8.f);
        Check(TEXT("v45_repositioned_machineguns_keep_two_real_bipod_contacts"),GunCount==2&&GunContacts);
        Clear(B);B->Trenches[2].Owner=0;
        if(B->SpawnSquad(0,EFrontlineRole::Rifle,2))
        {
            auto* U=B->Squads.Last().Members[0].Get();Quiet(U);Pose(U,0);
            U->ReceiveHit(1,0,1,FVector::ForwardVector,EFrontlineDamageKind::Bullet);auto* Node=U->Soldier->GetSingleNodeInstance();
            const float Before=Node?Node->GetCurrentTime():0;Pose(U,.20f);
            Check(TEXT("v45_actual_bullet_fall_advances_30_percent_faster"),Node&&!U->IsAlive()&&U->DeathState.bAuthoredFall
                &&FMath::IsNearlyEqual(Node->GetPlayRate(),1.30f,.001f)&&FMath::IsNearlyEqual(Node->GetCurrentTime()-Before,.26f,.01f));
        }
        else Check(TEXT("v45_actual_bullet_fall_advances_30_percent_faster"),false);
        Clear(B);FrontlineMotionV41::RunOutcomeChecksV45(B,Check);
        UE_LOG(LogTemp,Display,TEXT("WF_V45_INFANTRY slots=%d moving_frames=%d bank_contacts=%d max_body_bank_depth_cm=%.3f max_gun_bank_depth_cm=%.3f"),Examined,MovingFrames,Contacts,WorstPose,WorstGun);
    }
};

void FrontlineInfantryV45::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineInfantryV45Review::Run(B,Check);}
TSharedRef<FJsonObject> FrontlineInfantryV45::RunCrewChecksV46(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineInfantryV45Review::Crew(B,Check);}
TSharedRef<FJsonObject> FrontlineInfantryV45::RunAntiTankStanceChecksV48(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineInfantryV45Review::AntiTankV48(B,Check);}
