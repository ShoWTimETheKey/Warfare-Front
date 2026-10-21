#include "WarfareFrontlineWaterV51.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFRRUContentV82.h"
#include "WarfareFRRUWorldTerrainV826.inl"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_EDITOR
#include "StaticMeshResources.h"
#include "StaticMeshComponentLODInfo.h"
#endif

namespace
{
constexpr float Life=6.2f;
constexpr float BloodLife=110.f;
const FName BloodPositions[8]={TEXT("BloodSource0V53"),TEXT("BloodSource1V53"),TEXT("BloodSource2V53"),TEXT("BloodSource3V53"),TEXT("BloodSource4V53"),TEXT("BloodSource5V53"),TEXT("BloodSource6V53"),TEXT("BloodSource7V53")};
const FName BloodStates[8]={TEXT("BloodState0V53"),TEXT("BloodState1V53"),TEXT("BloodState2V53"),TEXT("BloodState3V53"),TEXT("BloodState4V53"),TEXT("BloodState5V53"),TEXT("BloodState6V53"),TEXT("BloodState7V53")};
TMap<TWeakObjectPtr<UWorld>,TWeakObjectPtr<UFrontlineWaterResponseV51>> NativeResponsesV82;
UFrontlineWaterResponseV51* FindResponse(UWorld* W)
{
    if(W)if(const auto* B=W->GetAuthGameMode<AWarfareFrontlineGameMode>();WarfareFRRUCampaignV82::Active(B))
    {
        const auto* Weak=NativeResponsesV82.Find(W);auto* Response=Weak?Weak->Get():nullptr;
        return Response&&Response->IsNativeBindingCurrentV82()?Response:nullptr;
    }
    if(W)for(TActorIterator<AWarfareFrontlineGround> It(W);It;++It)
        if(auto* R=It->FindComponentByClass<UFrontlineWaterResponseV51>())return R;
    return nullptr;
}
}

UFrontlineWaterResponseV51::UFrontlineWaterResponseV51()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bStartWithTickEnabled=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
}

void UFrontlineWaterResponseV51::Bind(AWarfareFrontlineGround* Ground)
{
    if(!IsValid(Ground)||!Ground->GetWorld()||!Ground->GetWorld()->IsGameWorld())return;
    auto* Response=Ground->FindComponentByClass<UFrontlineWaterResponseV51>();
    if(!Response)
    {
        if(!IsValid(Ground->BasinWaterV44)||!Ground->BasinWaterV44->IsRegistered())return;
        Response=NewObject<UFrontlineWaterResponseV51>(Ground);
        Ground->AddInstanceComponent(Response);Response->RegisterComponent();
    }
    Response->Rebind(Ground);
}

