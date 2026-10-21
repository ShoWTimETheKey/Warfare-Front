#include "WarfareFrontlineWoundFX.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAppearanceV29.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineMaterialV27.h"
#include "WarfareFrontlineWaterV51.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include <limits>

namespace
{
constexpr int32 BloodMarkCountV86=64,TransientBloodMarkCountV86=20;
constexpr float BloodDepositLifetimeV86=110.f,BloodRetirementSecondsV86=.32f;
bool BloodStepV828(const FVector& P,FVector& Next,TFunctionRef<float(float,float)> Height)
{
    // Resolve the real small rut, not a 38 cm chord across several ridges.
    // Every 2 cm contact must descend continuously; neither a steep wall nor
    // a locally rising saddle grants an invented flow through the surface.
    const float Z=Height(P.X,P.Y);if(!FMath::IsFinite(Z))return false;
    float Drop=.035f;bool Found=false;
    for(int32 I=0;I<24;++I)
    {
        const float Angle=2*PI*I/24.f;FVector Q=P+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0)*12.f;
        Q.Z=Height(Q.X,Q.Y);const float D=Z-Q.Z;
        if(!FMath::IsFinite(Q.Z)||D<=Drop||D>6.f)continue;
        bool Continuous=true;float Previous=Z;
        for(int32 J=1;J<=6;++J)
        {
            const FVector Mid=FMath::Lerp(P,Q,J/6.f);const float H=Height(Mid.X,Mid.Y);
            Continuous&=FMath::IsFinite(H)&&H<=Previous+.008f&&Previous-H<=1.5f;
            Previous=H;
            if(!Continuous)break;
        }
        if(Continuous){Drop=D;Next=Q;Found=true;}
    }
    return Found;
}
}

AWarfareFrontlineWoundFX::AWarfareFrontlineWoundFX()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PostPhysics;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Bounded wound presentation")));
}

AWarfareFrontlineWoundFX* AWarfareFrontlineWoundFX::Get(UWorld* World)
{
    if (!World) return nullptr;
    for (TActorIterator<AWarfareFrontlineWoundFX> It(World);It;++It) if (IsValid(*It)) return *It;
    return World->SpawnActor<AWarfareFrontlineWoundFX>();
}

void AWarfareFrontlineWoundFX::BeginPlay()
{
    Super::BeginPlay();
    auto* Quad=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    auto* Mist=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V84411/Materials/M_BloodMist_V84411.M_BloodMist_V84411"));
    if(!Mist)Mist=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V75/Materials/M_BloodMist_V75.M_BloodMist_V75"));
    if(!Mist)Mist=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V16/Materials/M_BloodMist_V16.M_BloodMist_V16"));
    auto* Stain=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V84411/Materials/M_BloodDeposit_V84411.M_BloodDeposit_V84411"));
    if(!Stain)Stain=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V8211/Materials/M_BloodDeposit_V8211.M_BloodDeposit_V8211"));
    if(!Stain)Stain=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V75/Materials/M_BloodStain_V75.M_BloodStain_V75"));
    if(!Stain)Stain=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V16/Materials/M_BloodStain_V16.M_BloodStain_V16"));
    if (!Quad || !Mist || !Stain)
    {
        UE_LOG(LogTemp,Error,TEXT("WF_V16_WOUND_ASSETS_MISSING: import and cook V16 FX before visual acceptance"));
        SetActorTickEnabled(false);return;
    }
    Bursts.SetNum(24);MarkState.SetNum(BloodMarkCountV86);PartState.SetNum(8);
    for (int32 I=0;I<72;++I)
    {
        auto* Plane=NewObject<UStaticMeshComponent>(this,*FString::Printf(TEXT("BloodMist%d"),I));
        Plane->SetupAttachment(RootComponent);Plane->SetStaticMesh(Quad);
        Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);Plane->SetCastShadow(false);Plane->SetVisibility(false);Plane->SetReceivesDecals(false);
        auto* Material=UMaterialInstanceDynamic::Create(Mist,this);Material->SetScalarParameterValue(TEXT("Density"),0);
        Plane->SetMaterial(0,Material);Plane->RegisterComponent();Mists.Add(Plane);MistMaterials.Add(Material);
    }
    for (int32 I=0;I<BloodMarkCountV86;++I)
    {
        auto* Decal=NewObject<UDecalComponent>(this,*FString::Printf(TEXT("GroundBlood%d"),I));
        Decal->SetupAttachment(RootComponent);Decal->SetVisibility(false);Decal->SetFadeScreenSize(.0005f);
        auto* Material=UMaterialInstanceDynamic::Create(Stain,this);Material->SetScalarParameterValue(TEXT("Density"),0);
        Decal->SetDecalMaterial(Material);Decal->RegisterComponent();Marks.Add(Decal);MarkMaterials.Add(Material);
    }
    RunoffsV828.SetNum(48);
    for(int32 I=0;I<48;++I)
    {
        auto* Decal=NewObject<UDecalComponent>(this,*FString::Printf(TEXT("SurfaceBloodRunoff%d"),I));
        Decal->SetupAttachment(RootComponent);Decal->SetVisibility(false);Decal->SetFadeScreenSize(.0003f);
        auto* Material=UMaterialInstanceDynamic::Create(Stain,this);
        Material->SetScalarParameterValue(TEXT("Density"),0);Material->SetScalarParameterValue(TEXT("SeepV65"),2);
        Decal->SetDecalMaterial(Material);Decal->RegisterComponent();RunoffDecalsV828.Add(Decal);RunoffMaterialsV828.Add(Material);
    }
    auto* Cloud=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V828/Materials/M_BloodVolume_V828.M_BloodVolume_V828"));
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    if(Cloud&&Cube)
    {
        BlastCloudStateV828.SetNum(8);
        for(int32 I=0;I<8;++I)
        {
            auto* Mesh=NewObject<UStaticMeshComponent>(this,*FString::Printf(TEXT("BlastBloodVolume%d"),I));
            Mesh->SetupAttachment(RootComponent);Mesh->SetStaticMesh(Cube);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Mesh->SetCastShadow(false);Mesh->SetReceivesDecals(false);Mesh->SetVisibility(false);
            auto* Mat=UMaterialInstanceDynamic::Create(Cloud,this);Mat->SetScalarParameterValue(TEXT("VolumeProfile"),2);
            Mat->SetVectorParameterValue(TEXT("DustColor"),FLinearColor(.108f,.00585f,.00162f));
            Mesh->SetMaterial(0,Mat);Mesh->RegisterComponent();BlastCloudsV828.Add(Mesh);BlastCloudMaterialsV828.Add(Mat);
        }
    }
    for (int32 I=0;I<8;++I)
    {
        auto* Piece=NewObject<UStaticMeshComponent>(this,*FString::Printf(TEXT("ActualSingleLimb%d"),I));
        Piece->SetupAttachment(RootComponent);Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Piece->SetCastShadow(true);Piece->SetVisibility(false);Piece->SetReceivesDecals(false);Piece->RegisterComponent();Parts.Add(Piece);
    }
    bReady=true;
    if(const auto* Battle=GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>())SeepEncounterV53=Battle->GetEncounterId();
}

