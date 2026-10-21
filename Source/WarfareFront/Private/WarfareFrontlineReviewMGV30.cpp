#include "WarfareFrontlineReviewMGV30.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"

struct FFrontlineMGReviewV30
{
    TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;
    TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Crew,Targets;
    int32 Stage=0,CarryRejected=0,DeployRejected=0,GroundReady=0,TrenchReady=0,SupportedShots=0;
    float GroundCenterX=0,Clock=0;
    bool bGroundLogged=false,bTrenchLogged=false;
    AWarfareFrontlineSoldier* Spawn(int32 Team,int32 Member,int32 Id,EFrontlineRole Role,const FVector& Feet)
    {
        auto* B=Battle.Get();FActorSpawnParameters Params;
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(Feet+FVector(0,0,88),FRotator(0,B->GetTeamDirection(Team)>0?0:180,0),Params);
        U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->MotionSpeed=0;U->bMoving=false;
        B->Units.Add(U);return U;
    }
    void Begin(AWarfareFrontlineGameMode* B,float X)
    {
        Battle=B;GroundCenterX=X;
        for(int32 Team=0;Team<2;++Team)
        {
            FFrontlineSquad Squad;Squad.Id=30000+Team;Squad.Team=Team;Squad.Role=EFrontlineRole::Lewis;Squad.Slot=1;
            for(int32 M=0;M<3;++M)
            {
                FVector P(X+(Team?350.f:-350.f),float(M-1)*100.f,0);P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y);
                auto* U=Spawn(Team,M,Squad.Id,EFrontlineRole::Lewis,P);Squad.Members.Add(U);Crew.Add(U);
            }
            B->Squads.Add(Squad);
            FVector P(X+(Team?2100.f:-2100.f),1100,0);P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y);
            auto* T=Spawn(Team,0,30100+Team,EFrontlineRole::Rifle,P);Targets.Add(T);
        }
        const EFrontlinePhase Phase=B->Phase;B->Phase=EFrontlinePhase::Fighting;
        for(auto& W:Crew)if(auto* U=W.Get();U && U->IsMachineGunner())
        {
            const int32 Before=U->ShotsDischarged;U->DischargeWeapon(Targets[1-U->Team].Get());CarryRejected+=Before==U->ShotsDischarged?1:0;
        }
        B->Phase=Phase;
    }
    void SetStage(int32 Next)
    {
        auto* B=Battle.Get();if(!B)return;Stage=Next;Clock=0;
        if(Next==3)
            for(auto& W:Crew)if(auto* U=W.Get();U && U->IsMachineGunner())
            {U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->RoundsRemaining=FMath::Max(1,U->GetMagazineCapacity()-8);U->StartReload();}
        if(Next==5)
        {
            for(auto& W:Crew)if(auto* U=W.Get())
            {
                auto* C=FrontlineMachinegunsV30::Get(U);if(C)C->State=EFrontlineGunStanceV30::Carry;
                U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->FirePoseClock=0;U->bMoving=false;U->MotionSpeed=0;U->SetMotion(U->GetIdleMotion());
                const int32 T=B->GetHomeTrenchIndex(U->Team);if(!B->Trenches.IsValidIndex(T))continue;
                if(auto* S=B->FindSquad(U->SquadId)){S->Trench=T;S->Destination=T;S->bAdvancing=false;}
                FVector P(B->Trenches[T].X,float(U->MemberIndex-1)*100.f,0);P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y);
                U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(U->Team)>0?0:180,0));
            }
        }
    }
    void Tick(float DT)
    {
        auto* B=Battle.Get();if(!B || DT<=0)return;Clock+=DT;
        for(auto& W:Crew)if(auto* U=W.Get();U && U->IsAlive())
        {
            U->TickWeaponEffects(DT);U->UpdateWeaponCycle(DT);
            if(Stage==0)continue;
            auto* C=FrontlineMachinegunsV30::Get(U);if(!C)continue;
            const bool Blocked=C->Update(DT,Stage==4);
            if(Blocked)
            {
                if(auto* A=C->TransitionMotion()){U->SetMotion(A,false);if(auto* N=U->Soldier->GetSingleNodeInstance())N->SetPlayRate(C->TransitionRate());}
                if(Stage==1 && Clock<.10f && U->IsMachineGunner())
                {
                    const auto Phase=B->Phase;B->Phase=EFrontlinePhase::Fighting;const int32 Before=U->ShotsDischarged;
                    U->DischargeWeapon(Targets[1-U->Team].Get());if(Before==U->ShotsDischarged)++DeployRejected;B->Phase=Phase;
                }
                continue;
            }
            if(U->WeaponPhase==EFrontlineWeaponPhase::Ready)U->SetMotion(U->GetAimMotion());
            if(Stage==2 && U->IsMachineGunner() && U->WeaponPhase==EFrontlineWeaponPhase::Ready)
            {
                U->ShotClock-=DT;
                if(U->ShotClock<=0)
                {
                    const auto Phase=B->Phase;B->Phase=EFrontlinePhase::Fighting;const int32 Before=U->ShotsDischarged;
                    U->DischargeWeapon(Targets[1-U->Team].Get());SupportedShots+=U->ShotsDischarged-Before;B->Phase=Phase;
                }
            }
        }
        if(Stage==2 && !bGroundLogged && Clock>.4f)
        {
            for(auto& W:Crew)if(auto* U=W.Get())if(auto* C=FrontlineMachinegunsV30::Get(U);C && C->CanFire() && C->IsProne())++GroundReady;
            bGroundLogged=true;
        }
        if(Stage==5 && !bTrenchLogged && Clock>1.4f)
        {
            for(auto& W:Crew)if(auto* U=W.Get();U && U->IsMachineGunner())if(auto* C=FrontlineMachinegunsV30::Get(U);C && C->CanFire() && C->IsTrench())++TrenchReady;
            bTrenchLogged=true;
        }
    }
    TSharedRef<FJsonObject> Report() const
    {
        auto R=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
        R->SetStringField(TEXT("scope"),TEXT("Isolated actual-mesh/clip fixture, with real DischargeWeapon gates; not a full campaign benchmark."));
        R->SetNumberField(TEXT("carry_shots_rejected"),CarryRejected);R->SetNumberField(TEXT("deploy_shots_rejected"),DeployRejected);
        R->SetNumberField(TEXT("ground_supported_crew"),GroundReady);R->SetNumberField(TEXT("trench_supported_gunners"),TrenchReady);R->SetNumberField(TEXT("actual_supported_shots"),SupportedShots);
        bool Assets=true;int32 PoseErrors=0;
        for(const auto& W:Crew)if(const auto* U=W.Get())
        {
            const auto* C=FrontlineMachinegunsV30::Get(U);auto Row=MakeShared<FJsonObject>();
            Row->SetNumberField(TEXT("team"),U->Team);Row->SetNumberField(TEXT("member"),U->MemberIndex);Row->SetBoolField(TEXT("gunner"),U->IsMachineGunner());
            Row->SetBoolField(TEXT("assets_ready"),C && C->HasAssets());Assets &= C && C->HasAssets();
            Row->SetBoolField(TEXT("prone"),C && C->IsProne());Row->SetBoolField(TEXT("can_fire"),C && C->CanFire());
            Row->SetNumberField(TEXT("deployments"),C?C->Deployments:0);Row->SetNumberField(TEXT("packs"),C?C->Packs:0);
            Row->SetNumberField(TEXT("last_deploy_seconds"),C?C->LastDeploymentSeconds:0);Row->SetNumberField(TEXT("shots"),U->ShotsDischarged);
            Row->SetStringField(TEXT("playing"),U->Playing?U->Playing->GetPathName():TEXT(""));
            Row->SetNumberField(TEXT("muzzle_above_feet_cm"),U->GetMuzzleLocation().Z-U->Feet().Z);
            if(auto* A=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))
            {
                const auto D=A->GetCharacterDiagnostics();PoseErrors+=D.InvalidBoneFrames+D.UnsafeTorsoHierarchyFrames;
                Row->SetNumberField(TEXT("arm_length_error_cm"),D.MaximumArmLengthErrorCm);Row->SetNumberField(TEXT("grip_residual_cm"),A->GetMaximumGripResidualCm());
            }
            Rows.Add(MakeShared<FJsonValueObject>(Row));
        }
        R->SetArrayField(TEXT("crew"),Rows);R->SetNumberField(TEXT("invalid_pose_frames"),PoseErrors);
        R->SetBoolField(TEXT("success"),Assets && CarryRejected==2 && DeployRejected>=2 && GroundReady==6 && TrenchReady==2 && SupportedShots>0 && PoseErrors==0);
        return R;
    }
};
namespace{TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,TSharedPtr<FFrontlineMGReviewV30>> Reviews;}
void BeginFrontlineV30MachinegunReview(AWarfareFrontlineGameMode* B,float X)
{
    if(!B || Reviews.Contains(B))return;auto R=MakeShared<FFrontlineMGReviewV30>();Reviews.Add(B,R);R->Begin(B,X);
}
void SetFrontlineV30MachinegunReviewStage(AWarfareFrontlineGameMode* B,int32 Stage){if(auto* R=Reviews.Find(B))(*R)->SetStage(Stage);}
void TickFrontlineV30MachinegunReview(AWarfareFrontlineGameMode* B,float DT){if(auto* R=Reviews.Find(B))(*R)->Tick(DT);}
TSharedRef<FJsonObject> GetFrontlineV30MachinegunReviewReport(AWarfareFrontlineGameMode* B)
{
    if(auto* R=Reviews.Find(B))return (*R)->Report();auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),false);R->SetStringField(TEXT("error"),TEXT("MG review not started"));return R;
}