void UFrontlineWaterResponseV51::Rebind(AWarfareFrontlineGround* Ground)
{
    ResetBindingV82();
    WaterSurface=Ground&&IsValid(Ground->BasinWaterV44)&&Ground->BasinWaterV44->IsRegistered()?Ground->BasinWaterV44.Get():nullptr;WaterMID=nullptr;
    for(auto& I:Impulses)I=FImpulse();
    Accepted=Rejected=0;WeatherAge=1.f;ContactPositionsV64.Reset();FootContactsV64=TrackContactsV64=0;
    if(!WaterSurface||!WaterSurface->GetMaterial(0))return;
    if(auto* Parent=WaterSurface->GetMaterial(0)->GetMaterial())
    {
        const FString Version=Parent->GetName()==TEXT("M_BasinWater_V83")?TEXT("V83"):Parent->GetName()==TEXT("M_BasinWater_V75")?TEXT("V75"):FString();
        if(!Version.IsEmpty()){const FString Name=TEXT("MI_BasinWaterBlood_")+Version+TEXT("_V84411");
            if(auto* Pigment=LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/Warfare/Field/V84411/Materials/")+Name+TEXT(".")+Name)))WaterSurface->SetMaterial(0,Pigment);}
    }
    WaterMID=WaterSurface->CreateDynamicMaterialInstance(0);
    Weather=AWarfareFrontlineWeather::Get(GetWorld());
    Rain=Weather.IsValid()?Weather->GetRainAmount():0.f;
    MaxImpulses=UFrontlineRenderSettingsSubsystem::EffectsBudgetV43()<.5f?2:8;
    MaxBloodSources=UFrontlineRenderSettingsSubsystem::EffectsBudgetV43()<.5f?2:8;
    ClearBlood();
    if(const auto* B=GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>())EncounterV53=B->GetEncounterId();
    Upload(GetWorld()->GetTimeSeconds());
}

void UFrontlineWaterResponseV51::ResetBindingV82()
{
    WaterSurface=nullptr;WaterMID=nullptr;
    WaterMaterialSlotV82=0;NativeSceneIdV82.Reset();NativeRevisionV82=0;
    for(auto& Impulse:Impulses)Impulse=FImpulse();
    Accepted=Rejected=0;ClearBlood();ContactPositionsV64.Reset();
    ContactCursorV64=FootContactsV64=TrackContactsV64=0;
    Weather.Reset();Rain=0.f;WeatherAge=1.f;EncounterV53.Invalidate();
}

bool UFrontlineWaterResponseV51::HasBoundSurfaceV8219() const
{
    return IsValid(GetOwner())&&IsValid(WaterSurface)&&WaterSurface->IsRegistered()
        &&WaterSurface->GetOwner()==GetOwner()&&IsValid(WaterSurface->GetStaticMesh())&&IsValid(WaterMID)
        &&WaterMaterialSlotV82>=0&&WaterMaterialSlotV82<WaterSurface->GetNumMaterials()
        &&WaterSurface->GetMaterial(WaterMaterialSlotV82)==WaterMID;
}

bool UFrontlineWaterResponseV51::BindNativeV82(AActor* SceneOwner,
    const TArray<FFrontlineNativeWaterBindingV82>& Bindings,const FString& SceneId,uint32 Revision)
{
    if(!SceneOwner||!SceneOwner->ActorHasTag(TEXT("FRRU_V82Scene"))||!SceneOwner->GetWorld()
        ||!SceneOwner->GetWorld()->IsGameWorld()||Bindings.Num()!=1
        ||WarfareFRRUContentV82::WaterHeightTexturePathV82(SceneId).IsEmpty())return false;
    for(const auto& Binding:Bindings)
        if(!IsValid(Binding.Surface)||!Binding.Surface->IsRegistered()||Binding.MaterialSlot<0
            ||Binding.MaterialSlot>=Binding.Surface->GetNumMaterials()||!IsValid(Binding.Material)
            ||(!Binding.Material->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Water/M_Water_"))
                &&!Binding.Material->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Water/WorldRedesign/M_Water_"))
                &&!Binding.Material->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Water/WorldFamilies/M_Water_"))
                &&Binding.Material->GetPathName()!=WarfareFRRUWorldTerrainV826::WaterMaterial(SceneId)))return false;
    const auto* Battle=SceneOwner->GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>();
    FString ActualId;uint32 ActualRevision=0;int32 SolidCount=0;FVector2D Minimum,Maximum;
    if(!Battle||!WarfareFRRUWorldV82::SceneInfo(Battle,ActualId,ActualRevision,SolidCount,Minimum,Maximum)
        ||ActualId!=SceneId||ActualRevision!=Revision)return false;
    auto* Response=SceneOwner->FindComponentByClass<UFrontlineWaterResponseV51>();
    if(!Response){Response=NewObject<UFrontlineWaterResponseV51>(SceneOwner);SceneOwner->AddInstanceComponent(Response);Response->RegisterComponent();}
    TArray<UMaterialInstanceDynamic*> Prepared;
    for(const auto& Binding:Bindings)
    {
        auto* MID=UMaterialInstanceDynamic::Create(Binding.Material,Response);
        if(!MID)return false;Prepared.Add(MID);
    }
    Response->ResetBindingV82();Response->NativeSceneIdV82=SceneId;Response->NativeRevisionV82=Revision;
    for(int32 I=0;I<Bindings.Num();++I)
    {
        Bindings[I].Surface->SetMaterial(Bindings[I].MaterialSlot,Prepared[I]);
        Response->WaterSurface=Bindings[I].Surface;Response->WaterMaterialSlotV82=Bindings[I].MaterialSlot;Response->WaterMID=Prepared[I];
    }
    for(auto& Impulse:Response->Impulses)Impulse=FImpulse();
    Response->Accepted=Response->Rejected=0;Response->WeatherAge=1.f;Response->ContactPositionsV64.Reset();
    Response->FootContactsV64=Response->TrackContactsV64=0;
    Response->Weather=AWarfareFrontlineWeather::Get(SceneOwner->GetWorld());
    Response->Rain=Response->Weather.IsValid()?Response->Weather->GetRainAmount():0.f;
    Response->MaxImpulses=UFrontlineRenderSettingsSubsystem::EffectsBudgetV43()<.5f?2:8;
    Response->MaxBloodSources=Response->MaxImpulses;
    if(const auto* B=SceneOwner->GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>())Response->EncounterV53=B->GetEncounterId();
    Response->ClearBlood();Response->Upload(SceneOwner->GetWorld()->GetTimeSeconds());
    NativeResponsesV82.Add(SceneOwner->GetWorld(),Response);return true;
}

bool UFrontlineWaterResponseV51::IsNativeBindingCurrentV82(bool ReuseFrameValidation) const
{
    const auto* B=GetWorld()?GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>():nullptr;
    FString Id;uint32 Revision=0;int32 Solids=0;FVector2D Minimum,Maximum;
    const FString HeightPath=WarfareFRRUContentV82::WaterHeightTexturePathV82(NativeSceneIdV82);
    UTexture* Height=nullptr;
    const bool HeightBound=WaterMID&&WaterMID->GetTextureParameterValue(FMaterialParameterInfo(TEXT("NativeHeightGridV82")),Height)
        &&Height&&!HeightPath.IsEmpty()&&Height->GetPathName()==HeightPath;
    return IsValid(GetOwner())&&GetOwner()->ActorHasTag(TEXT("FRRU_V82Scene"))&&!NativeSceneIdV82.IsEmpty()
        &&IsValid(WaterSurface)&&WaterSurface->IsRegistered()&&WaterSurface->GetOwner()==GetOwner()
        &&WaterMID&&WaterSurface->GetMaterial(WaterMaterialSlotV82)==WaterMID&&HeightBound
        &&B&&B->GetEncounterId()==EncounterV53&&WarfareFRRUWorldV82::SceneInfo(B,Id,Revision,Solids,Minimum,Maximum)
        &&Id==NativeSceneIdV82&&Revision==NativeRevisionV82
        &&(ReuseFrameValidation?WarfareFRRUContentV82::SceneStagedForWater(B,Id,Revision)
            :WarfareFRRUContentV82::SceneStaged(B,Id,Revision));
}

void UFrontlineWaterResponseV51::UnbindNativeV82(AActor* SceneOwner)
{
    if(SceneOwner)if(auto* Response=SceneOwner->FindComponentByClass<UFrontlineWaterResponseV51>();Response&&!Response->NativeSceneIdV82.IsEmpty())
    {
        if(auto* Weak=NativeResponsesV82.Find(SceneOwner->GetWorld());Weak&&Weak->Get()==Response)NativeResponsesV82.Remove(SceneOwner->GetWorld());
        Response->ResetBindingV82();for(auto& Impulse:Response->Impulses)Impulse=FImpulse();Response->ClearBlood();Response->ContactPositionsV64.Reset();
    }
}

void UFrontlineWaterResponseV51::SetScalarV82(FName Name,float Value)
{
    if(WaterMID)WaterMID->SetScalarParameterValue(Name,Value);
}
void UFrontlineWaterResponseV51::SetVectorV82(FName Name,const FLinearColor& Value)
{
    if(WaterMID)WaterMID->SetVectorParameterValue(Name,Value);
}

bool UFrontlineWaterResponseV51::NotifyImpact(UWorld* World,const FVector& Position,float Radius,float Strength)
{
    if(auto* R=FindResponse(World))return R->Admit(Position,Radius,Strength);
    return false;
}
bool UFrontlineWaterResponseV51::NotifyBlood(UWorld* World,const FVector& Position,float Mass,float Radius,bool bCorpseSeep)
{
    if(auto* R=FindResponse(World))if(R->AdmitBlood(Position,Mass,Radius)){R->CorpseSeepDeposits+=bCorpseSeep?1:0;return true;}
    return false;
}
void UFrontlineWaterResponseV51::ClearBlood()
{
    for(auto& Source:Blood)Source=FBloodSource();
    BloodDeposits=BloodRejected=BloodRunoffDeposits=BloodMerged=CorpseSeepDeposits=0;
    UploadBlood(GetWorld()?GetWorld()->GetTimeSeconds():0.f);
}
bool UFrontlineWaterResponseV51::FindBloodEntry(const FVector& P,FVector& Wet,float& Group,bool& Runoff) const
{
    float Level=0,Depth=0;Runoff=false;
    if(AWarfareFrontlineGround::QueryWaterSurfaceV51(P.X,P.Y,Level,Depth,&Group))
    {
        if(P.Z>Level+65.f||P.Z<Level-FMath::Max(Depth+35.f,65.f))return false;
        Wet=FVector(P.X,P.Y,Level);return true;
    }
    const float Start=AWarfareFrontlineGround::HeightAt(P.X,P.Y);
    if(FMath::Abs(float(P.Z)-Start)>65.f)return false;
    // Near-shore seepage follows a short continuously descending ground path.
    // A radial proximity test alone would bleed through a dry ridge into a pool.
    float Best=MAX_flt;FVector BestWet=FVector::ZeroVector;float BestGroup=-1.f;
    for(int32 Direction=0;Direction<8;++Direction)
    {
        const float A=Direction*PI*.25f;const FVector2D Step(FMath::Cos(A)*20.f,FMath::Sin(A)*20.f);
        float Previous=Start;
        for(int32 I=1;I<=6;++I)
        {
            const float X=P.X+Step.X*I,Y=P.Y+Step.Y*I;
            const float Bed=AWarfareFrontlineGround::HeightAt(X,Y);
            if(Bed>Previous+.25f||Bed>Start+.5f)break;
            if(AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth,&Group))
            {
                if(Level<=Start+1.5f&&I*20.f<Best){Best=I*20.f;BestWet=FVector(X,Y,Level);BestGroup=Group;}
                break;
            }
            Previous=Bed;
        }
    }
    if(BestGroup<0)return false;
    Wet=BestWet;Group=BestGroup;Runoff=true;return true;
}
bool UFrontlineWaterResponseV51::AdmitBlood(const FVector& P,float Mass,float Radius)
{
    if(!HasBoundSurfaceV8219()||P.ContainsNaN()||FMath::Abs(P.X)>200000.||FMath::Abs(P.Y)>200000.
        ||!FMath::IsFinite(Mass)||!FMath::IsFinite(Radius)||Mass<=0.f||Radius<=0.f)
    {++BloodRejected;return false;}
    FVector Wet;float Group=-1;bool Runoff=false;
    if(!FindBloodEntry(P,Wet,Group,Runoff)){++BloodRejected;return false;}
    const float Now=GetWorld()->GetTimeSeconds();int32 Slot=INDEX_NONE;
    for(int32 I=0;I<MaxBloodSources;++I)
    {
        const auto& Source=Blood[I];
        if(Source.Mass>0&&Now-Source.Began<100.f&&FMath::Abs(Source.Group-Group)<.25f/255.f
            &&FMath::Abs(Source.Level-Wet.Z)<2.f&&FVector2D::DistSquared(Source.Position,FVector2D(Wet))<FMath::Square(65.f))
        {Slot=I;++BloodMerged;break;}
    }
    if(Slot==INDEX_NONE)
    {
        Slot=0;float Weakest=MAX_flt;
        for(int32 I=0;I<MaxBloodSources;++I)
        {
            const auto& Source=Blood[I];const float Age=FMath::Max(0.f,Now-Source.Began);
            const float Remaining=Age>=BloodLife?0.f:Source.Mass*FMath::Exp(-Age*.025f);
            if(Remaining<Weakest){Weakest=Remaining;Slot=I;}
        }
        Blood[Slot]=FBloodSource();Blood[Slot].Position=FVector2D(Wet);Blood[Slot].Group=Group;Blood[Slot].Level=Wet.Z;Blood[Slot].Began=Now;Blood[Slot].Radius=FMath::Clamp(Radius,55.f,240.f);
    }
    auto& Source=Blood[Slot];Source.Mass=FMath::Min(4.f,Source.Mass+FMath::Min(Mass,1.5f));Source.Radius=FMath::Max(Source.Radius,FMath::Clamp(Radius,55.f,240.f));
    ++BloodDeposits;BloodRunoffDeposits+=Runoff?1:0;UploadBlood(Now);return true;
}
void UFrontlineWaterResponseV51::UploadBlood(float Now)
{
    if(!WaterMID)return;
    SetScalarV82(TEXT("BloodCountV53"),float(MaxBloodSources));
    for(int32 I=0;I<8;++I)
    {
        const auto& Source=Blood[I];const bool Active=I<MaxBloodSources&&Source.Mass>0.f&&Now>=Source.Began&&Now-Source.Began<BloodLife;
        SetVectorV82(BloodPositions[I],FLinearColor(Source.Position.X,Source.Position.Y,Source.Began,Source.Group));
        SetVectorV82(BloodStates[I],FLinearColor(Source.Mass,Source.Radius,Source.Level,Active?1.f:0.f));
    }
}

bool UFrontlineWaterResponseV51::Admit(const FVector& P,float Radius,float Strength,bool Contact)
{
    if(!HasBoundSurfaceV8219()||P.ContainsNaN()||FMath::Abs(P.X)>200000.||FMath::Abs(P.Y)>200000.
        ||!FMath::IsFinite(Radius)||!FMath::IsFinite(Strength)||Radius<=0||Strength<=0)
    {++Rejected;return false;}
    Radius=FMath::Clamp(Radius,60.f,1400.f);Strength=FMath::Clamp(Strength,0.f,1.35f);
    float Level=0,Depth=0,Group=-1.f;FVector Entry=P;
    bool Wet=AWarfareFrontlineGround::QueryWaterSurfaceV51(P.X,P.Y,Level,Depth,&Group);
    // Pressure can reach a neighbouring pool even when the shell hits its lip.
    // The shader only renders on real clipped water; it never paints a puddle
    // on dry ground. Eight coarse pressure samples are constant per explosion.
    for(int32 I=0;!Wet&&!Contact&&I<8;++I)
    {
        const float Angle=I*PI*.25f;
        Entry=FVector(P.X+FMath::Cos(Angle)*Radius*.58f,P.Y+FMath::Sin(Angle)*Radius*.58f,P.Z);
        Wet=AWarfareFrontlineGround::QueryWaterSurfaceV51(Entry.X,Entry.Y,Level,Depth,&Group);
    }
    if(!Wet||FMath::Abs(float(P.Z)-Level)>700.f){++Rejected;return false;}
    const float Now=GetWorld()->GetTimeSeconds();
    const int32 First=Contact?(MaxImpulses>2?4:1):0,Last=Contact?MaxImpulses:(MaxImpulses>2?4:1);
    int32 Slot=First;float Lowest=MAX_flt;
    for(int32 I=First;I<Last;++I)
    {
        const float Age=Now-Impulses[I].Began;
        const float Remaining=Age>=Life?0.f:Impulses[I].Strength*FMath::Exp(-FMath::Max(0.f,Age)*.8f);
        if(Remaining<Lowest){Lowest=Remaining;Slot=I;}
    }
    Impulses[Slot]={FVector2D(Entry.X,Entry.Y),Now,Radius,Strength,Group,Level,Contact};
    ++Accepted;Upload(Now);return true;
}

void UFrontlineWaterResponseV51::Upload(float Now)
{
    if(!WaterMID)return;
    SetScalarV82(TEXT("WaterClockV51"),Now);
    SetScalarV82(TEXT("RainV51"),Rain);
    SetScalarV82(TEXT("ImpulseCountV51"),float(MaxImpulses));
    FLinearColor Strengths(0,0,0,0);
    float* Values[]={&Strengths.R,&Strengths.G,&Strengths.B,&Strengths.A};
    FLinearColor ContactStrengths(0,0,0,0);float* ContactValues[]={&ContactStrengths.R,&ContactStrengths.G,&ContactStrengths.B,&ContactStrengths.A};
    for(int32 I=0;I<8;++I)
    {
        const auto& Impulse=Impulses[I];
        const bool Active=I<MaxImpulses&&Now>=Impulse.Began&&Now-Impulse.Began<Life;
        *(I<4?Values[I]:ContactValues[I-4])=Active?Impulse.Strength:0.f;
        SetVectorV82(FName(*FString::Printf(TEXT("WaterImpulse%dV51"),I)),
            FLinearColor(float(Impulse.Position.X),float(Impulse.Position.Y),Impulse.Began,Impulse.Radius));
        SetVectorV82(FName(*FString::Printf(TEXT("WaterBody%dV64"),I)),FLinearColor(Impulse.Group,Impulse.Level,Impulse.Contact?1.f:0.f,0));
    }
    SetVectorV82(TEXT("WaterStrengthsV51"),Strengths);
    SetVectorV82(TEXT("WaterContactsV64"),ContactStrengths);
}

void UFrontlineWaterResponseV51::SampleContactsV64()
{
    auto* B=GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>();if(!B||B->Phase!=EFrontlinePhase::Fighting)return;
    auto Sample=[&](AActor* A,bool Tank)
    {
        if(!IsValid(A))return;const FVector P=A->GetActorLocation();const TWeakObjectPtr<AActor> Key(A);
        FVector* Previous=ContactPositionsV64.Find(Key);if(!Previous){ContactPositionsV64.Add(Key,P);return;}
        const float Travel=FVector2D::Distance(FVector2D(*Previous),FVector2D(P));
        if(Travel<(Tank?85.f:52.f))return;*Previous=P;if(Travel>500.f)return;
        float Level=0,Depth=0;const float Offset=Tank?90.f:12.f;
        const FVector Side=A->GetActorRightVector()*Offset;
        for(int32 I=0;I<(Tank?2:1);++I)
        {
            const FVector Q=P+Side*(I?1.f:-1.f);
            if(!AWarfareFrontlineGround::QueryWaterSurfaceV51(Q.X,Q.Y,Level,Depth)||Depth<.8f||Depth>(Tank?110.f:75.f))continue;
            if(Admit(FVector(Q.X,Q.Y,Level),Tank?155.f:85.f,Tank?.22f:.10f,true)){if(Tank)++TrackContactsV64;else ++FootContactsV64;}
        }
    };
    const int32 Count=B->Units.Num();
    for(int32 I=0;I<FMath::Min(Count,32);++I){AWarfareFrontlineSoldier* U=B->Units[(ContactCursorV64+I)%Count];if(IsValid(U)&&U->IsAlive()&&U->bFieldEntered)Sample(U,false);}
    if(Count)ContactCursorV64=(ContactCursorV64+32)%Count;
    for(TActorIterator<AFrontlineTank> It(GetWorld());It;++It)if(It->IsOperational())Sample(*It,true);
    for(auto It=ContactPositionsV64.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
}

void UFrontlineWaterResponseV51::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
    if(!GetWorld()||!GetWorld()->HasBegunPlay())return;
    if(!HasBoundSurfaceV8219()){ResetBindingV82();return;}
    if(!NativeSceneIdV82.IsEmpty()&&!IsNativeBindingCurrentV82(true))return;
    if(const auto* B=GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>();B&&EncounterV53!=B->GetEncounterId())
    {EncounterV53=B->GetEncounterId();ClearBlood();ContactPositionsV64.Reset();for(auto& I:Impulses)I=FImpulse();Upload(GetWorld()->GetTimeSeconds());}
    const float Now=GetWorld()->GetTimeSeconds();
    // One scalar is updated per frame. Four source vectors change only on a
    // hit/profile change; there is no ripple actor, texture simulation or readback.
    SetScalarV82(TEXT("WaterClockV51"),Now);
    WeatherAge+=DeltaTime;
    if(WeatherAge<.1f)return;WeatherAge=0.f;
    if(!Weather.IsValid())Weather=AWarfareFrontlineWeather::Get(GetWorld());
    Rain=Weather.IsValid()?Weather->GetRainAmount():0.f;
    SampleContactsV64();
    const int32 Budget=UFrontlineRenderSettingsSubsystem::EffectsBudgetV43()<.5f?2:8;
    if(Budget!=MaxImpulses){MaxImpulses=Budget;Upload(Now);}
    else SetScalarV82(TEXT("RainV51"),Rain);
    const int32 BloodBudget=Budget==2?2:8;
    if(BloodBudget!=MaxBloodSources){MaxBloodSources=BloodBudget;UploadBlood(Now);}
    bool Expired=false;
    for(auto& Source:Blood)if(Source.Mass>0.f&&Now-Source.Began>=BloodLife){Source.Mass=0.f;Expired=true;}
    if(Expired)UploadBlood(Now);
}

TSharedRef<FJsonObject> UFrontlineWaterResponseV51::Describe() const
{
    auto J=MakeShared<FJsonObject>();const float Now=GetWorld()?GetWorld()->GetTimeSeconds():0;
    int32 Count=0;for(int32 I=0;I<MaxImpulses;++I)Count+=Impulses[I].Strength>0&&Now-Impulses[I].Began<Life;
    const bool Bound=HasBoundSurfaceV8219();
    J->SetBoolField(TEXT("bound"),Bound);
    J->SetStringField(TEXT("native_scene_v82"),NativeSceneIdV82);J->SetNumberField(TEXT("native_revision_v82"),NativeRevisionV82);
    J->SetNumberField(TEXT("bound_material_slots_v82"),WaterMID?1:0);
    J->SetNumberField(TEXT("primary_material_slot_v82"),WaterMaterialSlotV82);
    if(!NativeSceneIdV82.IsEmpty())
    {
        UTexture* Height=nullptr;
        if(WaterMID)WaterMID->GetTextureParameterValue(FMaterialParameterInfo(TEXT("NativeHeightGridV82")),Height);
        J->SetStringField(TEXT("native_height_texture_v82"),GetPathNameSafe(Height));
        J->SetBoolField(TEXT("current_native_binding_v82"),IsNativeBindingCurrentV82());
    }
    J->SetNumberField(TEXT("rain"),Rain);J->SetNumberField(TEXT("active_impulses"),Count);J->SetNumberField(TEXT("impulse_budget"),MaxImpulses);
    J->SetNumberField(TEXT("accepted_impacts"),Accepted);J->SetNumberField(TEXT("rejected_impacts"),Rejected);
    J->SetNumberField(TEXT("foot_contacts_v64"),FootContactsV64);J->SetNumberField(TEXT("track_contacts_v64"),TrackContactsV64);
    J->SetBoolField(TEXT("pressure_isolated_by_pool_and_level_v64"),true);
    int32 BloodCount=0;float Mass=0;
    for(int32 I=0;I<MaxBloodSources;++I)if(Blood[I].Mass>0&&Now>=Blood[I].Began&&Now-Blood[I].Began<BloodLife){++BloodCount;Mass+=Blood[I].Mass;}
    J->SetNumberField(TEXT("blood_deposits"),BloodDeposits);J->SetNumberField(TEXT("blood_rejected"),BloodRejected);
    J->SetNumberField(TEXT("corpse_seep_deposits"),CorpseSeepDeposits);J->SetNumberField(TEXT("drop_deposits"),BloodDeposits-CorpseSeepDeposits);
    J->SetNumberField(TEXT("blood_runoff_deposits"),BloodRunoffDeposits);J->SetNumberField(TEXT("blood_merged"),BloodMerged);
    J->SetNumberField(TEXT("active_sources"),BloodCount);J->SetNumberField(TEXT("total_mass"),Mass);J->SetNumberField(TEXT("blood_source_budget"),MaxBloodSources);
    J->SetStringField(TEXT("material"),WaterMID?WaterMID->GetPathName():TEXT("missing"));
#if WITH_EDITOR
    // Review-only inspection of the actual displayed LOD buffer, not the
    // authored mesh description or an assumed vertex-colour encoding.
    if((FParse::Param(FCommandLine::Get(),TEXT("WFV53Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterReview")))&&WaterSurface)
    {
        const UStaticMesh* Mesh=WaterSurface->GetStaticMesh();const auto* Render=Mesh?Mesh->GetRenderData():nullptr;
        const auto* LOD=Render&&Render->LODResources.Num()?&Render->LODResources[0]:nullptr;
        J->SetStringField(TEXT("water_mesh"),GetPathNameSafe(Mesh));
        TArray<TSharedPtr<FJsonValue>> Probes;
        if(LOD)
        {
            const auto& Positions=LOD->VertexBuffers.PositionVertexBuffer;
            const auto* Colors=&LOD->VertexBuffers.ColorVertexBuffer;
            const bool Overridden=WaterSurface->LODData.Num()>0&&WaterSurface->LODData[0].OverrideVertexColors;
            if(Overridden)Colors=WaterSurface->LODData[0].OverrideVertexColors;
            const bool Readable=Positions.GetVertexData()&&Colors->GetVertexData()&&Positions.GetNumVertices()==Colors->GetNumVertices();
            J->SetBoolField(TEXT("water_vertex_probe_readable"),Readable);J->SetBoolField(TEXT("water_vertex_color_override"),Overridden);
            J->SetNumberField(TEXT("water_lod0_vertices"),Positions.GetNumVertices());
            for(int32 I=0;Readable&&I<MaxBloodSources;++I)
            {
                const auto& Source=Blood[I];if(Source.Mass<=0.f)continue;
                const FVector Centre(Source.Position.X,Source.Position.Y,Source.Level);float Nearest=MAX_flt;uint32 Index=0;
                int32 NearVertices=0,GroupMatches=0,HeightMatches=0;FVector Closest=FVector::ZeroVector;
                for(uint32 V=0;V<Positions.GetNumVertices();++V)
                {
                    const FVector P=WaterSurface->GetComponentTransform().TransformPosition(FVector(Positions.VertexPosition(V)));
                    const float D=float(FVector::DistSquared(P,Centre));
                    if(D<Nearest){Nearest=D;Index=V;Closest=P;}
                    if(FVector2D::DistSquared(FVector2D(P),Source.Position)<=FMath::Square(Source.Radius))
                    {
                        ++NearVertices;const bool Height=FMath::Abs(P.Z-Source.Level)<=3.f;HeightMatches+=Height;
                        GroupMatches+=Height&&FMath::Abs(Colors->VertexColor(V).G/255.f-Source.Group)<.5f/255.f;
                    }
                }
                const FColor Raw=Colors->VertexColor(Index);const FLinearColor U=WaterMID->K2_GetVectorParameterValue(BloodPositions[I]);
                const FLinearColor S=WaterMID->K2_GetVectorParameterValue(BloodStates[I]);auto P=MakeShared<FJsonObject>();
                P->SetNumberField(TEXT("slot"),I);P->SetStringField(TEXT("source_position"),Centre.ToString());P->SetNumberField(TEXT("age"),Now-Source.Began);
                P->SetNumberField(TEXT("mass"),Source.Mass);P->SetNumberField(TEXT("source_group"),Source.Group);P->SetNumberField(TEXT("radius_cm"),Source.Radius);
                P->SetStringField(TEXT("source_uniform"),U.ToString());P->SetStringField(TEXT("state_uniform"),S.ToString());
                P->SetNumberField(TEXT("nearest_vertex_index"),Index);P->SetNumberField(TEXT("nearest_distance_cm"),FMath::Sqrt(Nearest));
                P->SetStringField(TEXT("nearest_world_position"),Closest.ToString());P->SetStringField(TEXT("nearest_raw_rgba"),Raw.ToString());
                P->SetNumberField(TEXT("nearest_vertex_group"),Raw.G/255.f);P->SetNumberField(TEXT("nearest_vertex_depth"),Raw.R/255.f);
                P->SetNumberField(TEXT("nearest_raw_b"),Raw.B);P->SetBoolField(TEXT("regional_medium"),Raw.B>127);
                P->SetNumberField(TEXT("nearby_vertices"),NearVertices);P->SetNumberField(TEXT("height_eligible_vertices"),HeightMatches);P->SetNumberField(TEXT("fully_eligible_vertices"),GroupMatches);
                Probes.Add(MakeShared<FJsonValueObject>(P));
            }
        }
        J->SetArrayField(TEXT("blood_vertex_probes"),Probes);
    }
#endif
    J->SetStringField(TEXT("model"),TEXT("single-layer water; analytic damped surface waves and sediment response; no fluid-volume simulation"));
    return J;
}

void UFrontlineWaterResponseV51::RunLifecycleChecksV8219(UWorld* World,TFunctionRef<void(const FString&,bool)> Check)
{
    auto* Battle=World?World->GetAuthGameMode<AWarfareFrontlineGameMode>():nullptr;
    FString User;
    const bool ReviewDirectory=FrontlineUltimateArchiveV8215::ReviewUserDir(User);
    const FString ReviewRoot=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8219/Runtime/Run/"));
    const FString Stamp=User.StartsWith(ReviewRoot,ESearchCase::IgnoreCase)?User.Mid(ReviewRoot.Len()):FString();
    const bool ExactReview=ReviewDirectory&&Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
        &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
    const bool Safe=World&&World->IsGameWorld()&&Battle&&Battle->IsProgressionVolatile()
        &&Battle->Phase==EFrontlinePhase::Fighting&&!WarfareFRRUCampaignV82::Active(Battle)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&ExactReview;
    Check(TEXT("v8219_water_lifecycle_strict_disposable_uk_de_world"),Safe);if(!Safe)return;
    auto* Response=FindResponse(World);
    auto* Ground=Response?Cast<AWarfareFrontlineGround>(Response->GetOwner()):nullptr;
    const bool Bound=Ground&&Response->HasBoundSurfaceV8219()&&Ground->BasinWaterV44==Response->WaterSurface
        &&IsValid(Response->WaterMID->GetMaterial());
    Check(TEXT("v8219_water_lifecycle_actual_displayed_surface"),Bound);if(!Bound)return;
    FVector Wet;float Level=0.f,Depth=0.f;bool Found=false;
    for(float X=-FrontlineBattlefield::GoalHalfLength;!Found&&X<FrontlineBattlefield::GoalHalfLength;X+=180.f)
        for(float Y=-2300.f;!Found&&Y<2300.f;Y+=180.f)
            if(AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth)&&Depth>.8f){Wet=FVector(X,Y,Level);Found=true;}
    Check(TEXT("v8219_water_lifecycle_real_wet_contact"),Found);if(!Found)return;
    const FString MeshPath=Response->WaterSurface->GetStaticMesh()->GetPathName();
    const FString GraphPath=Response->WaterMID->GetMaterial()->GetPathName();
    FImpulse SavedImpulses[8];FBloodSource SavedBlood[8];
    for(int32 I=0;I<8;++I){SavedImpulses[I]=Response->Impulses[I];SavedBlood[I]=Response->Blood[I];}
    const auto SavedContacts=Response->ContactPositionsV64;
    const int32 SavedCounts[]={Response->Accepted,Response->Rejected,Response->FootContactsV64,Response->TrackContactsV64,
        Response->BloodDeposits,Response->BloodRejected,Response->BloodRunoffDeposits,Response->BloodMerged,Response->CorpseSeepDeposits,Response->ContactCursorV64};
    const int32 SavedImpulseBudget=Response->MaxImpulses,SavedBloodBudget=Response->MaxBloodSources;
    const float SavedRain=Response->Rain,SavedWeatherAge=Response->WeatherAge;
    const auto SavedWeather=Response->Weather;const FGuid SavedEncounter=Response->EncounterV53;
    ON_SCOPE_EXIT
    {
        if(!Response->HasBoundSurfaceV8219()||Ground->BasinWaterV44!=Response->WaterSurface)Ground->BindWaterV44();
        if(Response->HasBoundSurfaceV8219())
        {
            for(int32 I=0;I<8;++I){Response->Impulses[I]=SavedImpulses[I];Response->Blood[I]=SavedBlood[I];}
            Response->ContactPositionsV64=SavedContacts;Response->Accepted=SavedCounts[0];Response->Rejected=SavedCounts[1];
            Response->FootContactsV64=SavedCounts[2];Response->TrackContactsV64=SavedCounts[3];
            Response->BloodDeposits=SavedCounts[4];Response->BloodRejected=SavedCounts[5];Response->BloodRunoffDeposits=SavedCounts[6];
            Response->BloodMerged=SavedCounts[7];Response->CorpseSeepDeposits=SavedCounts[8];Response->ContactCursorV64=SavedCounts[9];
            Response->MaxImpulses=SavedImpulseBudget;Response->MaxBloodSources=SavedBloodBudget;
            Response->Rain=SavedRain;Response->WeatherAge=SavedWeatherAge;Response->Weather=SavedWeather;Response->EncounterV53=SavedEncounter;
            Response->Upload(World->GetTimeSeconds());Response->UploadBlood(World->GetTimeSeconds());
        }
    };
    Check(TEXT("v8219_water_lifecycle_live_events_accept_before_destroy"),NotifyImpact(World,Wet,400.f,.8f)&&NotifyBlood(World,Wet,.2f));
    Ground->BasinWaterV44->DestroyComponent();
    Check(TEXT("v8219_destroyed_water_component_rejects_impact_and_blood"),!NotifyImpact(World,Wet,400.f,.8f)&&!NotifyBlood(World,Wet,.2f));
    Response->TickComponent(.01f,LEVELTICK_All,nullptr);
    Check(TEXT("v8219_invalid_water_tick_clears_surface_mid_and_sources"),!Response->WaterSurface&&!Response->WaterMID
        &&Response->Describe()->GetNumberField(TEXT("active_impulses"))==0&&Response->Describe()->GetNumberField(TEXT("active_sources"))==0);
    Ground->BasinWaterV44=nullptr;Bind(Ground);
    Check(TEXT("v8219_empty_water_bind_clears_and_rejects_new_events"),!Response->HasBoundSurfaceV8219()
        &&!NotifyImpact(World,Wet,400.f,.8f)&&!NotifyBlood(World,Wet,.2f));
    Ground->BindWaterV44();
    const bool Restored=Response->HasBoundSurfaceV8219()&&Ground->BasinWaterV44==Response->WaterSurface
        &&Response->WaterSurface->GetStaticMesh()->GetPathName()==MeshPath&&Response->WaterMID->GetMaterial()->GetPathName()==GraphPath;
    Check(TEXT("v8219_normal_water_rebind_restores_same_mesh_and_graph"),Restored);
    Check(TEXT("v8219_normal_water_rebind_restores_visual_events"),Restored&&NotifyImpact(World,Wet,400.f,.8f)&&NotifyBlood(World,Wet,.2f));
}