int32 AWarfareFrontlineWoundFX::AddMark(const FVector& Position,const FVector& Direction,float Size,float Opacity,int32 Seed,bool bFatalDeposit,bool bSeep,float PoolMassV84411)
{
    if(!bReady||Position.ContainsNaN()||!FMath::IsFinite(Size)||!FMath::IsFinite(Opacity))return INDEX_NONE;
    // A live corpse source owns its receiving pool. The old circular writer
    // let terminal runoff deposits steal that pool; its identity mismatch then
    // re-emitted the same corpse's pool from Spread=0, sometimes every frame.
    const int32 I=ReserveMarkV86(bFatalDeposit);if(I==INDEX_NONE)return INDEX_NONE;
    if(bFatalDeposit)++FatalMarkCursor;else ++MarkCursor;
    auto& Mark=MarkState[I];Mark=FFrontlineBloodMark();Mark.Age=0;Mark.Opacity=Opacity;Mark.SpreadSeconds=bSeep?6.f:.32f;
    Mark.IdentityV84411=++BloodMarkIdentityV84411;Mark.bMassDrivenV84411=bSeep;Mark.MassV84411=FMath::Max(0.f,PoolMassV84411);
    auto* B=GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>();
    const auto Height=[B](float X,float Y){return FrontlineInfantryV45::SurfaceHeight(B,FVector(X,Y,0));};
    const float Z=Height(Position.X,Position.Y);
    const FVector N=FVector(Height(Position.X-2,Position.Y)-Height(Position.X+2,Position.Y),
        Height(Position.X,Position.Y-2)-Height(Position.X,Position.Y+2),4).GetSafeNormal();
    FVector Along=FVector::VectorPlaneProject(Direction,N).GetSafeNormal();
    if(Along.IsNearlyZero())Along=FVector::VectorPlaneProject(FVector::ForwardVector,N).GetSafeNormal();
    const FRotator Rotation=FRotationMatrix::MakeFromXZ(-N,Along).Rotator();
    Marks[I]->SetWorldLocation(FVector(Position.X,Position.Y,Z)+N*.55f);
    // Project only a shallow ground layer. The long axis follows the arriving
    // droplets. The dense liquid boundary follows the receiver's light and
    // does not add a fuzzy soil-absorption halo.
    const float HalfWidth=Size*(.86f+.055f*FMath::Sin(float(Seed)*1.73f));
    Marks[I]->SetWorldRotation(Rotation);Marks[I]->DecalSize=FVector(4,HalfWidth,Size);
    Marks[I]->SetVisibility(true);Marks[I]->MarkRenderStateDirty();
    MarkMaterials[I]->SetScalarParameterValue(TEXT("Seed"),float(Seed%31));
    MarkMaterials[I]->SetScalarParameterValue(TEXT("Density"),Opacity);
    MarkMaterials[I]->SetScalarParameterValue(TEXT("Freshness"),1.f);
    MarkMaterials[I]->SetScalarParameterValue(TEXT("SpreadV65"),0.f);
    MarkMaterials[I]->SetScalarParameterValue(TEXT("SeepV65"),bSeep?1.f:0.f);
    if((bFatalDeposit && FatalMarkCursor<=2) || (!bFatalDeposit && MarkCursor<=2))
        UE_LOG(LogTemp,Display,TEXT("WF_V16_BLOOD_GROUND fatal=%d position=%s normal=%s halfdepth_cm=4 halfwidth_cm=%.2f halflength_cm=%.2f density=%.3f"),
            int32(bFatalDeposit),*Marks[I]->GetComponentLocation().ToString(),*N.ToString(),HalfWidth,Size,Opacity);
    return I;
}

bool AWarfareFrontlineWoundFX::IsMarkSourceOwnedV86(int32 Slot) const
{
    if(!MarkState.IsValidIndex(Slot))return false;
    const uint32 Identity=MarkState[Slot].IdentityV84411;
    return Identity!=0&&CorpseSeepsV53.ContainsByPredicate([Slot,Identity](const auto& S)
    {return S.Unit.IsValid()&&S.Age<=18.f&&S.Remaining>.001f
        &&S.GroundMarkV84411==Slot&&S.GroundMarkIdentityV84411==Identity;});
}

int32 AWarfareFrontlineWoundFX::ReserveMarkV86(bool Fatal)
{
    const int32 Begin=Fatal?TransientBloodMarkCountV86:0,End=Fatal?MarkState.Num():TransientBloodMarkCountV86;
    int32 Oldest=INDEX_NONE;bool Retiring=false;
    for(int32 I=Begin;I<End;++I)
    {
        const auto& M=MarkState[I];
        if(M.Age>=BloodDepositLifetimeV86&&!IsMarkSourceOwnedV86(I))return I;
        Retiring|=M.RetirementAgeV86>=0;
        if(!IsMarkSourceOwnedV86(I)&&M.RetirementAgeV86<0
            &&(Oldest==INDEX_NONE||M.Age>MarkState[Oldest].Age))Oldest=I;
    }
    // Reclaim ended deposits gradually before reuse. Waiting callers retain
    // their finite reservoir; an allocation never steals a visible wet source.
    if(!Retiring&&Oldest!=INDEX_NONE)MarkState[Oldest].RetirementAgeV86=0;
    return INDEX_NONE;
}

void AWarfareFrontlineWoundFX::FatalHit(AWarfareFrontlineSoldier* Unit,const FVector& Direction,bool bBlast)
{
    if (!bReady || !Unit || Unit->IsAlive()) return;
    if(RegisteredCorpseSeepsV86.Contains(Unit))return;
    RegisteredCorpseSeepsV86.Add(Unit);
    FrontlineMaterialV27::Fatal(Unit,Direction);
    Unit->Soldier->SetReceivesDecals(false);
    // Register the corpse, but sample only after the death path has published a
    // physical pose. FatalHit precedes DeathState initialization in ReceiveHit.
    if(!CorpseSeepsV53.ContainsByPredicate([Unit](const auto& S){return S.Unit.Get()==Unit;}))
    {
        if(CorpseSeepsV53.Num()>=24)CorpseSeepsV53.RemoveAt(0);
        auto& S=CorpseSeepsV53.AddDefaulted_GetRef();S.Unit=Unit;S.LastBone=Unit->Soldier->GetBoneLocation(TEXT("spine_03"));S.Remaining=bBlast?1.45f:1.2f;
    }
    const int32 I=BurstCursor++%Bursts.Num();auto& Burst=Bursts[I];
    Burst.Position=Unit->Soldier->GetBoneLocation(TEXT("spine_03"));
    Burst.Direction=Direction.GetSafeNormal();if(Burst.Direction.IsNearlyZero())Burst.Direction=-Unit->GetActorForwardVector();
    // Explosive casualties contribute to the detonation's dust in ResolveStrike.
    // They never emit a second, detached red plume from the animated torso.
    Burst.Age=bBlast?10.f:0.f;Burst.Size=.88f;Burst.bBlast=false;
    Burst.Seed=float((Unit->SquadId*17+Unit->MemberIndex*7+HitsShown*3)%41);
    const FVector Side=FVector::CrossProduct(FVector::UpVector,Burst.Direction).GetSafeNormal();
    Burst.Velocity=Burst.Direction*(135.f+Burst.Seed*.95f)+Side*(FMath::Sin(Burst.Seed*2.7f)*15.f)+FVector(0,0,18.f+FMath::Fmod(Burst.Seed,19.f));
    Burst.bDepositPending=false;Burst.bFatalDeposit=false;
    ++HitsShown;
    if(HitsShown<=3 || HitsShown%20==0)
        UE_LOG(LogTemp,Display,TEXT("WF_V26_FATAL_HIT squad=%d member=%d blast=%d hits=%d burst_budget=24 mark_budget=64"),
            Unit->SquadId,Unit->MemberIndex,int32(bBlast),HitsShown);
}

void AWarfareFrontlineWoundFX::FatalHeavyBulletV75(AWarfareFrontlineSoldier* Unit,const FVector& Direction)
{
    if(!Unit||RegisteredCorpseSeepsV86.Contains(Unit))return;
    FatalHit(Unit,Direction,false);
    if(!bReady||!Unit||Unit->IsAlive()||Bursts.IsEmpty())return;
    auto& Exit=Bursts[(BurstCursor-1+Bursts.Num())%Bursts.Num()];
    Exit.Size=1.72f;Exit.Velocity=Direction.GetSafeNormal()*425.f+FVector(0,0,42.f);
    // The ordinary casualty owns its physical pose and finite blood seep.
    // A second short exit spray uses the existing pooled budget, not explosion fire.
    const auto Original=Exit;auto& Second=Bursts[BurstCursor++%Bursts.Num()];
    Second=Original;Second.Position+=FVector(0,0,-21);Second.Size=.72f;
    Second.Velocity=Direction.GetSafeNormal()*245.f+FVector(0,0,70.f);
    Second.bDepositPending=false;Second.bFatalDeposit=false;
    if(!BlastCloudStateV828.IsEmpty())
    {
        auto& Aerosol=BlastCloudStateV828[BlastCloudCursorV828++%BlastCloudStateV828.Num()];
        Aerosol=FFrontlineBloodBurst();Aerosol.Age=0;Aerosol.Size=.56f;
        Aerosol.Position=Exit.Position+Direction.GetSafeNormal()*18.f;
        Aerosol.Velocity=Direction.GetSafeNormal()*240.f+FVector(0,0,15.f);
        Aerosol.Seed=Exit.Seed;
    }
}

