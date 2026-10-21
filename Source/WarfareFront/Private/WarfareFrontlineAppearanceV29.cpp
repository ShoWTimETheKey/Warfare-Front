#include "WarfareFrontlineAppearanceV29.h"
#include "WarfareFrontline.h"
#include "Animation/MorphTarget.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"

namespace
{
const TCHAR* FaceChannels[]={TEXT("WFV29_BlinkLeft"),TEXT("WFV29_BlinkRight"),
    TEXT("WFV29_BrowTension"),TEXT("WFV29_JawSet"),TEXT("WFV29_CheekLean"),TEXT("WFV29_JawBroad")};

float BlinkEnvelope(float Age,float Started,float Duration,float EyeDelay)
{
    const float T=(Age-Started-EyeDelay)/FMath::Max(.1f,Duration);
    if(T<0 || T>=1)return 0;
    // Fast closing lid, a short contact and a slower reopening lid.
    if(T<.30f)return FMath::SmoothStep(0.f,.30f,T);
    if(T<.43f)return 1;
    return 1.f-FMath::SmoothStep(.43f,1.f,T);
}
}

UFrontlineAppearanceV29::UFrontlineAppearanceV29()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PrePhysics;
    PrimaryComponentTick.TickInterval=1.f/30.f;
}

void UFrontlineAppearanceV29::InitializeAppearance(AWarfareFrontlineSoldier* InSoldier)
{
    OwnerSoldier=InSoldier;
    if(!OwnerSoldier || !OwnerSoldier->Soldier){SetComponentTickEnabled(false);return;}
    // Spawn order within a unit and its faction are stable; no shared global RNG
    // and no frame-count dependence can synchronize a marching section's faces.
    const uint32 Seed=0x29fac31du ^ (uint32(OwnerSoldier->SquadId+1)*0x9e3779b9u)
        ^ (uint32(OwnerSoldier->MemberIndex+1)*0x85ebca6bu)
        ^ (uint32(OwnerSoldier->Team+1)*0xc2b2ae35u) ^ (uint32(OwnerSoldier->Role)*0x27d4eb2fu);
    IdentitySeed=int32(Seed);FaceRandom.Initialize(IdentitySeed);
    RestLid=FaceRandom.FRandRange(.055f,.17f);
    RestBrow=FaceRandom.FRandRange(.12f,.34f);
    Lean=FaceRandom.FRandRange(.08f,.78f);Broad=FaceRandom.FRandRange(.06f,.72f);
    NextBlink=FaceRandom.FRandRange(.6f,4.4f);
    BrowWeight=RestBrow;
    AddTickPrerequisiteActor(OwnerSoldier);
    OwnerSoldier->Soldier->AddTickPrerequisiteComponent(this);
    BindMesh(OwnerSoldier->Soldier);
}

void UFrontlineAppearanceV29::Apply(USkeletalMeshComponent* Mesh,int32 Channel,float Weight)
{
    if(ResolvedNames[Channel].IsNone())return;
    Weight=FMath::Clamp(Weight,0.f,1.f);
    if(FMath::Abs(Mesh->GetMorphTarget(ResolvedNames[Channel])-Weight)>.002f)
        Mesh->SetMorphTarget(ResolvedNames[Channel],Weight,true);
}

void UFrontlineAppearanceV29::BindMesh(USkeletalMeshComponent* Mesh)
{
    BoundAsset=Mesh->GetSkeletalMeshAsset();bHasMorphs=true;
    for(int32 Channel=0;Channel<6;++Channel)
    {
        ResolvedNames[Channel]=NAME_None;
        if(BoundAsset)
        {
            // Legacy FBX import can prefix the Blender key container name.
            // Resolve exactly once per mesh swap, including weapon recovery
            // and the corresponding casualty mesh, then use FNames at runtime.
            for(const auto& Morph:BoundAsset->GetMorphTargets())
                if(Morph && (Morph->GetName()==FaceChannels[Channel]
                    || Morph->GetName().EndsWith(FString(TEXT("_"))+FaceChannels[Channel])))
                {ResolvedNames[Channel]=Morph->GetFName();break;}
        }
        bHasMorphs&=!ResolvedNames[Channel].IsNone();
    }
    if(bHasMorphs)
    {
        Apply(Mesh,4,Lean);Apply(Mesh,5,Broad);
        Apply(Mesh,0,RestLid);Apply(Mesh,1,RestLid*.94f);
        Apply(Mesh,2,BrowWeight);Apply(Mesh,3,.16f);
    }
}