void UFrontlineWaterResponseV51::RunChecks(UWorld* World,TFunctionRef<void(const FString&,bool)> Check)
{
    const auto* B=World?Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode()):nullptr;FString User;
    const bool Isolated=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV51Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty();
    Check(TEXT("v51_water_checks_require_isolated_world"),Isolated);if(!Isolated)return;
    auto* R=FindResponse(World);Check(TEXT("v51_water_displayed_material_has_response_owner"),R&&R->WaterMID&&R->WaterSurface&&R->WaterSurface->GetMaterial(0)==R->WaterMID);if(!R)return;
    FVector WetPoint;bool Found=false;float Level=0,Depth=0;
    for(float X=-FrontlineBattlefield::GoalHalfLength;!Found&&X<FrontlineBattlefield::GoalHalfLength;X+=280.f)
        for(float Y=-2300;!Found&&Y<2300;Y+=210.f)
            if(AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth)&&Depth>.8f){WetPoint=FVector(X,Y,Level);Found=true;}
    Check(TEXT("v51_water_response_uses_actual_physical_pool"),Found);if(!Found)return;
    FImpulse Before[4];for(int32 I=0;I<4;++I)Before[I]=R->Impulses[I];const int32 A=R->Accepted,D=R->Rejected;
    bool Admitted=true;for(int32 I=0;I<9;++I)Admitted&=R->Admit(WetPoint,400.f,.8f);
    Check(TEXT("v51_water_multiple_blasts_stay_within_fixed_impulse_budget"),Admitted&&R->Describe()->GetNumberField(TEXT("active_impulses"))<=R->MaxImpulses);
    Check(TEXT("v51_water_rejects_invalid_and_remote_impacts"),!R->Admit(FVector(MAX_flt,MAX_flt,0),400,.8f)&&!R->Admit(WetPoint,-1,.8f));
    Check(TEXT("v51_water_stays_noncolliding_and_no_new_shadow_caster"),R->WaterSurface->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!R->WaterSurface->CastShadow);
    for(int32 I=0;I<4;++I)R->Impulses[I]=Before[I];R->Accepted=A;R->Rejected=D;R->Upload(World->GetTimeSeconds());
}