bool AWarfareFrontlineWoundFX::HasLimbLossV8216(const AWarfareFrontlineSoldier* Unit) const
{
    for(const auto& View:CasualtyViews)if(IsValid(View)&&View->GetOwner()==Unit&&View->IsVisible())return true;
    return false;
}
void AWarfareFrontlineWoundFX::PrepareRoleV8216(USkeletalMesh* Original)
{
    if(!Original||!Original->GetName().EndsWith(TEXT("_V67"))||LimbPreloadsV8216.Contains(Original->GetFName()))return;
    FString Identity=Original->GetName();Identity.RemoveFromStart(TEXT("SK_"));Identity.RemoveFromEnd(TEXT("_V67"));
    if(!Identity.StartsWith(TEXT("British_"))&&!Identity.StartsWith(TEXT("German_")))return;
    TArray<FSoftObjectPath> Paths;
    for(const TCHAR* Label:{TEXT("CalfRight"),TEXT("ForearmLeft"),TEXT("ThighLeft")})
        for(const TCHAR* Prefix:{TEXT("SK_Casualty_"),TEXT("SM_Limb_")})
        {
            const FString N=FString::Printf(TEXT("%s%s_%s_V8216"),Prefix,*Identity,Label);
            Paths.Add(FSoftObjectPath(FString::Printf(TEXT("/Game/Warfare/Characters/V8216/Casualties/%s.%s"),*N,*N)));
        }
    // The exact-role legacy fallback is also prepared before combat. OnFatal
    // only resolves resident assets, even if a candidate pair is missing.
    const FString Legacy=FString::Printf(TEXT("SK_Casualty_%s_V67"),*Identity);
    Paths.Add(FSoftObjectPath(FString::Printf(TEXT("/Game/Warfare/Characters/V67/Casualties/%s.%s"),*Legacy,*Legacy)));
    const TCHAR* Faction=Identity.StartsWith(TEXT("German_"))?TEXT("German"):TEXT("British");
    const FString Piece=FString::Printf(TEXT("SM_CalfLeft_%s_V34"),Faction);
    Paths.Add(FSoftObjectPath(FString::Printf(TEXT("/Game/Warfare/Characters/V34/Casualties/%s.%s"),*Piece,*Piece)));
    const TWeakObjectPtr<AWarfareFrontlineWoundFX> ServiceRefV8216(this);
    auto Handle=UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths,[ServiceRefV8216,Paths]()
    {
        auto* Self=ServiceRefV8216.Get();if(!Self||Self->IsActorBeingDestroyed())return;
        for(const auto& Path:Paths)
        {
            UPrimitiveComponent* Warmup=nullptr;
            if(auto* Mesh=Cast<USkeletalMesh>(Path.ResolveObject()))
            {auto* C=NewObject<USkeletalMeshComponent>(Self);C->SetSkeletalMeshAsset(Mesh);Warmup=C;}
            else if(auto* PieceMesh=Cast<UStaticMesh>(Path.ResolveObject()))
            {auto* C=NewObject<UStaticMeshComponent>(Self);C->SetStaticMesh(PieceMesh);Warmup=C;}
            if(Warmup)
            {
                Warmup->SetVisibility(false);Warmup->SetCollisionEnabled(ECollisionEnabled::NoCollision);Warmup->SetCastShadow(true);
                // Unregistered components request the real mesh/material PSOs
                // without adding visible geometry, physics, or scene proxies.
                Warmup->PrecachePSOs();Self->LimbPSOWarmupV8216.Add(Warmup);
            }
        }
    },FStreamableManager::DefaultAsyncLoadPriority);
    if(Handle)LimbPreloadsV8216.Add(Original->GetFName(),MoveTemp(Handle));
    else UE_LOG(LogTemp,Warning,TEXT("WF_V8216_LIMB_PRELOAD_FAILED exact_role=%s"),*Original->GetName());
}
bool AWarfareFrontlineWoundFX::WaitForPreparedRoleV8216(USkeletalMesh* Original)
{
    auto* B=Cast<AWarfareFrontlineGameMode>(GetWorld()?GetWorld()->GetAuthGameMode():nullptr);
    if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!Original)return false;
    PrepareRoleV8216(Original);const auto* Handle=LimbPreloadsV8216.Find(Original->GetFName());
    if(!Handle||!Handle->IsValid())return false;
    // Only the exact disposable inspection profile may wait; production
    // recruitment requests asynchronously during the three-second approach.
    (*Handle)->WaitUntilComplete(5.f,true);return (*Handle)->HasLoadCompleted();
}
bool AWarfareFrontlineWoundFX::ResetFocusedReviewV8216()
{
    auto* B=Cast<AWarfareFrontlineGameMode>(GetWorld()?GetWorld()->GetAuthGameMode():nullptr);
    if(!FrontlineEffectsReviewSafetyV8216::Allowed(B))return false;
    ClearPresentation();return true;
}
FString AWarfareFrontlineWoundFX::LossVariantV8216(const AWarfareFrontlineSoldier* Unit) const
{
    for(const auto& View:CasualtyViews)
        if(IsValid(View)&&View->GetOwner()==Unit&&View->IsVisible()&&View->GetSkeletalMeshAsset())
            return View->GetSkeletalMeshAsset()->GetName();
    return FString();
}
int32 AWarfareFrontlineWoundFX::ActiveBodyMistV8216() const
{int32 N=0;for(const auto& B:Bursts)N+=B.Age<.20f;return N;}
bool AWarfareFrontlineWoundFX::ShowBlastLoss(AWarfareFrontlineSoldier* Unit)
{
    if (!bReady || !Unit || Unit->IsAlive() || !Unit->GetDeathState().bStarted || LossesShown>=8) return false;
    USkeletalMeshComponent* Source=Unit->Soldier.Get();auto* Original=Source?Source->GetSkeletalMeshAsset():nullptr;
    if (!Original || !Source->DoesSocketExist(TEXT("calf_l"))) return false;
    // Receiver bodies have their own skeleton and appearance. Legacy missing-
    // limb followers cannot safely replace them; retain the physical corpse.
    const FString SourcePath=Original->GetPathName();
    if(SourcePath.StartsWith(TEXT("/Game/FRRUExpansionR2/FormalV82/"))
        ||SourcePath.StartsWith(TEXT("/Game/FRRUExpansionR3/FormalV82/"))
        ||SourcePath.StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/")))return false;
    const bool German=Unit->GetFaction()==EFrontlineFaction::German;
    const bool bV68Appearance=Original->GetName().EndsWith(TEXT("_V68"));
    const bool bV67Appearance=Original->GetName().EndsWith(TEXT("_V67"));
    const bool bV62Appearance=Original->GetName().EndsWith(TEXT("_V62"));
    const bool bV34Appearance=bV68Appearance||bV67Appearance||bV62Appearance||Original->GetName().EndsWith(TEXT("_V34"));
    const bool bV29Appearance=Original->GetName().EndsWith(TEXT("_V29"));
    const bool bV24Appearance=Original->GetName().EndsWith(TEXT("_V24"));
    const bool bV23Appearance=Original->GetName().EndsWith(TEXT("_V23"));
    const bool bV16Appearance=Original->GetName().EndsWith(TEXT("_V16"));
    FString Name=Original->GetName().Replace(TEXT("SK_"),TEXT("SK_Casualty_"));
    if(!bV34Appearance&&!bV29Appearance&&!bV24Appearance&&!bV23Appearance&&!bV16Appearance)Name.ReplaceInline(TEXT("_V12"),TEXT("_V14"));
    const TCHAR* CasualtyFolder=bV68Appearance?TEXT("/Game/Warfare/Characters/V68/Casualties"):bV67Appearance?TEXT("/Game/Warfare/Characters/V67/Casualties"):bV62Appearance?TEXT("/Game/Warfare/Characters/V62/Casualties"):bV34Appearance?TEXT("/Game/Warfare/Characters/V34/Casualties"):bV29Appearance?TEXT("/Game/Warfare/Characters/V29/Casualties"):bV24Appearance?TEXT("/Game/Warfare/Characters/V24/Casualties"):
        (bV23Appearance?TEXT("/Game/Warfare/Characters/V23/Casualties"):
        (bV16Appearance?TEXT("/Game/Warfare/Characters/V16/Casualties"):TEXT("/Game/Warfare/Field/V14/Casualties")));
    USkeletalMesh* Missing=nullptr;UStaticMesh* Fragment=nullptr;FName CutBone=TEXT("calf_l");bool bAcceptedMultiLimb=false;
    FString PartName=bV34Appearance?(German?TEXT("SM_CalfLeft_German_V34"):TEXT("SM_CalfLeft_British_V34")):
        bV29Appearance?(German?TEXT("SM_CalfLeft_German_V29"):TEXT("SM_CalfLeft_British_V29")):(German?TEXT("SM_CalfLeft_German_V14"):TEXT("SM_CalfLeft_British_V14"));
    const TCHAR* PartFolder=bV34Appearance?TEXT("/Game/Warfare/Characters/V34/Casualties"):bV29Appearance?TEXT("/Game/Warfare/Characters/V29/Casualties"):TEXT("/Game/Warfare/Field/V14/Casualties");
    if(bV67Appearance)
    {
        static const TCHAR* Labels[]={TEXT("CalfRight"),TEXT("ForearmLeft"),TEXT("ThighLeft")};
        static const TCHAR* Bones[]={TEXT("calf_r"),TEXT("lowerarm_l"),TEXT("thigh_l")};
        const uint32 Seed=uint32(Unit->SquadId)*73856093u^uint32(Unit->MemberIndex)*19349663u^uint32(Unit->Team)*83492791u;
        const int32 Choice=int32(Seed%3u);
        FString Identity=Original->GetName();Identity.RemoveFromStart(TEXT("SK_"));Identity.RemoveFromEnd(TEXT("_V67"));
        const FString NewBody=FString::Printf(TEXT("SK_Casualty_%s_%s_V8216"),*Identity,Labels[Choice]);
        const FString NewPart=FString::Printf(TEXT("SM_Limb_%s_%s_V8216"),*Identity,Labels[Choice]);
        constexpr const TCHAR* Folder=TEXT("/Game/Warfare/Characters/V8216/Casualties");
        auto* Candidate=Cast<USkeletalMesh>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"),Folder,*NewBody,*NewBody)).ResolveObject());
        auto* Piece=Cast<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"),Folder,*NewPart,*NewPart)).ResolveObject());
        if(Candidate&&Piece&&Candidate->GetSkeleton()==Original->GetSkeleton()&&Source->DoesSocketExist(Bones[Choice]))
        {Missing=Candidate;Fragment=Piece;Name=NewBody;PartName=NewPart;CutBone=Bones[Choice];bAcceptedMultiLimb=true;}
        else UE_LOG(LogTemp,Warning,TEXT("WF_V8216_LIMB_FALLBACK source=%s requested=%s missing_body=%d missing_part=%d accepted_fallback=left_calf"),
            *Original->GetName(),Labels[Choice],Candidate==nullptr,Piece==nullptr);
    }
    if(!bAcceptedMultiLimb)
    {
        Missing=Cast<USkeletalMesh>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"),CasualtyFolder,*Name,*Name)).ResolveObject());
        Fragment=Cast<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"),PartFolder,*PartName,*PartName)).ResolveObject());
    }
    if (!Missing || !Fragment || Missing->GetSkeleton()!=Original->GetSkeleton())
    {
        UE_LOG(LogTemp,Warning,TEXT("WF_V14_SEVER_NOT_SHOWN: actual missing-segment mesh/limb asset unavailable for %s"),*Original->GetName());return false;
    }
    const auto& A=Original->GetRefSkeleton();const auto& B=Missing->GetRefSkeleton();
    if(A.GetNum()!=B.GetNum())return false;
    for(int32 I=0;I<A.GetNum();++I)if(A.GetBoneName(I)!=B.GetBoneName(I))return false;
    const FVector Extent=Fragment->GetBounds().BoxExtent*2.f;
    if(Extent.GetMax()>(bAcceptedMultiLimb?120.f:90.f) || Extent.GetMax()<18 || Extent.GetMin()<2)
    {
        UE_LOG(LogTemp,Error,TEXT("WF_V14_SEVER_BAD_FRAGMENT_BOUNDS %s %s"),*PartName,*Extent.ToString());return false;
    }
    // Complete an in-flight evaluation before reading the actual impact pose.
    // We do not create/delete physics bodies here, even during a late hit.
    Source->HandleExistingParallelEvaluationTask(true,true);
    const FTransform Bone=Source->GetSocketTransform(CutBone);
    auto* View=NewObject<USkeletalMeshComponent>(Unit,TEXT("Missing limb appearance"));
    View->SetupAttachment(Source);View->SetRelativeTransform(FTransform::Identity);
    View->SetSkeletalMeshAsset(Missing);View->SetCollisionEnabled(ECollisionEnabled::NoCollision);View->SetCastShadow(true);View->SetReceivesDecals(false);
    Source->SetReceivesDecals(false);
    View->SetLeaderPoseComponent(Source,true,false);View->RegisterComponent();
    InitializeFrontlineCasualtyAppearanceV29(Source,View);
    for(int32 Slot=0;Slot<Missing->GetMaterials().Num();++Slot)
    {
        const FName SlotName=Missing->GetMaterials()[Slot].MaterialSlotName;
        const int32 OldSlot=Source->GetMaterialIndex(SlotName);
        if(OldSlot!=INDEX_NONE)View->SetMaterial(Slot,Source->GetMaterial(OldSlot));
    }
    // The full physical component remains intact and continues the original
    // audit. Its follower shares every bone transform; only visible triangles
    // change at the lethal hit. No full-person mesh is ever used
    // as a fragment. The intact hidden body remains a full-body physics proxy;
    // this appearance event is never reported as physical joint severance.
    Source->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Source->SetVisibility(false,false);Source->SetCastShadow(false);View->SetVisibility(true);
    CasualtyViews.Add(View);
    const int32 I=LossesShown++;Parts[I]->SetStaticMesh(Fragment);
    for(int32 Slot=0;Slot<Fragment->GetStaticMaterials().Num();++Slot)
    {
        const int32 OldSlot=Source->GetMaterialIndex(Fragment->GetStaticMaterials()[Slot].MaterialSlotName);
        if(OldSlot!=INDEX_NONE)Parts[I]->SetMaterial(Slot,Source->GetMaterial(OldSlot));
    }
    auto& Part=PartState[I];Part.Age=0;Part.Position=Bone.GetLocation();Part.Rotation=Bone.GetRotation();
    const float Force=Unit->GetDeathState().ForceScale;
    const float Twist=FMath::Sin(float(Unit->SquadId)*.731f+Unit->MemberIndex*1.891f);
    const FVector Forward=Unit->GetDeathState().IncomingDirection;
    const FVector Side=FVector::CrossProduct(Forward,FVector::UpVector).GetSafeNormal();
    const float SegmentMass=CutBone==TEXT("thigh_l")?1.15f:CutBone==TEXT("lowerarm_l")?.74f:1.f;
    Part.Velocity=Forward*((90.f+Force*310.f)/SegmentMass)+Side*Twist*(60.f+Force*90.f)
        +FVector(0,0,(140.f+Force*170.f)*(1.f+.22f*Twist)/SegmentMass);
    Part.AngularVelocity=FVector(64.f+Twist*85.f,155.f*(I%2?1:-1),72.f-Twist*50.f);Part.bLanded=false;Part.Radius=8;
    Parts[I]->SetWorldTransform(FTransform(Part.Rotation,Part.Position));Parts[I]->SetVisibility(true);
    // The actual injury burst already owns its gravity-delayed ground deposit.
    // A visible mesh substitution must not stamp another pool through the body.
    UE_LOG(LogTemp,Display,TEXT("WF_V8216_SEVER_VISUAL_ONLY squad=%d member=%d kind=%d physics_age=%.3f published_samples=%d bone=%s replacement=%s fragment=%s extent_cm=%s matched_multi_limb=%d complete_physics_proxy_retained=1 count=%d budget=8"),
        Unit->SquadId,Unit->MemberIndex,int32(Unit->GetDeathState().Kind),Unit->GetDeathState().Age,Unit->GetDeathState().PublishedPoseSamples,*CutBone.ToString(),*Name,*PartName,*Extent.ToString(),bAcceptedMultiLimb,LossesShown);
    return true;
}

void AWarfareFrontlineWoundFX::ClearPresentation()
{
    for(auto& Burst:Bursts){Burst.Age=10;Burst.bDepositPending=false;}for(auto& Mark:MarkState)Mark=FFrontlineBloodMark();for(auto& Part:PartState)Part.Age=100;
    for(const auto& Mesh:Mists)if(Mesh)Mesh->SetVisibility(false);for(const auto& Mark:Marks)if(Mark)Mark->SetVisibility(false);
    for(auto& Flow:RunoffsV828)Flow=FFrontlineBloodRunoffV828();
    for(const auto& Mark:RunoffDecalsV828)if(Mark)Mark->SetVisibility(false);
    for(auto& Cloud:BlastCloudStateV828)Cloud.Age=10;
    for(const auto& Mesh:BlastCloudsV828)if(Mesh)Mesh->SetVisibility(false);
    RunoffCursorV828=BlastCloudCursorV828=0;
    for(const auto& Mesh:Parts)if(Mesh)Mesh->SetVisibility(false);
    for(const auto& View:CasualtyViews)if(IsValid(View))View->DestroyComponent();CasualtyViews.Reset();
    BurstCursor=MarkCursor=FatalMarkCursor=HitsShown=LossesShown=0;
    CorpseSeepsV53.Reset();RegisteredCorpseSeepsV86.Reset();SeepClockV53=0.f;
}

void AWarfareFrontlineWoundFX::TickCorpseSeepsV53(float DT)
{
    for(auto It=RegisteredCorpseSeepsV86.CreateIterator();It;++It)if(!It->IsValid())It.RemoveCurrent();
    SeepClockV53+=DT;if(SeepClockV53<.1f)return;
    const float Step=SeepClockV53;SeepClockV53=0.f;
    for(int32 I=CorpseSeepsV53.Num()-1;I>=0;--I)
    {
        auto& Source=CorpseSeepsV53[I];Source.Age+=Step;
        auto* Unit=Source.Unit.Get();
        if(!IsValid(Unit)||Unit->IsAlive()||!Unit->Soldier||Source.Age>18.f||Source.Remaining<=.001f)
        {CorpseSeepsV53.RemoveAtSwap(I);continue;}
        const auto& Death=Unit->GetDeathState();
        FVector Bone=Unit->Soldier->GetBoneLocation(TEXT("spine_03"));
        const FVector Pelvis=Unit->Soldier->GetBoneLocation(TEXT("pelvis"));
        if(Pelvis.Z<Bone.Z)Bone=Pelvis;
        const float Speed=FVector::Distance(Bone,Source.LastBone)/FMath::Max(Step,.001f);Source.LastBone=Bone;
        if(Bone.ContainsNaN()||!Death.bStarted||!Death.bPhysicsStarted||Death.PublishedPoseSamples<1||Death.bMissingPhysics||Death.bNonFinite)
        {Source.Stable=0.f;continue;}
        auto* Battle=GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>();
        float Floor=FrontlineInfantryV45::SurfaceHeight(Battle,Bone),Level=0,Depth=0;
        const bool Wet=AWarfareFrontlineGround::QueryWaterSurfaceV51(Bone.X,Bone.Y,Level,Depth);
        if(Wet)Floor=FMath::Max(Floor,Level);
        const bool AtRest=Source.Age>.65f&&Speed<55.f&&(Death.bSettled||(Death.CurrentBodySpeedCmS<45.f&&Death.CurrentAngularSpeedRadS<1.5f))
            &&Bone.Z<=Floor+45.f&&Bone.Z>=Floor-FMath::Max(Depth+25.f,35.f);
        Source.Stable=AtRest?Source.Stable+Step:0.f;
        if(Source.Stable<.4f)continue;
        Source.DepositAge+=Step;if(Source.DepositAge<.3f)continue;
        float Dose=FMath::Min(Source.Remaining,FMath::Min(Source.DepositAge,.45f)*.10f);Source.DepositAge=0.f;
        // A finite reservoir drains into either absorbent earth or an actual
        // connected pool. Moving/exploded bodies pause injection at the new pose.
        if(Wet)UFrontlineWaterResponseV51::NotifyBlood(GetWorld(),Bone,Dose,165.f,true);
        // Only actual corpse displacement creates a new receiving location.
        // A reclaimed/missing slot is not a new wound and cannot replay emission.
        if(!Wet&&Source.bGroundMarked&&FVector::DistSquared2D(Bone,Source.GroundDepositV84411)>FMath::Square(24.f))
        {Source.bGroundMarked=false;Source.bDryDrain=false;}
        if(!Wet&&!Source.bGroundMarked)
        {
            const int32 Seed=Unit->SquadId*41+Unit->MemberIndex;
            Source.GroundMarkV84411=AddMark(Bone,Unit->GetActorForwardVector(),86.f,1.f,Seed,true,true);
            if(Source.GroundMarkV84411==INDEX_NONE)continue;
            Source.GroundMarkIdentityV84411=MarkState[Source.GroundMarkV84411].IdentityV84411;Source.GroundDepositV84411=Bone;
            Source.bGroundMarked=true;
        }
        if(!Wet&&Source.bGroundMarked&&!Source.bDryDrain)
        {
            // Retry an unavailable chain without re-emitting or draining the
            // local pool. A successful reservation spends this source once.
            bool Waiting=false;
            const float Used=AddRunoffV828(Bone,Unit->SquadId*41+Unit->MemberIndex,FMath::Min(.32f,Source.Remaining*.28f),&Waiting);
            Source.Remaining-=Used;Source.bDryDrain=Used>0||!Waiting;
        }
        // The downhill reservation and the local dose share one reservoir.
        // A nearly exhausted source cannot spend its final dose twice.
        Dose=FMath::Min(Dose,Source.Remaining);
        if(!Wet&&MarkState.IsValidIndex(Source.GroundMarkV84411)
            &&MarkState[Source.GroundMarkV84411].IdentityV84411==Source.GroundMarkIdentityV84411)
            MarkState[Source.GroundMarkV84411].MassV84411+=Dose;
        Source.Remaining-=Dose;
    }
}

float AWarfareFrontlineWoundFX::AddRunoffV828(const FVector& Position,int32 Seed,float Mass,bool* bWaitingForCapacityV86)
{
    if(bWaitingForCapacityV86)*bWaitingForCapacityV86=false;
    if(!bReady||RunoffsV828.IsEmpty()||!FMath::IsFinite(Mass)||Mass<=0||Position.ContainsNaN())return 0;
    auto* B=GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>();
    const auto Height=[B](float X,float Y){return FrontlineInfantryV45::SurfaceHeight(B,FVector(X,Y,0));};
    FVector P(Position.X,Position.Y,Height(Position.X,Position.Y));
    TArray<FFrontlineBloodRunoffV828,TInlineAllocator<8>> Draft;
    float RemainingMass=Mass;
    for(int32 J=0;J<8;++J)
    {
        FVector Q;if(!BloodStepV828(P,Q,Height))break;
        bool WaterEntry=false,BlockedByWater=false;float Level=0,Depth=0;
        for(int32 K=1;K<=6;++K)
        {
            FVector Probe=FMath::Lerp(P,Q,K/6.f);
            if(AWarfareFrontlineGround::QueryWaterSurfaceV51(Probe.X,Probe.Y,Level,Depth))
            {
                if(Level>P.Z+.05f){BlockedByWater=true;break;}
                Q=Probe;Q.Z=Height(Q.X,Q.Y);WaterEntry=true;break;
            }
        }
        if(BlockedByWater)break;
        auto& F=Draft.AddDefaulted_GetRef();F.Start=P;F.End=Q;F.Age=0;F.Delay=J==0?.15f:0.f;
        F.bStartedV84411=J==0;
        const float Length=FVector::Dist2D(P,Q),Slope=(P.Z-Q.Z)/FMath::Max(Length,1.f);
        F.Duration=Length/FMath::Clamp(12.f+Slope*68.f,12.f,42.f);
        F.bWaterEntry=WaterEntry;F.Mass=RemainingMass;F.FilmMassV84411=RemainingMass*.12f;
        RemainingMass-=F.FilmMassV84411;P=Q;if(WaterEntry)break;
    }
    if(Draft.IsEmpty())return 0;
    TArray<int32,TInlineAllocator<8>> Slots;
    for(int32 J=0;J<RunoffsV828.Num()&&Slots.Num()<Draft.Num();++J)
    {
        const int32 I=(RunoffCursorV828+J)%RunoffsV828.Num();
        if(RunoffsV828[I].Age>=BloodDepositLifetimeV86)Slots.Add(I);
    }
    if(Slots.Num()!=Draft.Num())
    {
        if(bWaitingForCapacityV86)*bWaitingForCapacityV86=true;
        // Reserve the whole route before touching a slot. Overwriting its head
        // while later segments still own mass caused abrupt disappear/replay.
        // Retire only an entirely delivered old chain, all its film together.
        bool Pending=false;uint32 OldChain=0;float Oldest=-1;
        for(const auto& F:RunoffsV828)Pending|=F.RetirementAgeV86>=0&&F.Age<BloodDepositLifetimeV86;
        if(!Pending)for(const auto& Candidate:RunoffsV828)
        {
            if(Candidate.ChainV84411==0||!Candidate.bDelivered||Candidate.Age>=BloodDepositLifetimeV86)continue;
            bool Complete=true;float Youngest=MAX_flt;
            for(const auto& F:RunoffsV828)if(F.ChainV84411==Candidate.ChainV84411)
            {Complete&=F.bDelivered;Youngest=FMath::Min(Youngest,F.Age);}
            if(Complete&&Youngest>Oldest){Oldest=Youngest;OldChain=Candidate.ChainV84411;}
        }
        if(OldChain)for(auto& F:RunoffsV828)if(F.ChainV84411==OldChain&&F.Age<BloodDepositLifetimeV86)F.RetirementAgeV86=0;
        return 0; // No reservation, no source-mass debit and no partial chain.
    }
    const uint32 Chain=++BloodChainIdentityV84411;
    for(int32 J=0;J<Draft.Num();++J)
    {
        const int32 I=Slots[J];auto& F=RunoffsV828[I];F=Draft[J];F.ChainV84411=Chain;
        F.NextSlotV84411=J+1<Slots.Num()?Slots[J+1]:INDEX_NONE;F.bTerminal=J+1==Slots.Num();
        const FVector Mid=(F.Start+F.End)*.5,Along=(F.End-F.Start).GetSafeNormal();
        const FVector N=FVector(Height(Mid.X-2,Mid.Y)-Height(Mid.X+2,Mid.Y),
            Height(Mid.X,Mid.Y-2)-Height(Mid.X,Mid.Y+2),4).GetSafeNormal();
        auto* D=RunoffDecalsV828[I].Get();D->SetWorldLocation(Mid+N*.55f);
        D->SetWorldRotation(FRotationMatrix::MakeFromXZ(-N,FVector::VectorPlaneProject(Along,N).GetSafeNormal()).Rotator());
        D->DecalSize=FVector(3,8.f-J*.40f,FVector::Distance(F.Start,F.End)*.54f);D->MarkRenderStateDirty();D->SetVisibility(false);
        auto* M=RunoffMaterialsV828[I].Get();M->SetScalarParameterValue(TEXT("Seed"),float((Seed+J*7)%31));
        M->SetScalarParameterValue(TEXT("Density"),0);M->SetScalarParameterValue(TEXT("Freshness"),1);M->SetScalarParameterValue(TEXT("SpreadV65"),0);
    }
    RunoffCursorV828=(Slots.Last()+1)%RunoffsV828.Num();return Mass;
}

void AWarfareFrontlineWoundFX::TickRunoffsV828(float DT)
{
    for(int32 I=0;I<RunoffsV828.Num();++I)
    {
        auto& F=RunoffsV828[I];if(F.bStartedV84411)F.Age+=DT;const float T=F.Age-F.Delay;
        if(F.RetirementAgeV86>=0)
        {F.RetirementAgeV86+=DT;if(F.RetirementAgeV86>=BloodRetirementSecondsV86)F.Age=BloodDepositLifetimeV86;}
        const bool Visible=F.bStartedV84411&&T>=0&&F.Age<BloodDepositLifetimeV86;RunoffDecalsV828[I]->SetVisibility(Visible);if(!Visible)continue;
        const float Front=FMath::Clamp(T/F.Duration,0.f,1.f);
        auto* M=RunoffMaterialsV828[I].Get();M->SetScalarParameterValue(TEXT("SpreadV65"),Front);
        M->SetScalarParameterValue(TEXT("Freshness"),FMath::Exp(-T/33.f));
        const float Retire=F.RetirementAgeV86<0?1.f:FMath::Clamp(1-F.RetirementAgeV86/BloodRetirementSecondsV86,0.f,1.f);
        M->SetScalarParameterValue(TEXT("Density"),.94f*FMath::Clamp((BloodDepositLifetimeV86-F.Age)/18.f,0.f,1.f)*Retire);
        if(Front>=1&&!F.bDelivered)
        {
            const float Delivered=FMath::Max(0.f,F.Mass-F.FilmMassV84411);
            if(F.bWaterEntry)UFrontlineWaterResponseV51::NotifyBlood(GetWorld(),F.End,Delivered,90.f,true);
            else if(F.bTerminal)
            {
                if(AddMark(F.End,F.End-F.Start,22.f+20.f*FMath::Sqrt(Delivered),1.f,I*31,true,true,Delivered)==INDEX_NONE)continue;
            }
            else if(RunoffsV828.IsValidIndex(F.NextSlotV84411))
            {
                auto& Next=RunoffsV828[F.NextSlotV84411];
                // A reused pool slot cannot wake a different casualty's flow.
                if(Next.ChainV84411==F.ChainV84411&&Next.Start.Equals(F.End,.01f))
                {Next.bStartedV84411=true;Next.Age=-DT;}
            }
            F.bDelivered=true;
        }
    }
}

void AWarfareFrontlineWoundFX::RunChecksV828(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Allowed=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV828Review"));
    Check(TEXT("blood_review_requires_disposable_profile"),Allowed);if(!Allowed)return;
    FVector Q;
    Check(TEXT("blood_on_level_ground_does_not_invent_a_flow"),!BloodStepV828(FVector::ZeroVector,Q,[](float,float){return 0.f;}));
    const bool Sloped=BloodStepV828(FVector::ZeroVector,Q,[](float X,float){return -X*.15f;});
    Check(TEXT("blood_follows_downhill_slope"),Sloped&&Q.X>10&&Q.Z<0);
    Check(TEXT("blood_does_not_cross_vertical_trench_drop"),!BloodStepV828(FVector::ZeroVector,Q,[](float X,float Y){return FMath::Abs(X)+FMath::Abs(Y)>10?-100.f:0.f;}));
    Check(TEXT("blood_rejects_rising_bank"),!BloodStepV828(FVector::ZeroVector,Q,[](float X,float Y){return FMath::Sqrt(X*X+Y*Y)*.1f;}));
    Check(TEXT("blood_stops_at_local_uphill_lip"),!BloodStepV828(FVector::ZeroVector,Q,[](float X,float Y)
    {const float R=FMath::Sqrt(X*X+Y*Y);return R<1?0.f:R<14?-2.f:R<24?-1.f:R<33?-4.f:-6.f;}));
    auto* FX=Get(B->GetWorld());Check(TEXT("blood_pools_and_volumes_have_fixed_budgets"),FX&&FX->Marks.Num()==BloodMarkCountV86&&FX->RunoffsV828.Num()==48&&FX->BlastCloudStateV828.Num()==8);
    auto* Material=FX&&FX->Marks.IsValidIndex(0)?FX->Marks[0]->GetDecalMaterial():nullptr;
    Check(TEXT("blood_deposit_uses_cohesive_liquid_material"),Material&&Material->GetBaseMaterial()&&Material->GetBaseMaterial()->GetName()==TEXT("M_BloodDeposit_V84411"));
}

bool AWarfareFrontlineWoundFX::ReviewDepositsV828(FVector& Ground,const FVector& Burst)
{
    if(!bReady||!FParse::Param(FCommandLine::Get(),TEXT("WFV828Review")))return false;
    FVector Chosen=Ground;float Best=-MAX_flt;
    for(int32 X=-5;X<=5;++X)for(int32 Y=-5;Y<=5;++Y)
    {
        FVector P=Ground+FVector(X*40,Y*40,0),Q;P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y);
        float Level=0,Depth=0;
        if(AWarfareFrontlineGround::QueryWaterSurfaceV51(P.X,P.Y,Level,Depth))continue;
        if(!BloodStepV828(P,Q,[](float A,float B){return AWarfareFrontlineGround::HeightAt(A,B);}))continue;
        const float Score=P.Z-Q.Z-.008f*FVector::Dist2D(P,Ground);
        if(Score>Best){Best=Score;Chosen=P;}
    }
    Ground=Chosen;AddMark(Ground,FVector::ForwardVector,78,1,17,true,true,.40f);
    const float Used=AddRunoffV828(Ground,17,.32f);
    UE_LOG(LogTemp,Display,TEXT("WF_V828_BLOOD_FLOW origin=%s mass=%.3f segments=%d"),*Ground.ToString(),Used,RunoffCursorV828);
    return Used>0;
}

