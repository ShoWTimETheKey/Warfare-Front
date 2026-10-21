#include "WarfareFrontlineMaterialV27.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlineEmbersV27.h"
#include "WarfareFrontlineFieldKitV31.h"
#include "WarfareFrontlineSoldierSurfaceV62.h"
#include "WarfareFrontlineSoldierSurfaceV67.h"
#include "WarfareFrontlineArtV68.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
TAutoConsoleVariable<int32> GMaterialV27(TEXT("wf.Materials.V27"),1,TEXT("New deployments use V27 field PBR. ApplyExisting command updates a frozen review."));
bool HasResponse(const FString& Name)
{
    return Name.Contains(TEXT("Cloth")) || Name.Contains(TEXT("Lining")) || Name.Contains(TEXT("Canvas")) || Name.Contains(TEXT("Gear")) || Name.Contains(TEXT("GermanFieldPaint")) || Name.Contains(TEXT("BluedSteel")) || Name.Contains(TEXT("WornSteel")) || Name.Contains(TEXT("DullBrass"));
}
UMaterialInterface* BurnReceiverV84411(UMaterialInterface* Source)
{
    if(!Source)return Source;
    const FString P=Source->GetPathName(),N=Source->GetName();
    const bool Supported=P.StartsWith(TEXT("/Game/Warfare/Characters/V84/Materials/")) ||
        (P.StartsWith(TEXT("/Game/Warfare/Characters/V34/Materials/"))&&N.Contains(TEXT("Face_"))) ||
        (P.StartsWith(TEXT("/Game/Warfare/Characters/V81/Materials/"))&&N==TEXT("M_V34_HandSkin"));
    if(!Supported)return Source;
    static TMap<FString,TStrongObjectPtr<UMaterialInterface>> Receivers;
    static TSet<FString> Missing;
    if(auto* Found=Receivers.Find(N))return Found->Get();
    if(Missing.Contains(N))return Source;
    const FString Path=TEXT("/Game/Warfare/Characters/V84411/Materials/")+N+TEXT(".")+N;
    if(auto* Receiver=LoadObject<UMaterialInterface>(nullptr,*Path))
    {Receivers.Add(N,TStrongObjectPtr<UMaterialInterface>(Receiver));return Receiver;}
    Missing.Add(N);UE_LOG(LogTemp,Warning,TEXT("WF_V84411_BURN_RECEIVER_MISSING %s"),*Path);return Source;
}
float BurnProgressV84411(float Age,float Seconds)
{
    if(!FMath::IsFinite(Age)||!FMath::IsFinite(Seconds)||Seconds<=0)return 0;
    const float T=FMath::Clamp((Age/Seconds-.48f)/.52f,0.f,1.f);
    return T*T*(3.f-2.f*T);
}
}
void FrontlineMaterialV27::Apply(AWarfareFrontlineSoldier* Unit,bool bEnabled)
{
    if(!Unit || !Unit->Soldier || !Unit->Soldier->GetSkeletalMeshAsset())return;
    const auto& Slots=Unit->Soldier->GetSkeletalMeshAsset()->GetMaterials();
    const float Seed=float((Unit->SquadId*17+Unit->MemberIndex*11)%97);
    for(int32 I=0;I<Slots.Num();++I)
    {
        UMaterialInterface* Source=Slots[I].MaterialInterface.Get();
        const bool NativeV34=Source && Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V34/Materials/"));
        const bool NativeV29=Source && Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V29/Materials/"));
        if(!Source || (!HasResponse(Source->GetName()) && !NativeV29 && !NativeV34))continue;
        UMaterialInterface* Chosen=Source;
        if(bEnabled && GMaterialV27.GetValueOnGameThread()!=0 && !NativeV29 && !NativeV34)
        {
            const FString Name=TEXT("V27_")+Source->GetName();
            const FString Path=TEXT("/Game/Warfare/Characters/V27/Materials/")+Name+TEXT(".")+Name;
            if(auto* New=LoadObject<UMaterialInterface>(nullptr,*Path))Chosen=New;
            else UE_LOG(LogTemp,Warning,TEXT("WF_V27_MATERIAL_MISSING %s"),*Path);
        }
        // V30 changes only absorbed blood colors in copied material graphs.
        if(bEnabled && Chosen && !NativeV34 && !Chosen->GetName().Contains(TEXT("Face")))
        {
            const FString Name=Chosen->GetName();
            const FString Path=TEXT("/Game/Warfare/Characters/V30/Materials/")+Name+TEXT(".")+Name;
            if(auto* Darker=LoadObject<UMaterialInterface>(nullptr,*Path))Chosen=Darker;
        }
        if(bEnabled && !NativeV34)Chosen=FrontlineFieldKitV31::RefinedMaterial(Chosen);
        if(bEnabled)Chosen=FrontlineSoldierSurfaceV62::Resolve(Chosen);
        if(bEnabled)Chosen=FrontlineSoldierSurfaceV67::ResolveMaterial(Chosen);
        if(bEnabled)Chosen=FrontlineArtV68::ResolveMaterial(Chosen);
        if(bEnabled)Chosen=BurnReceiverV84411(Chosen);
        auto* MID=Unit->Soldier->CreateDynamicMaterialInstance(I,Chosen);
        if(!MID)continue;
        MID->SetScalarParameterValue(TEXT("DeploymentSeed"),Seed);
        MID->SetScalarParameterValue(TEXT("MudAmount"),.48f+.24f*Unit->GetMotionPhase());
        MID->SetScalarParameterValue(TEXT("FatalBlood"),Unit->IsAlive()?0.f:1.f);
        MID->SetVectorParameterValue(TEXT("BloodSide"),FLinearColor(1,0,0,0));
        const float Dye=.955f+Unit->GetMotionPhase()*.085f;
        MID->SetVectorParameterValue(TEXT("PersonalWeatheringTint"),FLinearColor(Dye,Dye*(.992f+Unit->GetMotionPhase()*.016f),Dye,1));
    }
    Unit->Soldier->SetReceivesDecals(false);
    if(bEnabled)FrontlineFieldKitV31::Apply(Unit);
    FrontlineSoldierSurfaceV62::ScheduleAudit(Unit);
}
void FrontlineMaterialV27::BurnV84411(AWarfareFrontlineSoldier* Unit,float Age,float Seconds)
{
    // Burn state comes from actual fire/fuel ignition, never from a death-kind
    // guess. Keep the final dry surface after the flame particles have ended.
    if(!Unit||!Unit->Soldier||Unit->IsAlive()||!FMath::IsFinite(Seconds)||Seconds<=0)return;
    const float Amount=BurnProgressV84411(Age,Seconds);
    for(int32 I=0;I<Unit->Soldier->GetNumMaterials();++I)
        if(auto* MID=Cast<UMaterialInstanceDynamic>(Unit->Soldier->GetMaterial(I)))
        {
            float Previous=0;
            if(MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BurnAmountV84411")),Previous)&&(Amount>Previous+.0001f||(Amount==1.f&&Previous<1.f)))
                MID->SetScalarParameterValue(TEXT("BurnAmountV84411"),Amount);
        }
}
void FrontlineMaterialV27::RunBurnChecksV84411(AWarfareFrontlineSoldier* Fixture,TFunctionRef<void(const TCHAR*,bool)> Check)
{
    Check(TEXT("v84411_char_zero_until_burn_second_half"),BurnProgressV84411(0,10)==0&&FMath::IsNearlyZero(BurnProgressV84411(4.8f,10)));
    Check(TEXT("v84411_char_continuous_mid_to_terminal"),BurnProgressV84411(7.4f,10)>.49f&&BurnProgressV84411(7.4f,10)<.51f&&BurnProgressV84411(10,10)==1);
    Check(TEXT("v84411_char_no_burn_duration_is_zero"),BurnProgressV84411(10,0)==0);
    Check(TEXT("v84411_char_fixture_is_disposable_dead"),Fixture&&Fixture->Soldier&&!Fixture->IsAlive());
    if(!Fixture||!Fixture->Soldier||Fixture->IsAlive())return;
    int32 Bound=0;bool Defaults=true;
    for(int32 I=0;I<Fixture->Soldier->GetNumMaterials();++I)
    {float V=0;if(auto* M=Fixture->Soldier->GetMaterial(I))if(M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BurnAmountV84411")),V)){++Bound;Defaults&=FMath::IsNearlyZero(V);}}
    Check(TEXT("v84411_char_actual_receiver_bound_unchanged_initial_surface"),Bound>=3&&Defaults);
    BurnV84411(Fixture,3,10);bool Early=true;
    for(int32 I=0;I<Fixture->Soldier->GetNumMaterials();++I)
    {float V=0;if(auto* M=Fixture->Soldier->GetMaterial(I))if(M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BurnAmountV84411")),V))Early&=FMath::IsNearlyZero(V);}
    Check(TEXT("v84411_char_early_flames_do_not_instantly_blacken"),Early);
    BurnV84411(Fixture,10,10);BurnV84411(Fixture,20,10);BurnV84411(Fixture,0,0);
    bool Terminal=true;
    for(int32 I=0;I<Fixture->Soldier->GetNumMaterials();++I)
    {float V=0;if(auto* M=Fixture->Soldier->GetMaterial(I))if(M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BurnAmountV84411")),V))Terminal&=FMath::IsNearlyEqual(V,1.f);}
    Check(TEXT("v84411_char_persists_after_fire_and_invalid_reset"),Terminal);
}
void FrontlineMaterialV27::Fatal(AWarfareFrontlineSoldier* Unit,const FVector& IncomingDirection)
{
    if(!Unit || !Unit->Soldier || Unit->IsAlive())return;
    FVector Side=Unit->Soldier->GetComponentTransform().InverseTransformVectorNoScale(-IncomingDirection).GetSafeNormal2D();
    if(Side.IsNearlyZero())Side=FVector::ForwardVector;
    for(int32 I=0;I<Unit->Soldier->GetNumMaterials();++I)
        if(auto* MID=Cast<UMaterialInstanceDynamic>(Unit->Soldier->GetMaterial(I)))
        {
            MID->SetScalarParameterValue(TEXT("FatalBlood"),1.f);
            MID->SetVectorParameterValue(TEXT("BloodSide"),FLinearColor(Side.X,Side.Y,0,0));
        }
}
void FrontlineMaterialV27::ApplyWorld(UWorld* World,bool bEnabled)
{
    if(!World)return;
    for(TActorIterator<AWarfareFrontlineSoldier> It(World);It;++It)Apply(*It,bEnabled);
    // The real level's ground and metal props: no mesh/placement/collision edits.
    for(TActorIterator<AActor> It(World);It;++It)
    {
        TInlineComponentArray<UStaticMeshComponent*> Meshes;It->GetComponents(Meshes);
        for(auto* Mesh:Meshes)for(int32 I=0;I<Mesh->GetNumMaterials();++I)
        {
            auto* Current=Mesh->GetMaterial(I);if(!Current)continue;
            const FString N=Current->GetBaseMaterial()->GetName();
            FString Path;
            if(N.Contains(TEXT("GroundMud_V23")) || N.Contains(TEXT("GroundMud_V27")) || N.Contains(TEXT("GroundMud_V30")))
                Path=bEnabled?TEXT("/Game/Warfare/Field/V30/Materials/MI_GroundMud_V30.MI_GroundMud_V30"):TEXT("/Game/Warfare/Field/V23/Materials/MI_GroundMud_V23.MI_GroundMud_V23");
            else if(N.Contains(TEXT("FieldIron")))
                Path=bEnabled?TEXT("/Game/Warfare/Field/V27/Materials/M_FieldIron_V27.M_FieldIron_V27"):TEXT("/Game/Warfare/Field/V15/Materials/M_1917_FieldIron_V15.M_1917_FieldIron_V15");
            if(Path.IsEmpty())continue;
            if(auto* Material=LoadObject<UMaterialInterface>(nullptr,*Path))
            {
                float Wet=.58f;Current->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),Wet);
                if(auto* MID=Mesh->CreateDynamicMaterialInstance(I,Material))MID->SetScalarParameterValue(TEXT("WeatherWetness"),Wet);
            }
        }
    }
    for(TActorIterator<AWarfareFrontlineWeather> It(World);It;++It)It->RebindGroundMaterial();
    FrontlineEmbersV27::Ensure(World,bEnabled);
}
static FAutoConsoleCommandWithWorldAndArgs GApplyMaterialV27(TEXT("wf.Materials.ApplyExisting"),TEXT("0/1: compare accepted/V27 materials on the same frozen scene and camera."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args,UWorld* World)
    {
        const bool Enabled=Args.Num()==0 || FCString::Atoi(*Args[0])!=0;
        GMaterialV27.AsVariable()->Set(Enabled?1:0,ECVF_SetByConsole);
        FrontlineMaterialV27::ApplyWorld(World,Enabled);
    }));