void UFrontlineAppearanceV29::TickComponent(float DT,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DT,TickType,TickFunction);
    if(!OwnerSoldier || !OwnerSoldier->Soldier){SetComponentTickEnabled(false);return;}
    auto* Mesh=OwnerSoldier->Soldier.Get();
    if(BoundAsset!=Mesh->GetSkeletalMeshAsset())BindMesh(Mesh);
    if(!bHasMorphs)
    {
        if(!OwnerSoldier->IsAlive())SetComponentTickEnabled(false);
        else SetComponentTickInterval(.20f); // A living assistant can recover a V29 weapon/mesh later.
        return;
    }
    if(GetWorld() && GetWorld()->IsPaused())return;
    Age+=DT;
    if(!OwnerSoldier->IsAlive())
    {
        DeathAge+=DT;
        // A settled facial state; no endless blinking on a dead ragdoll.
        const float Settle=FMath::SmoothStep(.15f,1.3f,DeathAge);
        const float Closed=FMath::Lerp(RestLid,.76f+.18f*Lean,Settle);
        Apply(Mesh,0,Closed);Apply(Mesh,1,Closed*.98f);
        Apply(Mesh,2,FMath::Lerp(BrowWeight,.20f,Settle));
        Apply(Mesh,3,FMath::Lerp(JawWeight,.10f,Settle));
        // Dead soldiers cannot recover weapons or be revived. The separate
        // missing-limb follower gets its own one-shot appearance initialization.
        if(DeathAge>=1.4f)SetComponentTickEnabled(false);
        else SetComponentTickInterval(1.f/20.f);
        return;
    }
    const bool bVisible=Mesh->WasRecentlyRendered(.28f) && Mesh->GetPredictedLODLevel()<2;
    if(!bVisible)
    {
        if(bWasVisible){Apply(Mesh,0,RestLid);Apply(Mesh,1,RestLid*.94f);}
        bWasVisible=false;
        // Continue independent schedules while offscreen. Otherwise every
        // overdue blink starts together when the camera reveals a new section.
        if(Age>=NextBlink)NextBlink=Age+FaceRandom.FRandRange(.6f,6.7f);
        SetComponentTickInterval(.18f);
        return;
    }
    bWasVisible=true;SetComponentTickInterval(1.f/30.f);
    if(Age>=NextBlink)
    {
        BlinkStarted=Age;BlinkDuration=FaceRandom.FRandRange(.155f,.205f);
        NextBlink=Age+FaceRandom.FRandRange(2.5f,6.7f);++CompletedBlinks;
    }
    const float Pressure=FMath::Clamp(OwnerSoldier->Suppression,0.f,1.f);
    const float Attention=OwnerSoldier->bEngaging?.19f:0.f;
    const float Exertion=OwnerSoldier->bMoving?.11f:0.f;
    const float DesiredBrow=FMath::Clamp(RestBrow+Attention+Pressure*.25f+Exertion,0.f,.82f);
    const float DesiredJaw=FMath::Clamp(.15f+Attention*.8f+Pressure*.20f+Exertion,0.f,.58f);
    BrowWeight=FMath::FInterpTo(BrowWeight,DesiredBrow,DT,3.8f);
    JawWeight=FMath::FInterpTo(JawWeight,DesiredJaw,DT,3.8f);
    const float Lid=FMath::Clamp(RestLid+Attention*.24f+Pressure*.07f,0.f,.27f);
    const float BlinkL=BlinkEnvelope(Age,BlinkStarted,BlinkDuration,0.f);
    const float BlinkR=BlinkEnvelope(Age,BlinkStarted,BlinkDuration,.007f);
    Apply(Mesh,0,FMath::Lerp(Lid,1.f,BlinkL));
    Apply(Mesh,1,FMath::Lerp(Lid*.94f,1.f,BlinkR));
    Apply(Mesh,2,BrowWeight);Apply(Mesh,3,JawWeight);
}

void InitializeFrontlineAppearanceV29(AWarfareFrontlineSoldier* Soldier)
{
    if(!Soldier || Soldier->FindComponentByClass<UFrontlineAppearanceV29>())return;
    auto* Appearance=NewObject<UFrontlineAppearanceV29>(Soldier,TEXT("FieldFace_V29"));
    Soldier->AddInstanceComponent(Appearance);Appearance->RegisterComponent();
    Appearance->InitializeAppearance(Soldier);
}

void InitializeFrontlineCasualtyAppearanceV29(USkeletalMeshComponent* Source,USkeletalMeshComponent* Follower)
{
    if(!Source || !Follower || !Follower->GetSkeletalMeshAsset())return;
    for(int32 Channel=0;Channel<6;++Channel)
    {
        float Value=0;
        for(const auto& Curve:Source->GetMorphTargetCurves())
            if(Curve.Key.ToString()==FaceChannels[Channel]
                || Curve.Key.ToString().EndsWith(FString(TEXT("_"))+FaceChannels[Channel]))
            {Value=Curve.Value;break;}
        if(Channel<2)Value=.87f;
        if(Channel==2)Value=.20f;
        if(Channel==3)Value=.10f;
        for(const auto& Morph:Follower->GetSkeletalMeshAsset()->GetMorphTargets())
            if(Morph && (Morph->GetName()==FaceChannels[Channel]
                || Morph->GetName().EndsWith(FString(TEXT("_"))+FaceChannels[Channel])))
            {Follower->SetMorphTarget(Morph->GetFName(),Value);break;}
    }
}