void UFrontlineWaterResponseV51::RunChecksV64(UWorld* World,TFunctionRef<void(const FString&,bool)> Check)
{
    const auto* B=World?World->GetAuthGameMode<AWarfareFrontlineGameMode>():nullptr;FString User;
    const bool Safe=B&&B->IsProgressionVolatile()&&(FParse::Param(FCommandLine::Get(),TEXT("WFV64Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV65Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterReview")))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty();
    Check(TEXT("v64_water_disposable_profile"),Safe);if(!Safe)return;
    auto* R=FindResponse(World);
    const bool Bound=R&&R->WaterMID&&R->WaterSurface&&R->WaterSurface->GetMaterial(0)==R->WaterMID
        &&(R->WaterMID->GetMaterial()->GetName()==TEXT("M_BasinWater_V64")||R->WaterMID->GetMaterial()->GetName()==TEXT("M_BasinWater_V65")||R->WaterMID->GetMaterial()->GetName()==TEXT("M_BasinWater_V67")||(R->WaterMID->GetMaterial()->GetName()==TEXT("M_BasinWater_V72")||R->WaterMID->GetMaterial()->GetName()==TEXT("M_BasinWater_V75")));
    Check(TEXT("v64_live_mesh_uses_new_single_layer_water"),Bound);if(!Bound)return;
    FImpulse Saved[8];for(int32 I=0;I<8;++I)Saved[I]=R->Impulses[I];
    const int32 Budget=R->MaxImpulses,A=R->Accepted,D=R->Rejected;
    ON_SCOPE_EXIT{for(int32 I=0;I<8;++I)R->Impulses[I]=Saved[I];R->MaxImpulses=Budget;R->Accepted=A;R->Rejected=D;R->Upload(World->GetTimeSeconds());};
    FVector Wet;float Level=0,Depth=0,Group=-1;bool Found=false;
    for(float X=-FrontlineBattlefield::GoalHalfLength;!Found&&X<FrontlineBattlefield::GoalHalfLength;X+=180.f)
        for(float Y=-2300;!Found&&Y<2300;Y+=180.f)
            if(AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth,&Group)&&Depth>.8f){Wet=FVector(X,Y,Level);Found=true;}
    Check(TEXT("v64_water_test_uses_real_clipped_pool"),Found);if(!Found)return;
    for(int32 Limit:{2,8})
    {
        R->MaxImpulses=Limit;for(auto& I:R->Impulses)I=FImpulse();
        const bool Blast=R->Admit(Wet,400.f,.8f,false);
        bool Contacts=true;for(int32 I=0;I<24;++I)Contacts&=R->Admit(Wet,85.f,.1f,true);
        const int32 ContactSlot=Limit==2?1:4;const auto Body=R->WaterMID->K2_GetVectorParameterValue(FName(*FString::Printf(TEXT("WaterBody%dV64"),ContactSlot)));
        Check(FString::Printf(TEXT("v64_water_budget_%d_contacts_preserve_blast"),Limit),Blast&&Contacts&&R->Impulses[0].Strength==.8f&&!R->Impulses[0].Contact&&R->Impulses[ContactSlot].Contact
            &&R->Describe()->GetNumberField(TEXT("active_impulses"))<=Limit);
        Check(FString::Printf(TEXT("v64_water_budget_%d_binds_real_pool_and_level"),Limit),FMath::IsNearlyEqual(Body.R,Group,.00001f)&&FMath::IsNearlyEqual(Body.G,Level,.01f)&&Body.B==1.f);
    }
    Check(TEXT("v64_dry_ground_and_airborne_contacts_rejected"),!R->Admit(FVector(90000,90000,0),85,.1f,true)&&!R->Admit(Wet+FVector(0,0,1500),85,.1f,true));
    Check(TEXT("v64_water_no_collision_or_new_shadow_caster"),R->WaterSurface->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!R->WaterSurface->CastShadow);
}