namespace
{
bool BloodReviewAllowedV84411(AWarfareFrontlineGameMode* B)
{
    FString User;return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(User);
}
// Historical selector retained only for paired diagnostic counts. It never
// controls production flow, delivery, opacity or ground-contact placement.
bool LegacyBloodStepV84411(const FVector& P,FVector& Q,TFunctionRef<float(float,float)> Height)
{
    const float Z=Height(P.X,P.Y);float Drop=.16f;bool Found=false;
    for(int32 I=0;I<12;++I)
    {
        const float Angle=2*PI*I/12.f;FVector Candidate=P+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0)*38.f;
        Candidate.Z=Height(Candidate.X,Candidate.Y);const float D=Z-Candidate.Z;if(!FMath::IsFinite(Candidate.Z)||D<=Drop||D>24.f)continue;
        bool Continuous=true;float Previous=Z;
        for(int32 J=1;J<=4;++J){const FVector At=FMath::Lerp(P,Candidate,J/4.f);const float H=Height(At.X,At.Y);Continuous&=FMath::IsFinite(H)&&H<=Previous+.04f;Previous=H;}
        if(Continuous){Drop=D;Q=Candidate;Found=true;}
    }
    return Found;
}
}

TSharedRef<FJsonObject> AWarfareFrontlineWoundFX::DescribeBloodV84411() const
{
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Flows,Pools;float Film=0,Terminal=0;int32 Active=0;
    for(int32 I=0;I<RunoffsV828.Num();++I)
    {
        const auto& F=RunoffsV828[I];if(F.Age>=110.f||F.ChainV84411==0)continue;
        auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("slot"),I);R->SetNumberField(TEXT("chain"),F.ChainV84411);
        R->SetStringField(TEXT("start"),F.Start.ToString());R->SetStringField(TEXT("end"),F.End.ToString());
        R->SetNumberField(TEXT("age"),F.Age);R->SetNumberField(TEXT("duration"),F.Duration);R->SetNumberField(TEXT("input_mass"),F.Mass);
        R->SetNumberField(TEXT("film_mass"),F.FilmMassV84411);R->SetNumberField(TEXT("next_slot"),F.NextSlotV84411);
        R->SetBoolField(TEXT("started"),F.bStartedV84411);R->SetBoolField(TEXT("delivered"),F.bDelivered);R->SetBoolField(TEXT("water_entry"),F.bWaterEntry);
        R->SetBoolField(TEXT("terminal"),F.bTerminal);R->SetBoolField(TEXT("visible"),RunoffDecalsV828[I]&&RunoffDecalsV828[I]->IsVisible());
        Film+=F.FilmMassV84411;if(F.bTerminal)Terminal+=F.Mass-F.FilmMassV84411;Active+=F.bStartedV84411?1:0;
        Flows.Add(MakeShared<FJsonValueObject>(R));
    }
    for(int32 I=0;I<MarkState.Num();++I)if(MarkState[I].Age<110.f)
    {
        const auto& P=MarkState[I];auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("slot"),I);R->SetNumberField(TEXT("mass"),P.MassV84411);
        R->SetNumberField(TEXT("spread"),P.SpreadV84411);R->SetBoolField(TEXT("finite_source"),P.bMassDrivenV84411);
        R->SetStringField(TEXT("position"),Marks[I]->GetComponentLocation().ToString());Pools.Add(MakeShared<FJsonValueObject>(R));
    }
    J->SetArrayField(TEXT("flows"),Flows);J->SetArrayField(TEXT("pools"),Pools);J->SetNumberField(TEXT("started_segments"),Active);
    J->SetNumberField(TEXT("film_mass"),Film);J->SetNumberField(TEXT("terminal_mass"),Terminal);
    J->SetNumberField(TEXT("flow_budget"),RunoffsV828.Num());J->SetNumberField(TEXT("pool_budget"),Marks.Num());
    J->SetStringField(TEXT("material"),Marks.Num()&&Marks[0]->GetDecalMaterial()?Marks[0]->GetDecalMaterial()->GetPathName():FString());return J;
}