void UFrontlineWaterResponseV51::RunChecksV53(UWorld* World,TFunctionRef<void(const FString&,bool)> Check)
{
    const auto* B=World?World->GetAuthGameMode<AWarfareFrontlineGameMode>():nullptr;FString User;
    const bool Isolated=B&&B->IsProgressionVolatile()&&(FParse::Param(FCommandLine::Get(),TEXT("WFV53Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV65Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterReview")))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty();
    Check(TEXT("v53_blood_checks_require_isolated_world"),Isolated);if(!Isolated)return;
    auto* R=FindResponse(World);
    const bool Bound=R&&R->WaterMID&&R->WaterSurface&&R->WaterSurface->GetMaterial(0)==R->WaterMID
        &&R->WaterMID->GetMaterial()&&(R->WaterMID->GetMaterial()->GetName()==TEXT("M_BasinWater_V53")||R->WaterMID->GetMaterial()->GetName()==TEXT("M_BasinWater_V65")||R->WaterMID->GetMaterial()->GetName()==TEXT("M_BasinWater_V67")||(R->WaterMID->GetMaterial()->GetName()==TEXT("M_BasinWater_V72")||R->WaterMID->GetMaterial()->GetName()==TEXT("M_BasinWater_V75")));
    Check(TEXT("v53_blood_uses_displayed_water_material"),Bound);if(!Bound)return;
    FBloodSource Saved[8];for(int32 I=0;I<8;++I)Saved[I]=R->Blood[I];
    const int32 SavedBudget=R->MaxBloodSources,A=R->BloodDeposits,D=R->BloodRejected,Runoff=R->BloodRunoffDeposits,Merged=R->BloodMerged,Seep=R->CorpseSeepDeposits;
    ON_SCOPE_EXIT
    {
        for(int32 I=0;I<8;++I)R->Blood[I]=Saved[I];R->MaxBloodSources=SavedBudget;
        R->BloodDeposits=A;R->BloodRejected=D;R->BloodRunoffDeposits=Runoff;R->BloodMerged=Merged;R->CorpseSeepDeposits=Seep;
        R->UploadBlood(World->GetTimeSeconds());
    };
    struct FPoolPoint {FVector Position;float Group;};TArray<FPoolPoint> Pools;
    float Level=0.f,Depth=0.f,Group=-1.f;bool HasOtherGroup=false;
    for(float X=-FrontlineBattlefield::GoalHalfLength;X<FrontlineBattlefield::GoalHalfLength&&(Pools.Num()<12||!HasOtherGroup);X+=180.f)
        for(float Y=-2300.f;Y<2300.f&&(Pools.Num()<12||!HasOtherGroup);Y+=180.f)
            if(AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth,&Group)&&Depth>.8f)
            {
                const bool Other=Pools.Num()>0&&FMath::Abs(Pools[0].Group-Group)>.5f/255.f;
                if(Pools.Num()<12||Other){Pools.Add({FVector(X,Y,Level),Group});HasOtherGroup|=Other;}
            }
    Check(TEXT("v53_blood_checks_find_real_clipped_water"),Pools.Num()>0);if(Pools.IsEmpty())return;
    const auto First=Pools[0];R->MaxBloodSources=8;R->ClearBlood();
    const bool FirstDrop=NotifyBlood(World,First.Position,.2f,165.f);
    const bool LaterSeep=NotifyBlood(World,First.Position,.3f,165.f,true);
    Check(TEXT("v53_actual_deposits_accumulate_without_restarting_diffusion"),FirstDrop&&LaterSeep&&R->BloodDeposits==2&&R->BloodMerged==1&&R->CorpseSeepDeposits==1
        &&FMath::IsNearlyEqual(R->Blood[0].Mass,.5f,.0001f)&&R->Describe()->GetNumberField(TEXT("active_sources"))==1);
    const bool Air=NotifyBlood(World,First.Position+FVector(0,0,300),.5f);
    const FVector Dry(FrontlineBattlefield::GoalHalfLength+20000.f,12000.f,0.f);
    const bool DryAccepted=NotifyBlood(World,FVector(Dry.X,Dry.Y,AWarfareFrontlineGround::HeightAt(Dry.X,Dry.Y)),.5f);
    const bool Invalid=NotifyBlood(World,FVector(MAX_flt,MAX_flt,0),.5f)||NotifyBlood(World,First.Position,-1.f)||NotifyBlood(World,First.Position,.5f,0.f);
    Check(TEXT("v53_blood_rejects_airborne_remote_dry_and_invalid_sources"),!Air&&!DryAccepted&&!Invalid&&FMath::IsNearlyEqual(R->Blood[0].Mass,.5f,.0001f));
    int32 OtherIndex=INDEX_NONE;for(int32 I=1;I<Pools.Num();++I)if(FMath::Abs(Pools[I].Group-First.Group)>.5f/255.f){OtherIndex=I;break;}
    Check(TEXT("v53_blood_fixture_has_distinct_hydraulic_groups"),OtherIndex!=INDEX_NONE);
    if(OtherIndex!=INDEX_NONE)
    {
        const auto Other=Pools[OtherIndex];const bool Deposited=NotifyBlood(World,Other.Position,.4f);
        bool ExactOther=false;for(const auto& S:R->Blood)ExactOther|=S.Mass>0&&FMath::IsNearlyEqual(S.Group,Other.Group,.0001f)&&FMath::IsNearlyEqual(S.Level,float(Other.Position.Z),.01f);
        Check(TEXT("v53_blood_keeps_separate_real_pool_identity_and_mass"),Deposited&&ExactOther&&FMath::IsNearlyEqual(R->Blood[0].Mass,.5f,.0001f)
            &&FMath::IsNearlyEqual(R->Blood[0].Group,First.Group,.0001f));
    }
    R->ClearBlood();NotifyBlood(World,First.Position,1.f);R->Blood[0].Began=World->GetTimeSeconds()-105.f;
    NotifyBlood(World,First.Position,.2f);
    Check(TEXT("v53_fresh_blood_does_not_merge_into_an_expiring_deposit"),R->Blood[1].Mass>.19f&&World->GetTimeSeconds()-R->Blood[1].Began<.1f);
    for(const int32 Budget:{2,8})
    {
        R->ClearBlood();R->MaxBloodSources=Budget;
        bool Accepted=true;for(int32 I=0;I<32;++I)Accepted&=NotifyBlood(World,Pools[I%Pools.Num()].Position,.8f);
        bool Finite=true;int32 Count=0;for(const auto& S:R->Blood)if(S.Mass>0){++Count;Finite&=FMath::IsFinite(S.Mass)&&S.Mass<=4.f&&S.Radius<=240.f&&S.Group>=0.f&&S.Group<=1.f;}
        Check(FString::Printf(TEXT("v53_blood_budget_%d_is_bounded_and_finite"),Budget),Accepted&&Finite&&Count<=Budget&&R->BloodDeposits==32);
    }
    R->ClearBlood();const auto Cleared=R->Describe();
    Check(TEXT("v53_blood_encounter_reset_clears_sources_and_counters"),Cleared->GetNumberField(TEXT("active_sources"))==0&&Cleared->GetNumberField(TEXT("total_mass"))==0
        &&Cleared->GetNumberField(TEXT("corpse_seep_deposits"))==0&&Cleared->GetNumberField(TEXT("blood_deposits"))==0);
    UE_LOG(LogTemp,Display,TEXT("WF_V53_WATER_CHECK_POOL position=%s group=%.8f real_pools=%d"),*First.Position.ToString(),First.Group,Pools.Num());
}