bool AWarfareFrontlineWoundFX::ReviewBloodDepositsV84411(FVector& Ground)
{
    auto* B=GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>();if(!BloodReviewAllowedV84411(B)||!bReady)return false;
    const auto Height=[B](float X,float Y){return FrontlineInfantryV45::SurfaceHeight(B,FVector(X,Y,0));};
    FVector Best=Ground;float Score=-MAX_flt;
    for(int32 X=-6;X<=6;++X)for(int32 Y=-6;Y<=6;++Y)
    {
        FVector P=Ground+FVector(X*28.f,Y*28.f,0),Q;P.Z=Height(P.X,P.Y);float Level=0,Depth=0;
        if(AWarfareFrontlineGround::QueryWaterSurfaceV51(P.X,P.Y,Level,Depth)||!BloodStepV828(P,Q,Height))continue;
        const float Value=P.Z-Q.Z-.004f*FVector::Dist2D(P,Ground);if(Value>Score){Score=Value;Best=P;}
    }
    if(Score==-MAX_flt)return false;
    Ground=Best;AddMark(Best,FVector::ForwardVector,76,1,17,true,true,.40f);
    return AddRunoffV828(Best,17,.32f)>.31f;
}

void AWarfareFrontlineWoundFX::RunBloodChecksV84411(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    Check(TEXT("v84411_blood_disposable_profile"),BloodReviewAllowedV84411(B));if(!BloodReviewAllowedV84411(B))return;
    FVector Q;
    Check(TEXT("v84411_blood_level_floor_forms_pool_not_fake_flow"),!BloodStepV828(FVector::ZeroVector,Q,[](float,float){return 0.f;}));
    Check(TEXT("v84411_blood_small_real_grade_still_flows"),BloodStepV828(FVector::ZeroVector,Q,[](float X,float){return -X*.005f;})&&Q.X>10.f&&Q.Z<-.035f);
    Check(TEXT("v84411_blood_no_uphill_or_nonfinite_transport"),!BloodStepV828(FVector::ZeroVector,Q,[](float X,float Y){return FMath::Sqrt(X*X+Y*Y)*.1f;})
        &&!BloodStepV828(FVector::ZeroVector,Q,[](float,float){return std::numeric_limits<float>::quiet_NaN();}));
    Check(TEXT("v84411_blood_cannot_jump_vertical_trench_face"),!BloodStepV828(FVector::ZeroVector,Q,[](float X,float Y){return FMath::Abs(X)+FMath::Abs(Y)>3?-90.f:0.f;}));
    // The old 38 cm chord crosses another rut's rising side. The real narrow
    // near-side descent remains traversable by the short checked contact.
    const auto Rutted=[](float X,float Y){return FMath::Abs(Y)*.03f+(X<=0?-X*.02f:X<15?-X*.04f:X<26?(X-15)*.12f-.60f:-.12f);};
    FVector Old;const bool Narrow=BloodStepV828(FVector::ZeroVector,Q,Rutted),Legacy=LegacyBloodStepV84411(FVector::ZeroVector,Old,Rutted);
    Check(TEXT("v84411_blood_resolves_narrow_downhill_rut_without_crossing_its_rise"),Narrow&&Q.X>0&&Q.Z<0&&!Legacy);
    auto* FX=Get(B->GetWorld());Check(TEXT("v84411_blood_fixed_pool_and_flow_budgets"),FX&&FX->bReady&&FX->Marks.Num()==BloodMarkCountV86&&FX->RunoffsV828.Num()==48);if(!FX||!FX->bReady)return;
    auto* Mat=FX->Marks[0]->GetDecalMaterial();Check(TEXT("v84411_blood_new_paired_wet_liquid_material_is_loaded"),Mat&&Mat->GetBaseMaterial()&&Mat->GetBaseMaterial()->GetName()==TEXT("M_BloodDeposit_V84411"));
    const auto Height=[B](float X,float Y){return FrontlineInfantryV45::SurfaceHeight(B,FVector(X,Y,0));};
    FVector Actual=FVector::ZeroVector;bool Found=false;int32 Samples=0,NewFlows=0,OldFlows=0;
    const int32 Home=B->GetHomeTrenchIndex(0),Next=FMath::Clamp(Home+B->GetTeamDirection(0),0,B->Trenches.Num()-1);
    const float Mid=B->Trenches.IsValidIndex(Home)&&B->Trenches.IsValidIndex(Next)?(B->Trenches[Home].X+B->Trenches[Next].X)*.5f:0.f;
    for(int32 X=-6;X<=6;++X)for(int32 Y=-6;Y<=6;++Y)
    {
        FVector P(Mid+X*55.f,Y*75.f,0);P.Z=Height(P.X,P.Y);float Level=0,Depth=0;
        if(AWarfareFrontlineGround::QueryWaterSurfaceV51(P.X,P.Y,Level,Depth))continue;++Samples;
        FVector N;const bool HasNew=BloodStepV828(P,N,Height);NewFlows+=HasNew?1:0;OldFlows+=LegacyBloodStepV84411(P,N,Height)?1:0;
        if(!HasNew||Found)continue;int32 Count=0;bool Dry=true;FVector Step=P;
        for(int32 I=0;I<8;++I){if(!BloodStepV828(Step,N,Height))break;Step=N;++Count;Dry&=!AWarfareFrontlineGround::QueryWaterSurfaceV51(Step.X,Step.Y,Level,Depth);}
        if(Count>=3&&Dry){Actual=P;Found=true;}
    }
    Check(TEXT("v84411_blood_actual_scene_has_contact_checked_flow_path"),Samples>0&&NewFlows>0&&Found);if(!Found)return;
    FX->ClearPresentation();const float Input=.32f;const bool Reserved=FX->AddRunoffV828(Actual,17,Input)>.31f;
    int32 Count=0,Started=0;float Budget=0;uint32 Chain=0;
    for(const auto& F:FX->RunoffsV828)if(F.ChainV84411!=0&&F.Age<110.f){++Count;Started+=F.bStartedV84411?1:0;Budget+=F.FilmMassV84411;if(F.bTerminal)Budget+=F.Mass-F.FilmMassV84411;Chain=F.ChainV84411;}
    Check(TEXT("v84411_blood_reserved_mass_equals_film_plus_terminal_reservoir"),Reserved&&Count>=3&&Started==1&&FMath::IsNearlyEqual(Budget,Input,.00001f));
    FX->TickRunoffsV828(.10f);Started=0;for(const auto& F:FX->RunoffsV828)if(F.ChainV84411==Chain&&F.bStartedV84411)++Started;
    Check(TEXT("v84411_blood_downstream_never_starts_on_fixed_delay"),Started==1);
    bool Causal=true;for(int32 I=0;I<240;++I)
    {
        FX->TickRunoffsV828(.05f);
        for(const auto& F:FX->RunoffsV828)if(F.ChainV84411==Chain&&F.NextSlotV84411!=INDEX_NONE)
            if(FX->RunoffsV828[F.NextSlotV84411].bStartedV84411&&!F.bDelivered)Causal=false;
    }
    bool Delivered=true;float Deposited=0;for(const auto& F:FX->RunoffsV828)if(F.ChainV84411==Chain){Delivered&=F.bDelivered;Deposited+=F.FilmMassV84411;}
    for(const auto& M:FX->MarkState)if(M.Age<110.f)Deposited+=M.MassV84411;
    Check(TEXT("v84411_blood_actual_chain_advances_causally_and_conserves_mass"),Causal&&Delivered&&FMath::IsNearlyEqual(Deposited,Input,.00001f));
    Check(TEXT("v84411_blood_route_projection_stays_shallow"),FX->RunoffDecalsV828[0]->DecalSize.X<=3.01f&&FX->Marks[20]->DecalSize.X<=4.01f);
    UE_LOG(LogTemp,Display,TEXT("WF_V84411_BLOOD_SAMPLE dry_samples=%d old_flows=%d new_flows=%d chained_segments=%d film_and_pool_mass=%.6f input=%.6f"),Samples,OldFlows,NewFlows,Count,Deposited,Input);
    FX->ClearPresentation();
}

void AWarfareFrontlineWoundFX::Tick(float DT)
{
    Super::Tick(DT);if(!bReady)return;
    if(const auto* Battle=GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>())
    {
        if(Battle->Elapsed+.01f<LastBattleTime||SeepEncounterV53!=Battle->GetEncounterId())ClearPresentation();
        LastBattleTime=Battle->Elapsed;SeepEncounterV53=Battle->GetEncounterId();
    }
    TickCorpseSeepsV53(DT);
    TickRunoffsV828(DT);
    const auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
    const FRotationMatrix Axes(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0));
    const FRotator Facing=FRotationMatrix::MakeFromXY(Axes.GetUnitAxis(EAxis::Y),Axes.GetUnitAxis(EAxis::Z)).Rotator();
    for(int32 I=0;I<Bursts.Num();++I)
    {
        auto& Burst=Bursts[I];Burst.Age+=DT;
        if(Burst.bDepositPending && Burst.Age>=Burst.LandingAge)
        {
            float WaterLevel=0,Depth=0;
            const bool Wet=AWarfareFrontlineGround::QueryWaterSurfaceV51(Burst.LandingPosition.X,Burst.LandingPosition.Y,WaterLevel,Depth);
            UFrontlineWaterResponseV51::NotifyBlood(GetWorld(),Burst.LandingPosition,.12f,165.f);
            if(!Wet)AddMark(Burst.LandingPosition,Burst.Velocity,Burst.MarkSize,Burst.MarkOpacity,int32(Burst.Seed)*37,Burst.bFatalDeposit);
            Burst.bDepositPending=false;
        }
        for(int32 J=0;J<3;++J)
        {
            const int32 K=I*3+J;const float T=Burst.Age-J*.018f,Life=(Burst.bBlast?.38f:.20f)+J*.035f;
            const bool Visible=T>=0 && T<Life;Mists[K]->SetVisibility(Visible);if(!Visible)continue;
            const float Phase=T/Life,Width=(23+T*(J?126.f:180.f))*Burst.Size;
            FVector Position=Burst.Position+Burst.Velocity*T+FVector(T*(J-1)*13,0,-490*T*T);
            Mists[K]->SetWorldLocation(Position);Mists[K]->SetWorldRotation(Facing);
            Mists[K]->SetWorldScale3D(FVector(Width/100,Width*(.70f+J*.1f)/100,1));
            MistMaterials[K]->SetScalarParameterValue(TEXT("Density"),((Burst.bBlast?.68f:.42f)-J*.08f)*FMath::Square(1-Phase));
        }
    }
    for(int32 I=0;I<BlastCloudStateV828.Num();++I)
    {
        auto& C=BlastCloudStateV828[I];C.Age+=DT;const float T=C.Age;
        if(T>=.85f){BlastCloudsV828[I]->SetVisibility(false);continue;}
        const float Expansion=1-FMath::Exp(-T*8),Width=(46+155*Expansion)*FMath::Clamp(C.Size,.3f,1.f);
        const float Density=FMath::Clamp(T/.045f,0.f,1.f)*FMath::Pow(FMath::Max(0.f,1-T/.85f),1.6f)*1.38f;
        const FVector At=C.Position+C.Velocity*(T*.36f)+FVector(0,0,-90*T*T);
        BlastCloudMaterialsV828[I]->SetScalarParameterValue(TEXT("Seed"),C.Seed);
        FrontlineSmokeVolumeV35::Update(BlastCloudsV828[I],BlastCloudMaterialsV828[I],At,FVector(Width,Width*.79f,Width*.86f),Axes.GetUnitAxis(EAxis::X),Density,T*3.2f);
    }
    TickMarksV86(DT);
    for(int32 I=0;I<PartState.Num();++I)
    {
        auto& Part=PartState[I];Part.Age+=DT;if(Part.Age>=35){Parts[I]->SetVisibility(false);continue;}
        if(!Part.bLanded)
        {
            const float Step=FMath::Min(DT,1.f/30.f);
            Part.Position+=Part.Velocity*Step+FVector(0,0,-490)*Step*Step;Part.Velocity.Z-=980*Step;
            Part.Rotation=(FRotator(Part.AngularVelocity.Y*Step,Part.AngularVelocity.Z*Step,Part.AngularVelocity.X*Step).Quaternion()*Part.Rotation).GetNormalized();
            const FTransform Transform(Part.Rotation,Part.Position);
            FBox Bounds=Parts[I]->GetStaticMesh()->GetBoundingBox().TransformBy(Transform);
            const float Ground=AWarfareFrontlineGround::HeightAt(Bounds.GetCenter().X,Bounds.GetCenter().Y)+1;
            if(Bounds.Min.Z<Ground)
            {
                Part.Position.Z+=Ground-Bounds.Min.Z;
                if(Part.Velocity.Z<0)
                {
                    const FVector Along=Part.Velocity.GetSafeNormal2D();
                    Part.Rotation=FRotationMatrix::MakeFromXY(FVector::UpVector,Along.IsNearlyZero()?FVector::ForwardVector:Along).ToQuat();
                    Bounds=Parts[I]->GetStaticMesh()->GetBoundingBox().TransformBy(FTransform(Part.Rotation,Part.Position));
                    Part.Position.Z+=Ground-Bounds.Min.Z;Part.bLanded=true;Part.Velocity=FVector::ZeroVector;
                }
            }
            Parts[I]->SetWorldLocationAndRotation(Part.Position,Part.Rotation);
        }
    }
}

void AWarfareFrontlineWoundFX::TickMarksV86(float DT)
{
    for(int32 I=0;I<MarkState.Num();++I)
    {
        auto& Mark=MarkState[I];Mark.Age+=DT;
        if(Mark.RetirementAgeV86>=0)
        {Mark.RetirementAgeV86+=DT;if(Mark.RetirementAgeV86>=BloodRetirementSecondsV86)Mark.Age=BloodDepositLifetimeV86;}
        const bool Visible=Mark.Age<BloodDepositLifetimeV86;Marks[I]->SetVisibility(Visible);
        if(Visible)
        {
            MarkMaterials[I]->SetScalarParameterValue(TEXT("Density"),Mark.Opacity*FMath::Clamp((BloodDepositLifetimeV86-Mark.Age)/18,0.f,1.f)
                *(Mark.RetirementAgeV86<0?1.f:FMath::Clamp(1-Mark.RetirementAgeV86/BloodRetirementSecondsV86,0.f,1.f)));
            MarkMaterials[I]->SetScalarParameterValue(TEXT("Freshness"),FMath::Exp(-Mark.Age/26.f));
            if(Mark.bMassDrivenV84411)
                Mark.SpreadV84411=FMath::FInterpConstantTo(Mark.SpreadV84411,FMath::Clamp(Mark.MassV84411/.60f,0.f,1.f),DT,.40f);
            else Mark.SpreadV84411=FMath::Clamp(Mark.Age/FMath::Max(.01f,Mark.SpreadSeconds),0.f,1.f);
            MarkMaterials[I]->SetScalarParameterValue(TEXT("SpreadV65"),Mark.SpreadV84411);
        }
    }
}

#include "WarfareFrontlineBloodReviewV86.inl"

void AWarfareFrontlineWoundFX::EndPlay(const EEndPlayReason::Type Reason)
{
    LimbPreloadsV8216.Reset();LimbPSOWarmupV8216.Reset();
    UE_LOG(LogTemp,Display,TEXT("WF_V14_WOUND_BUDGET_END hits=%d loss_appearances=%d burst_slots=24 decals=64 piece_slots=8"),HitsShown,LossesShown);
    Super::EndPlay(Reason);
}
